@echo off
setlocal

rem Проверка собранной DLL: разрядность, зависимости, экспортируемые функции.
rem dumpbin входит в Visual Studio, запускать из "Developer Command Prompt for VS 2022".

set DLL=%~1
if "%DLL%"=="" set DLL=%~dp0build\x64\Release\TIJPrinter1C.dll

if not exist "%DLL%" (
    echo Файл не найден: %DLL%
    exit /b 1
)

echo ============================================================
echo Файл: %DLL%
echo ============================================================
echo.
echo --- Разрядность (ожидается x64 или x86) ---
dumpbin /headers "%DLL%" | findstr /i "machine"
echo.
echo --- Зависимости (не должно быть MSVCP/VCRUNTIME/libstdc++/libgcc/api-ms-win-crt) ---
dumpbin /dependents "%DLL%"
echo.
echo --- Экспортируемые функции (ожидается 5 имён без искажений) ---
dumpbin /exports "%DLL%" | findstr /i "GetClassNames GetClassObject DestroyObject GetAttachType SetPlatformCapabilities"
echo.
exit /b 0
