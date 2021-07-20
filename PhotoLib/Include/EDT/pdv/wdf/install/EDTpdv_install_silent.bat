
:: Windows .bat file example of silent install for EDT PDV VisionLink driver packages
::
:: Exits with 0 if success, 1 otherwise
::
:: EXAMPLE ONLY -- no guarantees implied or otherwise
::

:: Modify this to point to a valid location / name for the current or desired
:: package version
set EDTINSTALLER=c:\Windows\temp\EDTpdv_5.5.1.7.exe

:: Modify this to point to some location where the EDT certificate has been
:: stored on your system or network. The EDT certificate file can be copied
:: from existing installation, by default C:\EDT\pdv\wdf\install\EDTcertificate.cer.
set CERTIFICATE=c:\Windows\temp\EDTcertificate.cer

:: This adds EDT certificate to the trusted publisher store on the target system,
:: and will prevent the interactive Trusted Publisher dialog from coming up
:: and causing the installation to wait for user input to proceed
certutil.exe -addstore TrustedPublisher %CERTIFICATE%
@if ERRORLEVEL 1 goto EXIT_FAIL

:: Use 'start' with these args so the install blocks instead of returning immediately
start /B /WAIT %EDTINSTALLER% /S
@if ERRORLEVEL 1 goto EXIT_FAIL

:: After installation, you can uncomment these two lines if you want to delete the
:: cert. from the trusted publisher store for some reason
::certutil.exe -delstore TrustedPublisher  %CERTIFICATE%
::@if ERRORLEVEL 1 goto EXIT_FAIL

:: Silent install doesn't reboot which is required by the EDT pkg install, so if
:: you want this script to do that, uncomment the below, or do it later as appropriate
::shutdown -r -f -t 0

:: OR simply exit
exit /b 0

:EXIT_FAIL
exit /b 1

