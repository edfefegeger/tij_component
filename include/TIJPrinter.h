#pragma once
#include <string>
#include <vector>
#include <cstdint>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#define TIJ_ESC          0x1B
#define TIJ_STX          0x02
#define TIJ_ETX          0x03
#define TIJ_ACK          0x06
#define TIJ_NAK          0x15

#define CMD_START_PRINT  0x11
#define CMD_STOP_PRINT   0x12
#define CMD_FIELD_DATA   0x1D
#define CMD_CLEAR_BUFFER 0x21

class TIJPrinter {
public:
    TIJPrinter();
    ~TIJPrinter();

    bool Connect(const std::string& ip, int port = 8010);
    void Disconnect();
    bool IsConnected() const;

    int StartPrint();
    int StopPrint();
    int ClearBuffer();
    int SendField(uint8_t fieldId, const std::string& value);

    std::string GetLastError() const;

private:
    SOCKET      m_socket;
    bool        m_connected;
    std::string m_lastError;
    uint8_t     m_slaveAddr;

    std::vector<uint8_t> BuildCommand(uint8_t cmdId, const std::vector<uint8_t>& data);
    uint8_t CalcChecksum(const std::vector<uint8_t>& bytes);
    bool    SendRaw(const std::vector<uint8_t>& data);
    int     ReceiveReply(int timeoutMs = 2000);
    bool    InitWinsock();
};
