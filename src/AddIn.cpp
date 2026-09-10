/*
    TIJ Printer External Component for 1C:Enterprise
    Native API, 64-bit, Windows
*/

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <vector>
#include <memory>
#include "../include/TIJPrinter.h"

// ---- 1C Native API types ----
typedef unsigned short WCHAR_T;

#define VTYPE_EMPTY  0
#define VTYPE_BOOL   11
#define VTYPE_I4     3
#define VTYPE_PWSTR  22
#define VTYPE_PSTR   8

#pragma pack(push, 1)
struct tVariant {
    union {
        int32_t     i32Val;
        int64_t     i64Val;
        bool        bVal;
        double      dblVal;
        struct {
            char*    pstrVal;
            uint32_t strLen;
        };
        struct {
            WCHAR_T* pwstrVal;
            uint32_t wstrLen;
        };
    };
    int32_t vt;
};
#pragma pack(pop)

class IMemoryManager {
public:
    virtual bool __stdcall AllocMemory(void** pMemory, unsigned long ulCountByte) = 0;
    virtual void __stdcall FreeMemory(void** pMemory) = 0;
};

class IAddInDefBase {
public:
    virtual bool __stdcall AddError(unsigned short, const WCHAR_T*, const WCHAR_T*, long) = 0;
    virtual bool __stdcall Read(WCHAR_T*, tVariant*, long*, WCHAR_T**) = 0;
    virtual bool __stdcall Write(WCHAR_T*, tVariant*) = 0;
    virtual bool __stdcall RegisterProfileAs(WCHAR_T*) = 0;
    virtual bool __stdcall SetEventBufferDepth(long) = 0;
    virtual long __stdcall GetEventBufferDepth() = 0;
    virtual bool __stdcall ExternalEvent(WCHAR_T*, WCHAR_T*, WCHAR_T*) = 0;
    virtual void __stdcall CleanEventBuffer() = 0;
    virtual bool __stdcall SetStatusLine(WCHAR_T*) = 0;
    virtual void __stdcall ResetStatusLine() = 0;
};

// ---- Helpers ----
static void SetWString(tVariant* pVar, const std::wstring& wstr, IMemoryManager* pMem) {
    size_t len = wstr.size();
    WCHAR_T* buf = nullptr;
    pMem->AllocMemory((void**)&buf, (unsigned long)((len + 1) * sizeof(WCHAR_T)));
    if (buf) {
        for (size_t i = 0; i < len; i++) buf[i] = (WCHAR_T)wstr[i];
        buf[len] = 0;
    }
    pVar->pwstrVal = buf;
    pVar->wstrLen  = (uint32_t)len;
    pVar->vt       = VTYPE_PWSTR;
}

static std::wstring GetWString(tVariant* pVar) {
    if (!pVar) return L"";
    if (pVar->vt == VTYPE_PWSTR && pVar->pwstrVal)
        return std::wstring(pVar->pwstrVal, pVar->pwstrVal + pVar->wstrLen);
    if (pVar->vt == VTYPE_PSTR && pVar->pstrVal)
        return std::wstring(pVar->pstrVal, pVar->pstrVal + pVar->strLen);
    return L"";
}

static std::string WStr2Str(const std::wstring& w) {
    return std::string(w.begin(), w.end());
}

// ---- Component ----
class CTIJComponent {
public:
    CTIJComponent() : m_pConnect(nullptr), m_pMemory(nullptr) {
        m_printer = std::make_unique<TIJPrinter>();
    }

    bool Init(void* pConn) {
        m_pConnect = (IAddInDefBase*)pConn;
        return true;
    }

    bool setMemManager(void* mem) {
        m_pMemory = (IMemoryManager*)mem;
        return true;
    }

    static const int METHOD_COUNT = 8;
    const wchar_t* methodNames[METHOD_COUNT] = {
        L"Connect", L"Disconnect", L"StartPrint", L"StopPrint",
        L"ClearBuffer", L"SendField", L"GetLastError", L"IsConnected"
    };

    long FindMethod(const WCHAR_T* name) {
        std::wstring n(name, name + wcslen((const wchar_t*)name));
        for (int i = 0; i < METHOD_COUNT; i++)
            if (n == methodNames[i]) return i;
        return -1;
    }

    bool CallMethod(long idx, tVariant* params, long paramCount, tVariant* ret) {
        switch (idx) {
        case 0: { // Connect(ip, port)
            if (paramCount < 1) return false;
            std::string ip = WStr2Str(GetWString(&params[0]));
            int port = 8010;
            if (paramCount >= 2 && params[1].vt == VTYPE_I4) port = params[1].i32Val;
            bool ok = m_printer->Connect(ip, port);
            if (ret) { ret->bVal = ok; ret->vt = VTYPE_BOOL; }
            return true;
        }
        case 1: { // Disconnect
            m_printer->Disconnect();
            if (ret) { ret->bVal = true; ret->vt = VTYPE_BOOL; }
            return true;
        }
        case 2: { // StartPrint
            int s = m_printer->StartPrint();
            if (ret) { ret->i32Val = s; ret->vt = VTYPE_I4; }
            return true;
        }
        case 3: { // StopPrint
            int s = m_printer->StopPrint();
            if (ret) { ret->i32Val = s; ret->vt = VTYPE_I4; }
            return true;
        }
        case 4: { // ClearBuffer
            int s = m_printer->ClearBuffer();
            if (ret) { ret->i32Val = s; ret->vt = VTYPE_I4; }
            return true;
        }
        case 5: { // SendField(id, value)
            if (paramCount < 2) return false;
            uint8_t id  = (uint8_t)params[0].i32Val;
            std::string val = WStr2Str(GetWString(&params[1]));
            int s = m_printer->SendField(id, val);
            if (ret) { ret->i32Val = s; ret->vt = VTYPE_I4; }
            return true;
        }
        case 6: { // GetLastError
            if (ret && m_pMemory) {
                std::string e = m_printer->GetLastError();
                SetWString(ret, std::wstring(e.begin(), e.end()), m_pMemory);
            }
            return true;
        }
        case 7: { // IsConnected
            if (ret) { ret->bVal = m_printer->IsConnected(); ret->vt = VTYPE_BOOL; }
            return true;
        }
        }
        return false;
    }

private:
    std::unique_ptr<TIJPrinter> m_printer;
    IAddInDefBase*  m_pConnect;
    IMemoryManager* m_pMemory;
};

// ---- DLL exports ----
static CTIJComponent* g_comp = nullptr;

extern "C" {

__declspec(dllexport) long GetClassObject(const WCHAR_T* wsName, void** pInterface) {
    if (!g_comp) g_comp = new CTIJComponent();
    *pInterface = (void*)g_comp;
    return 0;
}

__declspec(dllexport) long DestroyObject(void** pInterface) {
    if (g_comp) { delete g_comp; g_comp = nullptr; }
    return 0;
}

__declspec(dllexport) const WCHAR_T* GetClassNames() {
    static WCHAR_T name[] = { 'T','I','J','P','r','i','n','t','e','r', 0 };
    return name;
}

__declspec(dllexport) long GetAttachType() {
    return 2;
}

} // extern "C"

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
