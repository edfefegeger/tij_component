#include "../include/TIJPrinter.h"
#include <stdexcept>
#include <sstream>
#include <cstring>

#pragma comment(lib, "ws2_32.lib")

TIJPrinter::TIJPrinter()
    : m_socket(INVALID_SOCKET)
    , m_connected(false)
    , m_slaveAddr(0x00)
{
    InitWinsock();
}

TIJPrinter::~TIJPrinter() {
    Disconnect();
    WSACleanup();
}

bool TIJPrinter::InitWinsock() {
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        m_lastError = "WSAStartup failed: " + std::to_string(result);
        return false;
    }
    return true;
}

bool TIJPrinter::Connect(const std::string& ip, int port) {
    Disconnect();

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        m_lastError = "socket() failed: " + std::to_string(WSAGetLastError());
        return false;
    }

    DWORD timeout = 5000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        m_lastError = "connect() failed: " + std::to_string(WSAGetLastError());
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    m_connected = true;
    return true;
}

void TIJPrinter::Disconnect() {
    if (m_socket != INVALID_SOCKET) {
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
    }
    m_connected = false;
}

bool TIJPrinter::IsConnected() const {
    return m_connected;
}

uint8_t TIJPrinter::CalcChecksum(const std::vector<uint8_t>& bytes) {
    uint32_t sum = 0;
    for (auto b : bytes) sum += b;
    return (uint8_t)(0x100 - (sum & 0xFF));
}

std::vector<uint8_t> TIJPrinter::BuildCommand(uint8_t cmdId, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> packet;
    packet.push_back(TIJ_ESC);
    packet.push_back(TIJ_STX);
    packet.push_back(m_slaveAddr);
    packet.push_back(cmdId);
    for (auto b : data) packet.push_back(b);
    packet.push_back(TIJ_ESC);
    packet.push_back(TIJ_ETX);

    std::vector<uint8_t> forChecksum(packet.begin() + 2, packet.end());
    packet.push_back(CalcChecksum(forChecksum));

    return packet;
}

bool TIJPrinter::SendRaw(const std::vector<uint8_t>& data) {
    int sent = send(m_socket, (const char*)data.data(), (int)data.size(), 0);
    if (sent == SOCKET_ERROR) {
        m_lastError = "send() failed: " + std::to_string(WSAGetLastError());
        m_connected = false;
        return false;
    }
    return true;
}

int TIJPrinter::ReceiveReply(int timeoutMs) {
    DWORD timeout = (DWORD)timeoutMs;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    uint8_t buf[64] = {};
    int received = recv(m_socket, (char*)buf, sizeof(buf), 0);

    if (received <= 0) {
        m_lastError = "recv() failed or timeout: " + std::to_string(WSAGetLastError());
        return -1;
    }

    if (received >= 5 && buf[0] == TIJ_ESC) {
        if (buf[1] == TIJ_ACK) {
            return buf[3]; // P_Status
        } else if (buf[1] == TIJ_NAK) {
            m_lastError = "NAK received, C_Status: " + std::to_string(buf[3]);
            return -2;
        }
    }

    m_lastError = "Invalid reply format";
    return -1;
}

int TIJPrinter::StartPrint() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    auto pkt = BuildCommand(CMD_START_PRINT, {});
    if (!SendRaw(pkt)) return -1;
    return ReceiveReply();
}

int TIJPrinter::StopPrint() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    auto pkt = BuildCommand(CMD_STOP_PRINT, {});
    if (!SendRaw(pkt)) return -1;
    return ReceiveReply();
}

int TIJPrinter::ClearBuffer() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    auto pkt = BuildCommand(CMD_CLEAR_BUFFER, {});
    if (!SendRaw(pkt)) return -1;
    return ReceiveReply();
}

int TIJPrinter::SendFieldData(const std::vector<std::pair<uint8_t, std::string>>& fields) {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    if (fields.empty()) { m_lastError = "No fields"; return -1; }

    std::vector<uint8_t> data;
    data.push_back((uint8_t)fields.size());

    for (auto& f : fields) {
        data.push_back(f.first);
        data.push_back((uint8_t)f.second.size());
        for (char c : f.second) data.push_back((uint8_t)c);
    }

    auto pkt = BuildCommand(CMD_FIELD_DATA, data);
    if (!SendRaw(pkt)) return -1;
    return ReceiveReply();
}

int TIJPrinter::SendField(uint8_t fieldId, const std::string& value) {
    return SendFieldData({{ fieldId, value }});
}

std::string TIJPrinter::GetLastError() const {
    return m_lastError;
}
