#include "AddInNative.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstring>

/*
 * Имена методов: [0] — латинское, [1] — локализованное.
 * Количество строк обязано совпадать с CAddInNative::eMethLast,
 * а g_MethodParams и g_MethodHasRetVal — идти в том же порядке.
 */
static const wchar_t* g_MethodNames[][2] = {
    { L"Connect",      L"Подключить"       },
    { L"Disconnect",   L"Отключить"        },
    { L"StartPrint",   L"НачатьПечать"     },
    { L"StopPrint",    L"ОстановитьПечать" },
    { L"ClearBuffer",  L"ОчиститьБуфер"    },
    { L"SendField",    L"ОтправитьПоле"    },
    { L"GetLastError", L"ПолучитьОшибку"   },
    { L"IsConnected",  L"ЕстьПодключение"  }
};

static const long g_MethodParams[]    = { 2, 0, 0, 0, 0, 2, 0, 0 };
static const bool g_MethodHasRetVal[] = { true, true, true, true, true, true, true, true };

CAddInNative::CAddInNative() : m_pConnect(nullptr), m_pMemory(nullptr)
{
    try {
        m_printer = std::make_unique<TIJPrinter>();
    }
    catch (...) {
        m_printer.reset();
    }
}

CAddInNative::~CAddInNative() {}

bool ADDIN_API CAddInNative::Init(void* pConnection)
{
    m_pConnect = static_cast<IAddInDefBase*>(pConnection);
    return true;
}

bool ADDIN_API CAddInNative::setMemManager(void* mem)
{
    m_pMemory = static_cast<IMemoryManager*>(mem);
    return m_pMemory != nullptr;
}

bool ADDIN_API CAddInNative::RegisterExtensionAs(WCHAR_T** wsExtensionName)
{
    return AllocWString(wsExtensionName, L"TIJPrinter");
}

bool CAddInNative::AllocWString(WCHAR_T** ppOut, const std::wstring& wstr)
{
    if (!ppOut)
        return false;

    *ppOut = nullptr;

    if (!m_pMemory)
        return false;

    const unsigned long bytes =
        static_cast<unsigned long>((wstr.size() + 1) * sizeof(WCHAR_T));

    if (!m_pMemory->AllocMemory(reinterpret_cast<void**>(ppOut), bytes))
        return false;

    if (!*ppOut)
        return false;

    for (size_t i = 0; i < wstr.size(); ++i)
        (*ppOut)[i] = static_cast<WCHAR_T>(wstr[i]);

    (*ppOut)[wstr.size()] = 0;
    return true;
}

long ADDIN_API CAddInNative::GetNMethods() { return eMethLast; }

long ADDIN_API CAddInNative::FindMethod(const WCHAR_T* wsMethodName)
{
    if (!wsMethodName)
        return -1;

    std::wstring name;
    for (const WCHAR_T* p = wsMethodName; *p; ++p)
        name += static_cast<wchar_t>(*p);

    for (int i = 0; i < eMethLast; i++) {
        if (name == g_MethodNames[i][0] || name == g_MethodNames[i][1])
            return i;
    }
    return -1;
}

const WCHAR_T* ADDIN_API CAddInNative::GetMethodName(const long lMethodNum, const long lMethodAlias)
{
    if (lMethodNum < 0 || lMethodNum >= eMethLast)
        return nullptr;

    const wchar_t* name = g_MethodNames[lMethodNum][lMethodAlias == 0 ? 0 : 1];

    WCHAR_T* buf = nullptr;
    if (!AllocWString(&buf, std::wstring(name)))
        return nullptr;

    return buf;
}

long ADDIN_API CAddInNative::GetNParams(const long lMethodNum)
{
    if (lMethodNum < 0 || lMethodNum >= eMethLast)
        return 0;
    return g_MethodParams[lMethodNum];
}

bool ADDIN_API CAddInNative::HasRetVal(const long lMethodNum)
{
    if (lMethodNum < 0 || lMethodNum >= eMethLast)
        return false;
    return g_MethodHasRetVal[lMethodNum];
}

/* --- Helpers --- */

bool CAddInNative::SetWString(tVariant* pVar, const std::wstring& wstr)
{
    if (!pVar)
        return false;

    WCHAR_T* buf = nullptr;
    if (!AllocWString(&buf, wstr))
        return false;

    pVar->pwstrVal = buf;
    pVar->wstrLen  = static_cast<uint32_t>(wstr.size());
    pVar->vt       = VTYPE_PWSTR;
    return true;
}

std::wstring CAddInNative::GetWString(const tVariant* pVar)
{
    if (!pVar)
        return std::wstring();

    if (pVar->vt == VTYPE_PWSTR && pVar->pwstrVal) {
        size_t len = pVar->wstrLen;
        if (len == 0) {                       /* длина не заполнена — считаем по нулю */
            while (pVar->pwstrVal[len])
                ++len;
        }
        std::wstring r;
        r.reserve(len);
        for (size_t i = 0; i < len; ++i)
            r += static_cast<wchar_t>(pVar->pwstrVal[i]);
        return r;
    }

    if (pVar->vt == VTYPE_PSTR && pVar->pstrVal) {
        size_t len = pVar->strLen;
        if (len == 0) {
            while (pVar->pstrVal[len])
                ++len;
        }
        return FromUtf8(std::string(pVar->pstrVal, len));
    }

    return std::wstring();
}

long CAddInNative::GetInt(const tVariant* pVar)
{
    if (!pVar)
        return 0;

    /*
     * Ветка default повторяет поведение исходной версии (чтение val_i32).
     * Значения неизвестных типов поэтому проверяются на стороне вызывающего:
     * порт ограничен диапазоном 1..65535, номер поля — 0..255.
     */
    switch (pVar->vt) {
    case VTYPE_I2:   return pVar->val_i16;
    case VTYPE_I4:   return pVar->val_i32;
    case VTYPE_BOOL: return pVar->val_bool ? 1 : 0;
    case VTYPE_R4:   return static_cast<long>(pVar->val_flt);
    case VTYPE_R8:   return static_cast<long>(pVar->val_dbl);
    default:         return pVar->val_i32;
    }
}

std::string CAddInNative::ToUtf8(const std::wstring& w)
{
    if (w.empty())
        return std::string();

    const int srcLen = static_cast<int>(w.size());
    const int need = ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), srcLen,
                                           nullptr, 0, nullptr, nullptr);
    if (need <= 0)
        return std::string();

    std::string out(static_cast<size_t>(need), '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, w.c_str(), srcLen, &out[0], need, nullptr, nullptr);
    return out;
}

std::wstring CAddInNative::FromUtf8(const std::string& s)
{
    if (s.empty())
        return std::wstring();

    const int srcLen = static_cast<int>(s.size());
    const int need = ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), srcLen, nullptr, 0);
    if (need <= 0)
        return std::wstring();

    std::wstring out(static_cast<size_t>(need), L'\0');
    ::MultiByteToWideChar(CP_UTF8, 0, s.c_str(), srcLen, &out[0], need);
    return out;
}

/* --- Dispatch --- */

bool ADDIN_API CAddInNative::CallAsProc(const long lMethodNum,
    tVariant* paParams, const long lSizeArray)
{
    tVariant dummy = {};
    return CallAsFunc(lMethodNum, &dummy, paParams, lSizeArray);
}

bool ADDIN_API CAddInNative::CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
    tVariant* paParams, const long lSizeArray)
{
    if (!pvarRetValue)
        return false;

    pvarRetValue->vt = VTYPE_EMPTY;

    if (!m_printer)
        return false;

    switch (lMethodNum) {
    case eConnect: {
        if (lSizeArray < 1)
            return false;

        const std::string ip = ToUtf8(GetWString(&paParams[0]));

        int port = 8010;
        if (lSizeArray >= 2) {
            const long p = GetInt(&paParams[1]);
            if (p > 0 && p <= 65535)
                port = static_cast<int>(p);
        }

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
        if (lSizeArray < 2)
            return false;

        const long rawId = GetInt(&paParams[0]);
        if (rawId < 0 || rawId > 255)          /* номер поля передаётся одним байтом */
            return false;

        const uint8_t id = static_cast<uint8_t>(rawId);
        const std::string val = ToUtf8(GetWString(&paParams[1]));

        pvarRetValue->val_i32 = m_printer->SendField(id, val);
        pvarRetValue->vt      = VTYPE_I4;
        return true;
    }
    case eGetLastError: {
        return SetWString(pvarRetValue, FromUtf8(m_printer->GetLastError()));
    }
    case eIsConnected: {
        pvarRetValue->val_bool = m_printer->IsConnected();
        pvarRetValue->vt       = VTYPE_BOOL;
        return true;
    }
    }

    return false;
}
