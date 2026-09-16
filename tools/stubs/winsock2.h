#ifndef STUB_WINSOCK2_H
#define STUB_WINSOCK2_H
#include <stdint.h>
/* На Windows winsock2.h подтягивает windows.h — повторяем это в заглушке. */
#include "windows.h"
typedef uintptr_t SOCKET;
#define INVALID_SOCKET (SOCKET)(~0)
#define SOCKET_ERROR (-1)
struct WSADATA { unsigned short wVersion; };
int WSAStartup(unsigned short v, struct WSADATA* d);
int WSACleanup(void);
#define MAKEWORD(a,b) ((unsigned short)(((a)&0xff)|(((b)&0xff)<<8)))
struct sockaddr { unsigned short sa_family; char sa_data[14]; };
struct in_addr { unsigned long s_addr; };
struct sockaddr_in { short sin_family; unsigned short sin_port; struct in_addr sin_addr; char sin_zero[8]; };
#define AF_INET 2
#define SOCK_STREAM 1
#define IPPROTO_TCP 6
#define SOL_SOCKET 0xffff
#define SO_RCVTIMEO 0x1006
#define SO_SNDTIMEO 0x1005
typedef unsigned short u_short;
int socket(int, int, int);
int connect(SOCKET, const struct sockaddr*, int);
int closesocket(SOCKET);
int send(SOCKET, const char*, int, int);
int recv(SOCKET, char*, int, int);
int setsockopt(SOCKET, int, int, const char*, int);
int WSAGetLastError(void);
unsigned short htons(unsigned short);
#endif
