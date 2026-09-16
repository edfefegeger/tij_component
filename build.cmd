@echo off
setlocal

rem Сборка компоненты. Первый аргумент — разрядность: x64 (по умолчанию), Win32 или x86.
rem Запускать из "Developer Command Prompt for VS 2022" либо из консоли, где доступны cmake и MSBuild.

set ARCH=%~1
if "%ARCH%"=="" set ARCH=x64
if /i "%ARCH%"=="x86" set ARCH=Win32

echo Сборка %ARCH% ...
echo.

cmake -S "%~dp0." -B "%~dp0build\%ARCH%" -G "Visual Studio 17 2022" -A %ARCH%
if errorlevel 1 goto :fail

cmake --build "%~dp0build\%ARCH%" --config Release
if errorlevel 1 goto :fail

echo.
echo Готово: %~dp0build\%ARCH%\Release\TIJPrinter1C.dll
echo Проверка: check.cmd "%~dp0build\%ARCH%\Release\TIJPrinter1C.dll"
exit /b 0

:fail
echo.
echo Сборка завершилась с ошибкой.
exit /b 1
