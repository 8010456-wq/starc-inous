@echo off
REM ============================================================
REM  build_starcc.bat - rebuild the starc compiler with MSVC BuildTools
REM  (original starcc.exe is an MSVC build: api-ms-win-crt-* + vcruntime)
REM  NOTE: keep this file ASCII-only. cmd.exe mis-parses UTF-8 comments.
REM ============================================================
setlocal

set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Tools\MSVC\14.44.35207
set SDKROOT=C:\Program Files (x86)\Windows Kits\10

for /f "delims=" %%i in ('dir /b /o-n "%SDKROOT%\Include"') do (
    if not defined SDKVER set SDKVER=%%i
)
if not defined SDKVER (
    echo [ERROR] Windows SDK not found
    exit /b 1
)

set "INCLUDE=%VCDIR%\include;%SDKROOT%\Include\%SDKVER%\ucrt;%SDKROOT%\Include\%SDKVER%\shared;%SDKROOT%\Include\%SDKVER%\um"
set "LIB=%VCDIR%\lib\x64;%SDKROOT%\Lib\%SDKVER%\ucrt\x64;%SDKROOT%\Lib\%SDKVER%\um\x64"

cd /d "%~dp0"

REM /utf-8 is required: the sources contain Chinese comments and are saved as UTF-8.
REM Without it MSVC reads them as GBK and the code breaks.
"%VCDIR%\bin\Hostx64\x64\cl.exe" /nologo /O2 /W3 /utf-8 /D_CRT_SECURE_NO_WARNINGS /Fe:starcc.exe /Fo:build_starcc.obj starcc.c /link /INCREMENTAL:NO

if errorlevel 1 (
    echo [FAIL] compile error
    exit /b 1
)
echo [OK] starcc.exe rebuilt
endlocal
