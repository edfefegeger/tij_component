#ifndef STUB_WINDOWS_H
#define STUB_WINDOWS_H
typedef int BOOL;
#define TRUE 1
#define FALSE 0
typedef unsigned long DWORD;
typedef void* LPVOID;
typedef void* HMODULE;
#define APIENTRY
#define CP_UTF8 65001
int WideCharToMultiByte(unsigned int cp, unsigned long flags, const wchar_t* src, int srcLen,
                        char* dst, int dstLen, const char* defChar, int* usedDefault);
int MultiByteToWideChar(unsigned int cp, unsigned long flags, const char* src, int srcLen,
                        wchar_t* dst, int dstLen);
void Sleep(unsigned long ms);
#endif
