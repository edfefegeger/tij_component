/*
 * Реализация функций Winsock поверх сокетов Linux.
 *
 * Нужна только для локальной проверки логики протокола без Windows, 1С и
 * принтера: tools/test-protocol.sh собирает с этим файлом src/TIJPrinter.cpp
 * и прогоняет все команды против имитатора принтера (tools/mock_printer.py).
 * В рабочую сборку под Windows этот файл не попадает.
 *
 * Системные заголовки Linux здесь намеренно не подключаются: их типы
 * (sockaddr, in_addr, SOCK_STREAM и другие) конфликтуют с типами из
 * winsock2.h. Поэтому к ядру обращаемся напрямую через syscall(), а мелкие
 * функции реализуем сами. Стенд рассчитан на x86_64.
 */

#include <winsock2.h>
#include <ws2tcpip.h>

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>

#if !defined(__x86_64__)
#error "posix_winsock.cpp рассчитан на x86_64"
#endif

extern "C" long syscall(long number, ...);

namespace {

/* Номера системных вызовов x86_64. */
constexpr long SYS_CLOSE      = 3;
constexpr long SYS_SOCKET     = 41;
constexpr long SYS_CONNECT    = 42;
constexpr long SYS_SENDTO     = 44;
constexpr long SYS_RECVFROM   = 45;
constexpr long SYS_SETSOCKOPT = 54;

/* Значения констант Linux. */
constexpr long AF_INET_L       = 2;
constexpr long SOCK_STREAM_L   = 1;
constexpr long IPPROTO_TCP_L   = 6;
constexpr long SOL_SOCKET_L    = 1;
constexpr long SO_RCVTIMEO_L   = 20;
constexpr long SO_SNDTIMEO_L   = 21;

int fdOf(SOCKET s) { return static_cast<int>(s); }

} /* namespace */

int WSAStartup(unsigned short, struct WSADATA*) { return 0; }

int WSACleanup(void) { return 0; }

int WSAGetLastError(void) { return errno; }

int socket(int, int, int) {
    const long fd = syscall(SYS_SOCKET, AF_INET_L, SOCK_STREAM_L, IPPROTO_TCP_L);
    if (fd < 0)
        return -1;
    return static_cast<int>(fd);
}

int connect(SOCKET s, const struct sockaddr* name, int namelen) {
    (void)namelen;

    /* Собираем обычную для Linux структуру адреса: 2 байта семейства,
     * 2 байта порта, 4 байта адреса, 8 байт выравнивания. */
    const sockaddr_in* win = reinterpret_cast<const sockaddr_in*>(name);
    unsigned char addr[16] = {};
    const unsigned short family = 2;                       /* AF_INET */
    std::memcpy(addr + 0, &family, sizeof(family));
    std::memcpy(addr + 2, &win->sin_port, sizeof(win->sin_port));
    std::memcpy(addr + 4, &win->sin_addr, 4);              /* адрес лежит в первых 4 байтах */

    const long rc = syscall(SYS_CONNECT, static_cast<long>(fdOf(s)),
                            reinterpret_cast<long>(addr), 16L);
    return static_cast<int>(rc);
}

int closesocket(SOCKET s) {
    return static_cast<int>(syscall(SYS_CLOSE, static_cast<long>(fdOf(s))));
}

int send(SOCKET s, const char* buf, int len, int) {
    return static_cast<int>(syscall(SYS_SENDTO, static_cast<long>(fdOf(s)),
                                    reinterpret_cast<long>(buf),
                                    static_cast<long>(len), 0L, 0L, 0L));
}

int recv(SOCKET s, char* buf, int len, int) {
    return static_cast<int>(syscall(SYS_RECVFROM, static_cast<long>(fdOf(s)),
                                    reinterpret_cast<long>(buf),
                                    static_cast<long>(len), 0L, 0L, 0L));
}

int setsockopt(SOCKET s, int, int optname, const char* optval, int) {
    /* В Windows таймауты задаются числом миллисекунд, в Linux ожидается
     * структура из двух длинных чисел: секунды и микросекунды. */
    if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO) {
        const DWORD ms = *reinterpret_cast<const DWORD*>(optval);
        long tv[2];
        tv[0] = static_cast<long>(ms / 1000);
        tv[1] = static_cast<long>((ms % 1000) * 1000);
        const long opt = (optname == SO_RCVTIMEO) ? SO_RCVTIMEO_L : SO_SNDTIMEO_L;
        return static_cast<int>(syscall(SYS_SETSOCKOPT, static_cast<long>(fdOf(s)),
                                        SOL_SOCKET_L, opt,
                                        reinterpret_cast<long>(tv), 16L));
    }
    return 0;
}

unsigned short htons(unsigned short value) {
    return static_cast<unsigned short>(__builtin_bswap16(value));
}

/* Аналог Windows Sleep: ожидание через nanosleep (номер вызова x86_64 = 35). */
void Sleep(unsigned long ms) {
    struct timespec { long tv_sec; long tv_nsec; } ts;
    ts.tv_sec  = static_cast<long>(ms / 1000);
    ts.tv_nsec = static_cast<long>((ms % 1000) * 1000000L);
    syscall(35, &ts, nullptr);
}

int inet_pton(int af, const char* src, void* dst) {
    if (af != 2 || src == nullptr || dst == nullptr)
        return 0;

    unsigned int a = 0, b = 0, c = 0, d = 0;
    if (std::sscanf(src, "%u.%u.%u.%u", &a, &b, &c, &d) != 4)
        return 0;
    if (a > 255 || b > 255 || c > 255 || d > 255)
        return 0;

    unsigned char* out = reinterpret_cast<unsigned char*>(dst);
    out[0] = static_cast<unsigned char>(a);
    out[1] = static_cast<unsigned char>(b);
    out[2] = static_cast<unsigned char>(c);
    out[3] = static_cast<unsigned char>(d);
    return 1;
}
