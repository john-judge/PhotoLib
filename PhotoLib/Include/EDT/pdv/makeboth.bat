@echo off

:: set various global vars
set board=PDV
set LOG=CHANGELOG_PDV.txt
set /P VER=< version

echo ========================================
echo makeboth: Building %board% version %VER%
echo ========================================

call findstr %VER% %LOG%

@if NOT ERRORLEVEL 1 goto changelog_ok
echo.
echo No entry for ver. %VER% found in changelog. To correct, ctrl-c now
echo and update shared/%LOG% then re-run getsource -s, and this script. Or
pause
:changelog_ok

:: set the QTDIR basename 
@if "%QTDIR%" == "" (set QTDIR=c:\Qt\4.8.4)
set envstr=%QTDIR%
call :strLen envstr strlen
set baselen=%strlen%
set endswith=%QTDIR:~-4%
@if "%endswith%" == "_x64" (set /a baselen=%strlen%-4)
set startchar=0
call set QTBASE=%%QTDIR:~%startchar%,%baselen%%%

:: set the Microsoft Visual Studio executable dir based on Windows version
for /f "tokens=4-5 delims=. " %%i in ('ver') do set VERSION=%%i.%%j
set MSV_DIR="C:\Program Files (x86)\Microsoft Visual Studio 9.0\vc\bin"
@if "%version%" == "10.0" (set MSV_DIR="C:\Program Files (x86)\Microsoft Visual Studio\2019\Professional\VC\Auxiliary\Build")

setlocal

set BUILD_ID=1001

echo ----------------------------------------
echo makeboth: Building 32-bit
echo ----------------------------------------
call %MSV_DIR%"\vcvars32.bat"

@if NOT ERRORLEVEL 1 goto check_vcvars32_success
echo "failed vcvars32.bat -- check your build environment!"
endlocal
@exit /b 1
:check_vcvars32_success

which cl | findstr 64
@if NOT ERRORLEVEL 1 (
    set ARCH=amd64
    set QTDIR=%QTBASE%_x64
) else (
    set ARCH=x86
    set QTDIR=%QTBASE%
)
@echo "ARCH = %ARCH%"
@echo QTDIR = %QTDIR%

qmake --version |findstr /I /C:"Using Qt version 4"
@if NOT ERRORLEVEL 1 goto check_qmake32_success
echo "%0%: missing qmake or invalid qmake/Qt version -- is Qt %QTDIR%/bin installed and in the path?"
@exit /b 1
endlocal
:check_qmake32_success

:: checking for 64-bit QT now so we can abort now rather than  going to
:: all the trouble of building 32-bit then finding out its not there
%QTDIR%_x64\bin\qmake.exe --version |findstr /I /C:"Using Qt version 4"
@if NOT ERRORLEVEL 1 goto check_qmake64_success
@echo "%0%: missing 64-bit qmake or invalid version -- is %QTDIR%_x64/bin installed and in the path?"
@endlocal
@exit /b 1
:check_qmake64_success

nmake -C -f makefile.pkg full

@if NOT ERRORLEVEL 1 goto make_32_success
@echo "%0%: nmake returned ERROR %errorlevel%, exiting"
goto:error_exit

:make_32_success

endlocal

echo ----------------------------------------
echo makeboth: Building 64-bit
echo ----------------------------------------
setlocal

set BUILD_ID=1001

call %MSV_DIR%"\vcvarsx86_amd64.bat"

@if NOT ERRORLEVEL 1 goto check_vcvars64_success
@echo "%0%: failed vcvars64.bat -- check your build environment!"
@endlocal
@exit /b 1
:check_vcvars64_success

which cl | findstr 64
@if %errorlevel% EQU 1 (
    set ARCH=x86
    set QTDIR=%QTBASE%
) else (
    set ARCH=amd64
    set QTDIR=%QTBASE%_x64
)
@echo "ARCH = %ARCH%"
@echo "QTDIR = %QTDIR%"

nmake -C -f makefile.pkg %1%

@if NOT ERRORLEVEL 1 goto end
@echo "%0%: nmake %1% returned ERROR %errorlevel%, exiting"
goto:error_exit

:strLen
setlocal enabledelayedexpansion
:strLen_Loop
  if not "!%1:~%len%!"=="" set /A len+=1 & goto :strLen_Loop
  (endlocal & set %2=%len%)
goto :eof

:end
@endlocal
@exit /b 0

:error_exit
@endlocal
exit /b 1
