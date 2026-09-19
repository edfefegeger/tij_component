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
    /*
     * Имя класса намеренно не проверяется.
     *
     * Платформа сама решает, какое имя сюда передать. В официальном шаблоне 1С
     * этот параметр не анализируется: "поскольку у нас только один класс, то это
     * имя здесь вообще не проверяется, просто создается объект через new".
     * Строгая проверка приводит к тому, что вызов платформы отбрасывается,
     * и при создании объекта встроенный язык сообщает "Тип не определен".
     */
    (void)clsName;

    if (!pIntf)
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
