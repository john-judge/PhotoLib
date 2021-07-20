::
:: If Windows 10, load the attestation code signed driver
:: otherwise load the std. (still EV but not attestation signed) one
::
ver | findstr /C:"Version 10"
if errorlevel 1 (
  wdf_reload64 install\amd64 pdv
) else (
  wdf_reload64 install\win10_amd64 pdv
)

exit %errorlevel%


