
set

@setlocal

@if "%PROCESSOR_ARCHITEW6432%" == "AMD64" set ARCH=64
@if "%PROCESSOR_ARCHITECTURE%" == "IA64" set ARCH=64
@if "%PROCESSOR_ARCHITECTURE%" == "AMD64" set ARCH=64

@if "%ARCH%" == "64" (
	call bin64.bat
	cd wdf
	call reload64.bat
	cd ..
	cd redist
    @echo running redist/vcredist_x64 ...
	call vcredist_x64.exe /q
	cd ..
) else (
	call bin32.bat
	cd wdf
	call reload32.bat
	cd ..
	cd redist
    @echo running redist/vcredist_x86 ...
	call vcredist_x86.exe /q
	cd ..
)

@endlocal
