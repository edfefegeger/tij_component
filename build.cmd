@echo off
setlocal

rem Сборка внешней компоненты.
rem
rem Первый аргумент: x64 по умолчанию, либо x86.
rem Запускать из командного окна Visual Studio:
rem   "x64 Native Tools Command Prompt" - для 64-битной сборки,
rem   "x86 Native Tools Command Prompt" - для 32-битной.
rem Версия Visual Studio значения не имеет.
rem
rem CMake здесь намеренно не используется: имя генератора CMake привязано к
rem версии Visual Studio, и на Build Tools 2026 сборка с генератором
rem "Visual Studio 17 2022" падает с ошибкой MSB8020, потому что набора
rem инструментов v143 в системе нет. Те же ключи сборки продублированы
rem в CMakeLists.txt, он нужен для сборки через CMake и для проверок на Linux.
rem
rem Правило для правки этого файла: в текстах echo и rem, которые находятся
rem внутри блоков if, не должно быть круглых скобок. cmd считает их границами
rem блока, разбор ломается и сборка падает с сообщением "Непредвиденное
rem появление". Вместо скобок ставьте тире или запятые.

set WANT=%~1
if "%WANT%"=="" set WANT=x64
if /i "%WANT%"=="Win32" set WANT=x86

if /i "%WANT%"=="x64" goto :arch_ok
if /i "%WANT%"=="x86" goto :arch_ok

echo Неизвестная разрядность: %WANT%. Ожидается x64 или x86.
exit /b 2

:arch_ok
where cl >nul 2>&1
if errorlevel 1 (
    echo.
    echo Не найден компилятор cl.exe.
    echo Откройте командное окно Visual Studio, нужен ярлык "x64 Native Tools"
    echo для 64 бит или "x86 Native Tools" для 32 бит.
    echo Затем запустите build.cmd оттуда.
    echo.
    exit /b 1
)

if defined VSCMD_ARG_TGT_ARCH (
    if /i not "%VSCMD_ARG_TGT_ARCH%"=="%WANT%" (
        echo.
        echo В этом окне настроена разрядность "%VSCMD_ARG_TGT_ARCH%", а запрошена "%WANT%".
        echo Откройте командное окно с нужной разрядностью и повторите.
        echo.
        exit /b 1
    )
)

set OUT=%~dp0build\%WANT%
if not exist "%OUT%" mkdir "%OUT%"

echo Сборка %WANT% ...
echo.

pushd "%OUT%"
cl /nologo /LD /MT /O2 /DNDEBUG /EHsc /utf-8 /W4 ^
   /I"%~dp0include" /I"%~dp0src" ^
   "%~dp0src\dllmain.cpp" "%~dp0src\AddInNative.cpp" "%~dp0src\TIJPrinter.cpp" ^
   /Fe:TIJPrinter1C.dll /link ws2_32.lib
if errorlevel 1 set BUILD_FAILED=1
popd

if defined BUILD_FAILED goto :fail

echo.
echo Готово: %OUT%\TIJPrinter1C.dll
echo Проверка: check.cmd "%OUT%\TIJPrinter1C.dll"
exit /b 0

:fail
echo.
echo Сборка завершилась с ошибкой.
exit /b 1
