#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "AddInNative.h"

static CAddInNative* g_pAddIn = nullptr;
static long g_lObjectCount = 0;

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    return TRUE;
}

extern "C" {

__declspec(dllexport) long GetClassObject(const WCHAR_T* wsName, IComponentBase** pInterface) {
    if (!pInterface) return -1;
    g_pAddIn = new CAddInNative();
    *pInterface = (IComponentBase*)g_pAddIn;
    ++g_lObjectCount;
    return 0;
}

__declspec(dllexport) long DestroyObject(IComponentBase** pInterface) {
    if (!pInterface || !*pInterface) return -1;
    delete (CAddInNative*)*pInterface;
    *pInterface = nullptr;
    --g_lObjectCount;
    return 0;
}

__declspec(dllexport) const WCHAR_T* GetClassNames() {
    static WCHAR_T name[] = { 'T','I','J','P','r','i','n','t','e','r', 0 };
    return name;
}

__declspec(dllexport) long GetAttachType() {
    return 2; // eCanAttachAny
}

} // extern "C"
