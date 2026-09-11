#include "AddInNative.h"
#include <cstring>

static const wchar_t* g_MethodNames[][2] = {
    { L"Connect",      L"Подключить"       },
    { L"Disconnect",   L"Отключить"        },
    { L"StartPrint",   L"НачатьПечать"     },
    { L"StopPrint",    L"ОстановитьПечать" },
    { L"ClearBuffer",  L"ОчиститьБуфер"   },
    { L"SendField",    L"ОтправитьПоле"   },
    { L"GetLastError", L"ПолучитьОшибку"  },
    { L"IsConnected",  L"ЕстьПодключение" }
};

static const long g_MethodParams[]    = { 2, 0, 0, 0, 0, 2, 0, 0 };
static const bool g_MethodHasRetVal[] = { true, true, true, true, true, true, true, true };

CAddInNative::CAddInNative() : m_pConnect(nullptr), m_pMemory(nullptr) {
    m_printer = std::make_unique<TIJPrinter>();
}

CAddInNative::~CAddInNative() {}

bool ADDIN_API CAddInNative::Init(void* pConnection) {
    m_pConnect = (IAddInDefBase*)pConnection;
    return true;
}

bool ADDIN_API CAddInNative::setMemManager(void* mem) {
    m_pMemory = (IMemoryManager*)mem;
    return true;
}

bool ADDIN_API CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName) {
    const wchar_t* name = L"TIJPrinter";
    size_t len = wcslen(name);
    m_pMemory->AllocMemory((void**)wsExtensionName, (unsigned long)((len + 1) * sizeof(WCHAR_T)));
    if (*wsExtensionName) {
        for (size_t i = 0; i <= len; i++)
            (*wsExtensionName)[i] = (WCHAR_T)name[i];
    }
    return true;
}

long ADDIN_API CAddInNative::GetNMethods() { return eMethLast; }

long ADDIN_API CAddInNative::FindMethod(const WCHAR_T* wsMethodName) {
    if (!wsMethodName) return -1;
    std::wstring name;
    for (const WCHAR_T* p = wsMethodName; *p; ++p)
        name += (wchar_t)*p;
    for (int i = 0; i < eMethLast; i++) {
        if (name == g_MethodNames[i][0] || name == g_MethodNames[i][1])
            return i;
    }
    return -1;
}

const WCHAR_T* ADDIN_API CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias) {
    if (lMethodNum < 0 || lMethodNum >= eMethLast) return nullptr;
    const wchar_t* name = g_MethodNames[lMethodNum][lMethodAlias == 0 ? 0 : 1];
    size_t len = wcslen(name);
    WCHAR_T* buf = nullptr;
    m_pMemory->AllocMemory((void**)&buf, (unsigned long)((len + 1) * sizeof(WCHAR_T)));
    if (buf) {
        for (size_t i = 0; i <= len; i++) buf[i] = (WCHAR_T)name[i];
    }
    return buf;
}

long ADDIN_API CAddInNative::GetNParams(const long lMethodNum) {
    if (lMethodNum < 0 || lMethodNum >= eMethLast) return 0;
    return g_MethodParams[lMethodNum];
}

bool ADDIN_API CAddInNative::HasRetVal(const long lMethodNum) {
    if (lMethodNum < 0 || lMethodNum >= eMethLast) return false;
    return g_MethodHasRetVal[lMethodNum];
}

// --- Helpers ---
void CAddInNative::SetWString(tVariant* pVar, const std::wstring& wstr) {
    size_t len = wstr.size();
    WCHAR_T* buf = nullptr;
    m_pMemory->AllocMemory((void**)&buf, (unsigned long)((len + 1) * sizeof(WCHAR_T)));
    if (buf) {
        for (size_t i = 0; i < len; i++) buf[i] = (WCHAR_T)wstr[i];
        buf[len] = 0;
    }
    pVar->pwstrVal = buf;
    pVar->wstrLen  = (uint32_t)len;
    pVar->vt       = VTYPE_PWSTR;
}

std::wstring CAddInNative::GetWString(tVariant* pVar) {
    if (!pVar) return L"";
    if (pVar->vt == VTYPE_PWSTR && pVar->pwstrVal) {
        std::wstring r;
        for (uint32_t i = 0; i < pVar->wstrLen; i++) r += (wchar_t)pVar->pwstrVal[i];
        return r;
    }
    if (pVar->vt == VTYPE_PSTR && pVar->pstrVal) {
        return std::wstring(pVar->pstrVal, pVar->pstrVal + pVar->strLen);
    }
    return L"";
}

std::string CAddInNative::WStr2Str(const std::wstring& w) {
    return std::string(w.begin(), w.end());
}

std::wstring CAddInNative::Str2WStr(const std::string& s) {
    return std::wstring(s.begin(), s.end());
}

// --- Dispatch ---
bool ADDIN_API CAddInNative::CallAsProc(const long lMethodNum,
    tVariant* paParams, const long lSizeArray) {
    tVariant dummy = {};
    return CallAsFunc(lMethodNum, &dummy, paParams, lSizeArray);
}

bool ADDIN_API CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
    tVariant* paParams, const long lSizeArray) {
    if (!pvarRetValue) return false;
    pvarRetValue->vt = VTYPE_EMPTY;

    switch (lMethodNum) {
    case eConnect: {
        if (lSizeArray < 1) return false;
        std::string ip = WStr2Str(GetWString(&paParams[0]));
        int port = 8010;
        if (lSizeArray >= 2 && paParams[1].vt == VTYPE_I4)
            port = paParams[1].val_i32;
        pvarRetValue->val_bool = m_printer->Connect(ip, port);
        pvarRetValue->vt       = VTYPE_BOOL;
        return true;
    }
    case eDisconnect: {
        m_printer->Disconnect();
        pvarRetValue->val_bool = true;
        pvarRetValue->vt       = VTYPE_BOOL;
        return true;
    }
    case eStartPrint: {
        pvarRetValue->val_i32 = m_printer->StartPrint();
        pvarRetValue->vt      = VTYPE_I4;
        return true;
    }
    case eStopPrint: {
        pvarRetValue->val_i32 = m_printer->StopPrint();
        pvarRetValue->vt      = VTYPE_I4;
        return true;
    }
    case eClearBuffer: {
        pvarRetValue->val_i32 = m_printer->ClearBuffer();
        pvarRetValue->vt      = VTYPE_I4;
        return true;
    }
    case eSendField: {
        if (lSizeArray < 2) return false;
        uint8_t id  = (uint8_t)paParams[0].val_i32;
        std::string val = WStr2Str(GetWString(&paParams[1]));
        pvarRetValue->val_i32 = m_printer->SendField(id, val);
        pvarRetValue->vt      = VTYPE_I4;
        return true;
    }
    case eGetLastError: {
        SetWString(pvarRetValue, Str2WStr(m_printer->GetLastError()));
        return true;
    }
    case eIsConnected: {
        pvarRetValue->val_bool = m_printer->IsConnected();
        pvarRetValue->vt       = VTYPE_BOOL;
        return true;
    }
    }
    return false;
}
