#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "AddInNative.h"

#include <new>

/*
 * Имя объекта компоненты, которое видно из встроенного языка 1С как
 * третья часть строки: Новый("AddIn.<ИмяПодключения>.TIJPrinter").
 * Должно совпадать с тем, что возвращает CAddInNative::RegisterExtensionAs().
 */
static const WCHAR_T kClassName[] = { 'T', 'I', 'J', 'P', 'r', 'i', 'n', 't', 'e', 'r', 0 };

static long g_lObjectCount = 0;

static bool ClassNameEquals(const WCHAR_T* wsName, const WCHAR_T* wsSample)
{
    if (!wsName || !wsSample)
        return false;

    while (*wsName && *wsSample) {
        if (*wsName != *wsSample)
            return false;
        ++wsName;
        ++wsSample;
    }
    return *wsName == 0 && *wsSample == 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    (void)hModule;
    (void)ul_reason_for_call;
    (void)lpReserved;
    return TRUE;
}

extern "C" {

/* Список имён объектов компоненты. */
__declspec(dllexport) const WCHAR_T* GetClassNames()
{
    return kClassName;
}

/*
 * Создание экземпляра объекта компоненты.
 *
 * ВАЖНО: ненулевое возвращаемое значение означает УСПЕХ, ноль — ошибку
 * ("если объект не может быть создан или не найден объект с указанным
 * именем — возвращается 0"). В предыдущей версии было наоборот, из-за чего
 * объект не создавался даже при успешном подключении компоненты.
 */
__declspec(dllexport) long GetClassObject(const WCHAR_T* clsName, IComponentBase** pIntf)
{
    if (!pIntf)
        return 0;

    if (!ClassNameEquals(clsName, kClassName))
        return 0;

    CAddInNative* pAddIn = new (std::nothrow) CAddInNative();
    if (!pAddIn)
        return 0;

    *pIntf = static_cast<IComponentBase*>(pAddIn);
    ++g_lObjectCount;
    return 1;
}

/*
 * Удаление ранее созданного объекта.
 * ВАЖНО: здесь семантика обратная — 0 означает успех, иначе код ошибки.
 */
__declspec(dllexport) long DestroyObject(IComponentBase** pIntf)
{
    if (!pIntf || !*pIntf)
        return -1;

    delete *pIntf;
    *pIntf = nullptr;
    --g_lObjectCount;
    return 0;
}

/*
 * Версия возможностей платформы, с которой умеет работать компонента.
 * Без этой функции компоненте недоступны вывод сообщений и запрос
 * информации о платформе.
 */
__declspec(dllexport) AppCapabilities SetPlatformCapabilities(const AppCapabilities capabilities)
{
    (void)capabilities;
    return eAppCapabilities1;
}

/*
 * Тип подключения, который поддерживает компонента.
 *
 * Было: return 2 (eCanAttachIsolated) при комментарии "eCanAttachAny".
 * Компонента заявляла, что умеет работать только изолированно, тогда как
 * на клиенте платформа подключает компоненты НЕ изолированно — и подключение
 * не выполнялось. Возвращаем eCanAttachAny (3).
 */
__declspec(dllexport) AttachType GetAttachType()
{
    return eCanAttachAny;
}

} /* extern "C" */
