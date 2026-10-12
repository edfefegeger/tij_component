#pragma once
#include <string>
#include <vector>
#include <utility>
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
#define CMD_BUFFER_COUNT 0x33
#define CMD_TRIGGER_SIG  0x35

/*
 * Коды возврата методов.
 * Неотрицательное значение это статус принтера P_Status из ответа подтверждением.
 */
#define TIJ_ERR_NETWORK  (-1)   /* ответа нет: таймаут, обрыв, нераспознанный кадр */
#define TIJ_ERR_NAK      (-2)   /* принтер ответил отказом, причина в C_Status */
#define TIJ_ERR_PARAM    (-3)   /* параметры неверны, команда не отправлялась */
#define TIJ_ERR_BUSY     (-4)   /* очередь принтера не освободилась за отведённое время */

/* Ожидание свободного места в очереди: как часто спрашивать и сколько ждать всего. */
#define TIJ_QUEUE_POLL_MS   200
#define TIJ_QUEUE_WAIT_MS   60000

/* Длина значения поля в кадре передаётся одним байтом. */
#define TIJ_MAX_VALUE_BYTES 255

/* Кодировка значения при отправке в принтер. */
#define TIJ_ENC_UTF8     0      /* поля с кодировщиком ASCII */
#define TIJ_ENC_UTF16LE  1      /* поля с кодировщиком Unicode */

class TIJPrinter {
public:
    /* Номер поля и значение в байтах той кодировки, которая нужна принтеру. */
    typedef std::pair<uint8_t, std::string> TField;

    TIJPrinter();
    ~TIJPrinter();

    bool Connect(const std::string& ip, int port = 8010);
    void Disconnect();
    bool IsConnected() const;

    int StartPrint();
    int StopPrint();
    int ClearBuffer();

    /* Отправка одного поля отдельной командой. Номер поля проверяется здесь. */
    int SendField(long fieldId, const std::string& value);

    /* Пакет: несколько полей уходят одним кадром, занимая одно место в буфере. */
    void BeginPackage();
    int  AddField(long fieldId, const std::string& value);
    int  SendPackage();

    /*
     * Отправка этикетки с ожиданием свободного места в очереди принтера.
     * depth — сколько сообщений разрешено держать в буфере, 0 значит не ждать.
     * timeoutMs — сколько всего ждать освобождения места, 0 значит взять умолчание.
     */
    int SendLabel(long depth, long timeoutMs);

    /* Сколько сообщений сейчас в буфере принтера, команда 33. */
    int RequestBufferCount();

    /* Режим переполнения буфера, команда 35: запретить вытеснение старой записи. */
    int SetRejectOnBufferFull(bool reject);

    std::string GetLastError() const;

    /* Расшифровка кода возврата: P_Status, C_Status и кодов компоненты. */
    std::string DescribeCode(int code) const;

private:
    SOCKET      m_socket;
    bool        m_connected;
    std::string m_lastError;
    uint8_t     m_slaveAddr;
    int         m_lastCommandStatus;  /* C_Status последнего отказа, для расшифровки */
    std::vector<TField> m_pending;    /* накопленные поля пакета */

    int SendFields(const std::vector<TField>& fields);
    std::vector<uint8_t> BuildCommand(uint8_t cmdId, const std::vector<uint8_t>& data);
    uint8_t CalcChecksum(const std::vector<uint8_t>& bytes);
    bool    SendRaw(const std::vector<uint8_t>& data);

    /*
     * Читает ответ принтера. Возвращает длину принятого кадра или отрицательный
     * код ошибки. В pStatus кладёт P_Status для подтверждения или C_Status для отказа.
     */
    int     ReceiveFrame(int timeoutMs, uint8_t* out, int maxLen, int* pStatus);
    int     ReceiveReply(int timeoutMs = 2000);
    bool    InitWinsock();
};
