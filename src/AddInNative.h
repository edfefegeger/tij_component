#pragma once
#include "../include/ComponentBase.h"
#include "../include/TIJPrinter.h"
#include <string>
#include <memory>

class CAddInNative : public IComponentBase
{
public:
    CAddInNative();
    virtual ~CAddInNative();

    // IComponentBase
    bool ADDIN_API Init(void* pConnection) override;
    bool ADDIN_API setMemManager(void* mem) override;
    long ADDIN_API GetInfo() override { return 2000; }
    void ADDIN_API Done() override {}

    bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsExtensionName) override;

    long ADDIN_API GetNProps() override { return 0; }
    long ADDIN_API FindProp(const WCHAR_T*) override { return -1; }
    const WCHAR_T* ADDIN_API GetPropName(long, long) override { return nullptr; }
    bool ADDIN_API GetPropVal(const long, tVariant*) override { return false; }
    bool ADDIN_API SetPropVal(const long, tVariant*) override { return false; }
    bool ADDIN_API IsPropReadable(const long) override { return false; }
    bool ADDIN_API IsPropWritable(const long) override { return false; }

    long ADDIN_API GetNMethods() override;
    long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) override;
    const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias) override;
    long ADDIN_API GetNParams(const long lMethodNum) override;
    bool ADDIN_API GetParamDefValue(const long, const long, tVariant*) override { return false; }
    bool ADDIN_API HasRetVal(const long lMethodNum) override;
    bool ADDIN_API CallAsProc(const long lMethodNum, tVariant* paParams, const long lSizeArray) override;
    bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
        tVariant* paParams, const long lSizeArray) override;

    void ADDIN_API SetLocale(const WCHAR_T*) override {}

    /* В официальном заголовке метод возвращает код результата, 0 означает успех. */
    long ADDIN_API SetUserInterfaceLanguageCode(const WCHAR_T*) override { return 0; }

private:
    IAddInDefBase*  m_pConnect;
    IMemoryManager* m_pMemory;
    std::unique_ptr<TIJPrinter> m_printer;

    /* Кодировка значений: TIJ_ENC_UTF8 для полей с ASCII, TIJ_ENC_UTF16LE для Unicode. */
    long m_encoding;

    /*
     * Выделяет строку памятью платформы (IMemoryManager::AllocMemory) и копирует
     * в неё wstr. Освобождать память должна платформа, поэтому использовать new
     * или malloc здесь нельзя.
     */
    bool AllocWString(WCHAR_T** ppOut, const std::wstring& wstr);

    bool SetWString(tVariant* pVar, const std::wstring& wstr);

    static std::wstring GetWString(const tVariant* pVar);

    /* Числовой параметр: платформа может передать его любым числовым типом. */
    static long GetInt(const tVariant* pVar);

    /* Строки протокола принтера — байтовые, во встроенном языке — UTF-16. */
    static std::string  ToUtf8(const std::wstring& w);
    static std::string  ToUtf16Le(const std::wstring& w);
    static std::wstring FromUtf8(const std::string& s);

    /* Кодировка значения зависит от того, какой кодировщик выбран у поля на принтере. */
    std::string EncodeValue(const std::wstring& w) const;

    enum eMethods {
        eConnect = 0,
        eDisconnect,
        eStartPrint,
        eStopPrint,
        eClearBuffer,
        eSendField,
        eGetLastError,
        eIsConnected,
        eAddField,
        eSendPackage,
        eBeginPackage,
        eDescribeCode,
        eSetEncoding,
        eGetBufferUsed,
        eSendLabel,
        eSetRejectOnBufferFull,
        eMethLast
    };
};
