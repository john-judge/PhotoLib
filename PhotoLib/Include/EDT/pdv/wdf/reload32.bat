::
:: If Windows 10, load the attestation code signed driver
:: otherwise load the std. (still EV but not attestation signed) one
::
ver | findstr /C:"Version 10"
if errorlevel 1 (
  wdf_reload install\x86 pdv
) else (
  wdf_reload install\win10_x86 pdv
)

exit %errorlevel%

