#include "../include/TIJPrinter.h"
#include <cstring>

TIJPrinter::TIJPrinter()
    : m_socket(INVALID_SOCKET), m_connected(false), m_slaveAddr(0x00)
{
    InitWinsock();
}

TIJPrinter::~TIJPrinter() {
    Disconnect();
    WSACleanup();
}

bool TIJPrinter::InitWinsock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        m_lastError = "WSAStartup failed";
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

bool TIJPrinter::IsConnected() const { return m_connected; }

uint8_t TIJPrinter::CalcChecksum(const std::vector<uint8_t>& bytes) {
    uint32_t sum = 0;
    for (auto b : bytes) sum += b;
    return (uint8_t)(0x100 - (sum & 0xFF));
}

std::vector<uint8_t> TIJPrinter::BuildCommand(uint8_t cmdId, const std::vector<uint8_t>& data) {
    std::vector<uint8_t> pkt;
    pkt.push_back(TIJ_ESC);
    pkt.push_back(TIJ_STX);
    pkt.push_back(m_slaveAddr);
    pkt.push_back(cmdId);
    for (auto b : data) pkt.push_back(b);
    pkt.push_back(TIJ_ESC);
    pkt.push_back(TIJ_ETX);
    std::vector<uint8_t> forCsum(pkt.begin()+2, pkt.end());
    pkt.push_back(CalcChecksum(forCsum));
    return pkt;
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
        m_lastError = "recv() failed: " + std::to_string(WSAGetLastError());
        return -1;
    }
    if (received >= 5 && buf[0] == TIJ_ESC) {
        if (buf[1] == TIJ_ACK) return buf[3];
        if (buf[1] == TIJ_NAK) {
            m_lastError = "NAK, C_Status=" + std::to_string(buf[3]);
            return -2;
        }
    }
    m_lastError = "Invalid reply";
    return -1;
}

int TIJPrinter::StartPrint() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    return SendRaw(BuildCommand(CMD_START_PRINT, {})) ? ReceiveReply() : -1;
}

int TIJPrinter::StopPrint() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    return SendRaw(BuildCommand(CMD_STOP_PRINT, {})) ? ReceiveReply() : -1;
}

int TIJPrinter::ClearBuffer() {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    return SendRaw(BuildCommand(CMD_CLEAR_BUFFER, {})) ? ReceiveReply() : -1;
}

int TIJPrinter::SendField(uint8_t fieldId, const std::string& value) {
    if (!m_connected) { m_lastError = "Not connected"; return -1; }
    std::vector<uint8_t> data;
    data.push_back(0x01); // number of fields
    data.push_back(fieldId);
    data.push_back((uint8_t)value.size());
    for (char c : value) data.push_back((uint8_t)c);
    return SendRaw(BuildCommand(CMD_FIELD_DATA, data)) ? ReceiveReply() : -1;
}

std::string TIJPrinter::GetLastError() const { return m_lastError; }
