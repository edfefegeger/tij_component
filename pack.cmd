@echo off
setlocal

rem Упаковка DLL и MANIFEST.XML в ZIP для загрузки компоненты в конфигурацию (макет).
rem Сначала соберите нужные разрядности: build.cmd x64 и build.cmd x86
rem
rem Аргументы пробрасываются в tools\pack.ps1, например:
rem   pack.cmd
rem   pack.cmd -MinGW64 build\mingw\TIJPrinter1C.dll

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\pack.ps1" %*
exit /b %errorlevel%
