#include "../include/TIJPrinter.h"
#include <cstring>
#include <cstdio>
#include <chrono>

/* --- Время: нужно для ожидания свободного места в очереди принтера --- */

static long NowMs() {
    using namespace std::chrono;
    return (long)duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count();
}

static void SleepMs(long ms) {
    if (ms > 0)
        ::Sleep((unsigned long)ms);
}

/* --- Расшифровка кодов протокола --- */

/* P_Status приходит в ответе подтверждением, раздел 4.2.1 протокола. */
static const char* DescribePrinterStatus(uint8_t status) {
    switch (status) {
    case 0x00: return "ошибок нет";
    case 0x01: return "есть ошибки принтера";
    case 0x0B: return "недопустимое число подмодулей QR-кода";
    case 0x0D: return "не задан серийный номер";
    case 0x0E: return "в команде меньше полей, чем в сообщении";
    case 0x0F: return "ошибка учётной записи";
    case 0x10: return "сообщение с таким именем не найдено";
    case 0x11: return "печать уже запущена";
    case 0x12: return "печать не запущена";
    case 0x13: return "ошибка режима группировки";
    case 0x14: return "интервал печати меньше минимально допустимого";
    case 0x15: return "не удалось сохранить сообщение";
    case 0x16: return "ошибка выбора колонок";
    case 0x17: return "принтер в режиме внутренней скорости";
    case 0x18: return "не удалось задать горизонтальное разрешение";
    case 0x42: return "команда заняла последнее свободное место в буфере";
    case 0x43: return "буфер был полон";
    case 0xFD: return "истёк срок обслуживания";
    case 0xFE: return "принтер разблокирован";
    case 0xFF: return "принтер заблокирован";
    default:   return "расшифровки нет";
    }
}

/* C_Status приходит в ответе отказом, раздел 4.1 протокола. */
static const char* DescribeCommandStatus(uint8_t status) {
    switch (status) {
    case 0x06: return "неверное начало команды";
    case 0x07: return "неверное окончание команды";
    case 0x08: return "неверная контрольная сумма";
    case 0x09: return "в команде не указано ни одного поля";
    case 0x0A: return "неизвестный тип модуля";
    case 0x11: return "неизвестная принтеру команда";
    case 0x3C: return "неверный режим печати";
    default:   return "расшифровки нет";
    }
}

/* Код выводится и шестнадцатеричным, и десятичным: в протоколе он записан hex. */
static std::string FormatCode(const char* prefix, uint8_t code, const char* text) {
    char buf[192];
    std::snprintf(buf, sizeof(buf), "%s=0x%02X (%u): %s", prefix,
                  (unsigned)code, (unsigned)code, text);
    return std::string(buf);
}

/* --- Реализация --- */

TIJPrinter::TIJPrinter()
    : m_socket(INVALID_SOCKET), m_connected(false), m_slaveAddr(0x00),
      m_lastCommandStatus(-1)
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
        m_lastError = "не удалось инициализировать сеть, код WSAStartup";
        return false;
    }
    return true;
}

bool TIJPrinter::Connect(const std::string& ip, int port) {
    m_lastError.clear();
    m_lastCommandStatus = -1;

    Disconnect();

    if (ip.empty()) {
        m_lastError = "не указан адрес принтера";
        return false;
    }

    m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (m_socket == INVALID_SOCKET) {
        m_lastError = "не удалось создать сокет, код " + std::to_string(WSAGetLastError());
        return false;
    }
    DWORD timeout = 5000;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));
    setsockopt(m_socket, SOL_SOCKET, SO_SNDTIMEO, (char*)&timeout, sizeof(timeout));

    sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    if (inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
        m_lastError = "неверный адрес принтера: " + ip;
        closesocket(m_socket);
        m_socket = INVALID_SOCKET;
        return false;
    }

    if (connect(m_socket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        m_lastError = "принтер " + ip + ":" + std::to_string(port) +
                      " недоступен, код " + std::to_string(WSAGetLastError());
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
    /*
     * Контрольная сумма считается от ВСЕХ байтов кадра, включая начальные
     * 1B 02 и конечные 1B 03 (раздел 2.4 протокола: "the modulo-256 sum of all
     * the bytes in the message"). В исходной версии первые два байта не
     * учитывались, поэтому каждый кадр уходил с неверной суммой и принтер
     * отвечал отказом.
     *
     * Проверка по примеру из протокола:
     *   1B 02 00 1D 01 01 0B 54 49 4A 20 50 72 69 6E 74 65 72 1B 03
     *   сумма всех байтов 450h, контрольная сумма 100h - 50h = B0h.
     */
    pkt.push_back(CalcChecksum(pkt));
    return pkt;
}

bool TIJPrinter::SendRaw(const std::vector<uint8_t>& data) {
    int sent = send(m_socket, (const char*)data.data(), (int)data.size(), 0);
    if (sent == SOCKET_ERROR) {
        m_lastError = "ошибка отправки, код " + std::to_string(WSAGetLastError());
        m_connected = false;
        return false;
    }
    return true;
}

int TIJPrinter::ReceiveFrame(int timeoutMs, uint8_t* out, int maxLen, int* pStatus) {
    m_lastCommandStatus = -1;
    if (pStatus) *pStatus = 0;

    DWORD timeout = (DWORD)timeoutMs;
    setsockopt(m_socket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout));

    uint8_t buf[64] = {};
    int received = recv(m_socket, (char*)buf, sizeof(buf), 0);
    if (received <= 0) {
        m_lastError = "нет ответа принтера за " + std::to_string(timeoutMs) +
                      " мс, код " + std::to_string(WSAGetLastError());
        return TIJ_ERR_NETWORK;
    }
    if (received >= 5 && buf[0] == TIJ_ESC) {
        if (buf[1] == TIJ_ACK) {
            if (pStatus) *pStatus = buf[3];
            const int n = (received < maxLen) ? received : maxLen;
            for (int i = 0; i < n; ++i) out[i] = buf[i];
            return received;
        }
        if (buf[1] == TIJ_NAK) {
            m_lastCommandStatus = buf[3];
            m_lastError = FormatCode("отказ принтера, C_Status", buf[3],
                                     DescribeCommandStatus(buf[3]));
            return TIJ_ERR_NAK;
        }
    }
    char hex[64] = {};
    for (int i = 0; i < received && i < 8; ++i)
        std::snprintf(hex + i * 3, 4, "%02X ", (unsigned)buf[i]);
    m_lastError = std::string("нераспознанный ответ принтера: ") + hex;
    return TIJ_ERR_NETWORK;
}

int TIJPrinter::ReceiveReply(int timeoutMs) {
    uint8_t buf[64] = {};
    int status = 0;
    const int len = ReceiveFrame(timeoutMs, buf, (int)sizeof(buf), &status);
    if (len < 0) return len;
    return status;
}

int TIJPrinter::StartPrint() {
    m_lastError.clear();
    if (!m_connected) { m_lastError = "нет подключения к принтеру"; return TIJ_ERR_NETWORK; }
    return SendRaw(BuildCommand(CMD_START_PRINT, {})) ? ReceiveReply() : TIJ_ERR_NETWORK;
}

int TIJPrinter::StopPrint() {
    m_lastError.clear();
    if (!m_connected) { m_lastError = "нет подключения к принтеру"; return TIJ_ERR_NETWORK; }
    return SendRaw(BuildCommand(CMD_STOP_PRINT, {})) ? ReceiveReply() : TIJ_ERR_NETWORK;
}

int TIJPrinter::ClearBuffer() {
    m_lastError.clear();
    if (!m_connected) { m_lastError = "нет подключения к принтеру"; return TIJ_ERR_NETWORK; }
    return SendRaw(BuildCommand(CMD_CLEAR_BUFFER, {})) ? ReceiveReply() : TIJ_ERR_NETWORK;
}

int TIJPrinter::SendField(long fieldId, const std::string& value) {
    m_lastError.clear();

    if (fieldId < 0 || fieldId > TIJ_MAX_VALUE_BYTES) {
        m_lastError = "номер поля " + std::to_string(fieldId) +
                      " вне диапазона 0..255, номер передаётся одним байтом";
        return TIJ_ERR_PARAM;
    }

    std::vector<TField> one;
    one.push_back(TField((uint8_t)fieldId, value));
    return SendFields(one);
}

/* --- Пакет полей --- */

void TIJPrinter::BeginPackage() {
    m_lastError.clear();
    m_pending.clear();
}

int TIJPrinter::AddField(long fieldId, const std::string& value) {
    m_lastError.clear();

    if (fieldId < 0 || fieldId > TIJ_MAX_VALUE_BYTES) {
        m_lastError = "номер поля " + std::to_string(fieldId) +
                      " вне диапазона 0..255, номер передаётся одним байтом";
        return TIJ_ERR_PARAM;
    }

    const std::string where = "поля " + std::to_string(fieldId);

    if (value.empty()) {
        m_lastError = "пустое значение для " + where +
                      ": протокол требует минимум один байт";
        return TIJ_ERR_PARAM;
    }
    if (value.size() > (size_t)TIJ_MAX_VALUE_BYTES) {
        m_lastError = "длина значения для " + where + " равна " +
                      std::to_string(value.size()) + " байт, допустимо не более " +
                      std::to_string(TIJ_MAX_VALUE_BYTES) +
                      ". Проверьте длину поля на принтере";
        return TIJ_ERR_PARAM;
    }
    if (m_pending.size() >= (size_t)TIJ_MAX_VALUE_BYTES) {
        m_lastError = "в пакете уже " + std::to_string(m_pending.size()) +
                      " полей, больше в один кадр не поместится";
        return TIJ_ERR_PARAM;
    }

    /* повторный вызов для того же номера заменяет значение, а не добавляет поле */
    for (size_t i = 0; i < m_pending.size(); ++i) {
        if (m_pending[i].first == (uint8_t)fieldId) {
            m_pending[i].second = value;
            return 0;
        }
    }

    m_pending.push_back(TField(fieldId, value));
    return 0;
}

int TIJPrinter::SendPackage() {
    if (m_pending.empty()) {
        m_lastError = "пакет пуст, сначала добавьте поля";
        return TIJ_ERR_PARAM;
    }

    const std::vector<TField> fields = m_pending;
    const int code = SendFields(fields);
    m_pending.clear();          /* отправка была попыткой, пакет всегда разбирается */
    return code;
}

/*
 * Все поля этикетки уходят одним кадром 1D (раздел 3.5.18 протокола):
 *
 *   1B 02 <адрес> 1D <число полей> <номер поля> <длина значения> <значение> ... 1B 03 <сумма>
 *
 * Одна команда занимает в буфере принтера одно место, поэтому поля этикетки
 * нельзя отправлять по одному: при буфере на N сообщений очередь переполнится
 * в N раз быстрее, а при настройке "вытеснять старое" на этикетку уедут
 * чужие данные.
 */
int TIJPrinter::SendFields(const std::vector<TField>& fields) {
    m_lastError.clear();

    /*
     * m_lastCommandStatus здесь не сбрасывается: если параметры неверны и
     * команда не ушла, расшифровка последнего отказа принтера должна остаться
     * доступной. Сбрасывает его ReceiveReply при каждой реальной отправке.
     */

    if (!m_connected) {
        m_lastError = "нет подключения к принтеру";
        return TIJ_ERR_NETWORK;
    }
    if (fields.empty()) {
        m_lastError = "пакет пуст, сначала добавьте поля";
        return TIJ_ERR_PARAM;
    }
    if (fields.size() > (size_t)TIJ_MAX_VALUE_BYTES) {
        m_lastError = "полей в пакете " + std::to_string(fields.size()) +
                      ", допустимо не более " + std::to_string(TIJ_MAX_VALUE_BYTES);
        return TIJ_ERR_PARAM;
    }

    std::vector<uint8_t> data;
    data.push_back((uint8_t)fields.size());

    for (size_t i = 0; i < fields.size(); ++i) {
        const uint8_t     id  = fields[i].first;
        const std::string& val = fields[i].second;
        const std::string where = "поля " + std::to_string((unsigned)id);

        if (val.empty()) {
            m_lastError = "пустое значение для " + where +
                          ": протокол требует минимум один байт";
            return TIJ_ERR_PARAM;
        }
        if (val.size() > TIJ_MAX_VALUE_BYTES) {
            m_lastError = "длина значения для " + where + " равна " +
                          std::to_string(val.size()) + " байт, допустимо не более " +
                          std::to_string(TIJ_MAX_VALUE_BYTES) + ". Проверьте длину поля на принтере";
            return TIJ_ERR_PARAM;
        }

        data.push_back(id);
        data.push_back((uint8_t)val.size());
        for (char c : val) data.push_back((uint8_t)c);
    }

    return SendRaw(BuildCommand(CMD_FIELD_DATA, data)) ? ReceiveReply() : TIJ_ERR_NETWORK;
}

/* --- Очередь буфера --- */

/*
 * Запрос числа сообщений в буфере, команда 33 (раздел 3.5.30 протокола).
 * Ответ: 1B 06 <адрес> <P_Status> 33 <число сообщений> 1B 03 <сумма>,
 * число сообщений лежит в пятом байте.
 */
int TIJPrinter::RequestBufferCount() {
    m_lastError.clear();

    if (!m_connected) {
        m_lastError = "нет подключения к принтеру";
        return TIJ_ERR_NETWORK;
    }

    if (!SendRaw(BuildCommand(CMD_BUFFER_COUNT, {})))
        return TIJ_ERR_NETWORK;

    uint8_t frame[64] = {};
    int status = 0;
    const int len = ReceiveFrame(2000, frame, (int)sizeof(frame), &status);
    if (len < 0)
        return len;

    if (len < 6 || frame[4] != CMD_BUFFER_COUNT) {
        m_lastError = "неожиданный ответ принтера на запрос буфера";
        return TIJ_ERR_NETWORK;
    }

    return frame[5];
}

/*
 * Отправка этикетки с ожиданием свободного места в очереди.
 *
 * Принтер держит в буфере до N сообщений (настройка буфера на самом принтере)
 * и забирает по одному на каждую напечатанную этикетку. Если очередь
 * переполнить, принтер либо отклонит команду, либо вытеснит самую старую
 * запись и напечатает на этикетке чужой код. Поэтому перед отправкой
 * спрашиваем занятость и ждём, пока место освободится.
 */
int TIJPrinter::SendLabel(long depth, long timeoutMs) {
    m_lastError.clear();

    if (m_pending.empty()) {
        m_lastError = "пакет пуст, сначала добавьте поля";
        return TIJ_ERR_PARAM;
    }

    if (depth > 0) {
        const long wait = (timeoutMs > 0) ? timeoutMs : (long)TIJ_QUEUE_WAIT_MS;
        const long deadline = NowMs() + wait;

        for (;;) {
            const int used = RequestBufferCount();
            if (used < 0) {
                /* Занятость узнать не удалось, печать не блокируем. */
                break;
            }
            if (used < depth)
                break;
            if (NowMs() >= deadline) {
                m_lastError = "в буфере принтера " + std::to_string(used) +
                              " сообщений, свободного места не появилось за " +
                              std::to_string(wait) + " мс";
                return TIJ_ERR_BUSY;
            }
            SleepMs(TIJ_QUEUE_POLL_MS);
        }
    }

    return SendPackage();
}

/*
 * Команда 35: вывод сигналов и режим переполнения буфера. Нас интересует
 * последний байт: 00 отклонять новую команду, 01 принимать с вытеснением
 * самой старой. Остальные поля не меняем, для этого служит FF.
 */
int TIJPrinter::SetRejectOnBufferFull(bool reject) {
    m_lastError.clear();

    if (!m_connected) {
        m_lastError = "нет подключения к принтеру";
        return TIJ_ERR_NETWORK;
    }

    std::vector<uint8_t> data;
    data.push_back(0xFF);                    /* сигнал триггера         не менять */
    data.push_back(0xFF);                    /* сигнал печати           не менять */
    data.push_back(0xFF);                    /* сигнал содержимого      не менять */
    data.push_back(0xFF);                    /* оставлять последнее поле не менять */
    data.push_back(0xFF);                    /* журнал печати           не менять */
    data.push_back(reject ? 0x00 : 0x01);    /* при переполнении: отклонять      */

    return SendRaw(BuildCommand(CMD_TRIGGER_SIG, data)) ? ReceiveReply() : TIJ_ERR_NETWORK;
}

std::string TIJPrinter::GetLastError() const { return m_lastError; }

std::string TIJPrinter::DescribeCode(int code) const {
    if (code >= 0)
        return FormatCode("P_Status", (uint8_t)code, DescribePrinterStatus((uint8_t)code));

    switch (code) {
    case TIJ_ERR_NETWORK:
        return "нет ответа принтера: таймаут 2 секунды, обрыв связи или нераспознанный ответ";
    case TIJ_ERR_NAK:
        if (m_lastCommandStatus >= 0)
            return FormatCode("отказ принтера, C_Status", (uint8_t)m_lastCommandStatus,
                              DescribeCommandStatus((uint8_t)m_lastCommandStatus));
        return "отказ принтера, причина в тексте последней ошибки";
    case TIJ_ERR_PARAM:
        return "параметры неверны, команда принтеру не отправлялась";
    case TIJ_ERR_BUSY:
        return "буфер принтера полон, свободного места не появилось за отведённое время";
    default:
        return "неизвестный код";
    }
}
