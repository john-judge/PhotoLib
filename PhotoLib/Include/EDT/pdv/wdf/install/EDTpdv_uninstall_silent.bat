::
:: Windows .bat file example of silent UNinstall for EDT PDV driver packages
::
:: EXAMPLE ONLY -- no guarantees implied or otherwise
::
:: Uninstall EDT driver package silently
::

start /B /WAIT Uninstall_pdv.exe /S

:: OPTIONALLY uncomment the below to remove the certificate from the public store
:: (no real reason to do this however)
::certutil -delstore TrustedPublisher "71afce488a4b4dbb184941c7eda0d1c7"
