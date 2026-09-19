#ifndef __COMPONENTBASE_H__
#define __COMPONENTBASE_H__

/*
 * Интерфейсы объекта компоненты Native API.
 *
 * ВАЖНО: это структура официального ComponentBase.h из комплекта
 * "Технология создания внешних компонент", и она принципиальна.
 *
 * Разбивка на три базовых интерфейса и множественное наследование задают
 * раскладку таблиц виртуальных методов в объекте компоненты. Платформа 1С
 * собрана по официальной раскладке: у объекта три таблицы, и методы второй и
 * третьей группы (RegisterExtensionAs, GetNMethods, CallAsFunc, SetLocale)
 * платформа вызывает через отдельные указатели со смещением. Если объявить
 * все методы одним плоским классом, раскладка получается другой: вызовы
 * первой группы ещё попадают куда нужно, а начиная с RegisterExtensionAs
 * платформа обращается не туда, и подключение молча не проходит.
 *
 * Поэтому менять состав интерфейсов и порядок методов в них нельзя.
 */

#include "types.h"
#include "AddInDefBase.h"
#include "IMemoryManager.h"

////////////////////////////////////////////////////////////////////////////////
/* Инициализация компоненты. */
class IInitDoneBase
{
public:
    virtual ~IInitDoneBase() {}

    virtual bool ADDIN_API Init(void* disp) = 0;
    virtual bool ADDIN_API setMemManager(void* mem) = 0;
    virtual long ADDIN_API GetInfo() = 0;
    virtual void ADDIN_API Done() = 0;
};

////////////////////////////////////////////////////////////////////////////////
/* Свойства и методы, видимые из встроенного языка. */
class ILanguageExtenderBase
{
public:
    virtual ~ILanguageExtenderBase() {}

    virtual bool ADDIN_API RegisterExtensionAs(WCHAR_T** wsExtensionName) = 0;

    virtual long ADDIN_API GetNProps() = 0;
    virtual long ADDIN_API FindProp(const WCHAR_T* wsPropName) = 0;
    virtual const WCHAR_T* ADDIN_API GetPropName(long lPropNum, long lPropAlias) = 0;
    virtual bool ADDIN_API GetPropVal(const long lPropNum, tVariant* pvarPropVal) = 0;
    virtual bool ADDIN_API SetPropVal(const long lPropNum, tVariant* varPropVal) = 0;
    virtual bool ADDIN_API IsPropReadable(const long lPropNum) = 0;
    virtual bool ADDIN_API IsPropWritable(const long lPropNum) = 0;

    virtual long ADDIN_API GetNMethods() = 0;
    virtual long ADDIN_API FindMethod(const WCHAR_T* wsMethodName) = 0;
    virtual const WCHAR_T* ADDIN_API GetMethodName(const long lMethodNum,
        const long lMethodAlias) = 0;
    virtual long ADDIN_API GetNParams(const long lMethodNum) = 0;
    virtual bool ADDIN_API GetParamDefValue(const long lMethodNum, const long lParamNum,
        tVariant* pvarParamDefValue) = 0;
    virtual bool ADDIN_API HasRetVal(const long lMethodNum) = 0;
    virtual bool ADDIN_API CallAsProc(const long lMethodNum,
        tVariant* paParams, const long lSizeArray) = 0;
    virtual bool ADDIN_API CallAsFunc(const long lMethodNum, tVariant* pvarRetValue,
        tVariant* paParams, const long lSizeArray) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/* Локализация компоненты. */
class LocaleBase
{
public:
    virtual ~LocaleBase() {}

    virtual void ADDIN_API SetLocale(const WCHAR_T* loc) = 0;

    /* Добавлен в поздних версиях платформы, поэтому объявлен последним:
     * порядок методов в интерфейсах платформа читает по смещениям. */
    virtual long ADDIN_API SetUserInterfaceLanguageCode(const WCHAR_T* lang) = 0;
};

////////////////////////////////////////////////////////////////////////////////
/* Общий интерфейс: объект компоненты наследуется от него и реализует все
 * методы трех интерфейсов выше. */
class IComponentBase :
    public IInitDoneBase,
    public ILanguageExtenderBase,
    public LocaleBase
{
public:
    virtual ~IComponentBase() {}
};

/* Возможности платформы, о поддержке которых компонента сообщает
 * в SetPlatformCapabilities(). */
enum AppCapabilities
{
    eAppCapabilitiesInvalid = -1,
    eAppCapabilities1 = 1,
    eAppCapabilitiesLast = eAppCapabilities1,
};

/* Сигнатуры функций, которые библиотека компоненты должна экспортировать. */
typedef long (*GetClassObjectPtr)(const WCHAR_T* wsName, IComponentBase** pIntf);
typedef long (*DestroyObjectPtr)(IComponentBase** pIntf);
typedef const WCHAR_T* (*GetClassNamesPtr)();
typedef AppCapabilities (*SetPlatformCapabilitiesPtr)(const AppCapabilities capabilities);

#endif /* __COMPONENTBASE_H__ */
