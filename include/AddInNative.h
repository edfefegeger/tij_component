#pragma once

#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

// Native API 1C types
typedef unsigned short WCHAR_T;
typedef void* IMemoryManager;

// Component methods enum
enum Methods {
    eMethConnect = 0,
    eMethDisconnect,
    eMethSendField,
    eMethStartPrint,
    eMethStopPrint,
    eMethClearBuffer,
    eMethGetStatus,
    eMethCount
};

// Component properties enum  
enum Props {
    ePropHost = 0,
    ePropPort,
    ePropSlaveAddress,
    ePropLastError,
    ePropConnected,
    ePropCount
};
