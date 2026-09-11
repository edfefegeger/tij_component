#ifndef __COMPONENTBASE_H__
#define __COMPONENTBASE_H__

#include "types.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"

class IComponentBase
{
public:
    virtual ~IComponentBase() {}
    virtual bool ADDIN_API Init(void* pConnection) = 0;
    virtual bool ADDIN_API setMemManager(void* mem) = 0;
    virtual long ADDIN_API GetInfo() = 0;
    virtual void ADDIN_API Done() = 0;

    // Properties
    virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsExtensionName) = 0;
    virtual long ADDIN_API GetNProps() = 0;
    virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName) = 0;
    virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) = 0;
    virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) = 0;
    virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal) = 0;
    virtual bool ADDIN_API IsPropReadable(const long lPropNum) = 0;
    virtual bool ADDIN_API IsPropWritable(const long lPropNum) = 0;

    // Methods
    virtual long ADDIN_API GetNMethods() = 0;
    virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) = 0;
    virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum, const long lMethodAlias) = 0;
    virtual long ADDIN_API GetNParams(const long lMethodNum) = 0;
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
        tVariant* pvarParamDefValue) = 0;
    virtual bool ADDIN_API HasRetVal(const long lMethodNum) = 0;
    virtual bool ADDIN_API CallAsProc(const long lMethodNum,
        tVariant* paParams, const long lSizeArray) = 0;
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
        tVariant* paParams, const long lSizeArray) = 0;

    virtual void ADDIN_API SetLocale(const WCHAR_T* loc) = 0;
    virtual void ADDIN_API SetUserInterfaceLanguageCode(const WCHAR_T* lang) = 0;
};

#endif
