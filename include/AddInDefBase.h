#ifndef __ADDINDEFBASE_H__
#define __ADDINDEFBASE_H__

#include "types.h"

class IAddInDefBase
{
public:
    /* В официальном заголовке деструктор объявлен первым, и это важно:
     * порядок виртуальных методов задает смещения в таблице, по которым
     * платформа вызывает методы. Без него все вызовы смещаются на один. */
    virtual ~IAddInDefBase() {}

    virtual bool ADDIN_API AddError(unsigned short wcode, const WCHAR_T* source,
        const WCHAR_T* descr, long scode) = 0;
    virtual bool ADDIN_API Read(WCHAR_T* varName, tVariant* pVal,
        long* pErrCode, WCHAR_T** errDescr) = 0;
    virtual bool ADDIN_API Write(WCHAR_T* varName, tVariant* pVal) = 0;
    virtual bool ADDIN_API RegisterProfileAs(WCHAR_T* wsProfileName) = 0;
    virtual bool ADDIN_API SetEventBufferDepth(long lDepth) = 0;
    virtual long ADDIN_API GetEventBufferDepth() = 0;
    virtual bool ADDIN_API ExternalEvent(WCHAR_T* wsSource, WCHAR_T* wsMessage,
        WCHAR_T* wsData) = 0;
    virtual void ADDIN_API CleanEventBuffer() = 0;
    virtual bool ADDIN_API SetStatusLine(WCHAR_T* wsStatusLine) = 0;
    virtual void ADDIN_API ResetStatusLine() = 0;
};

#endif
