SETLOCAL ENABLEDELAYEDEXPANSION

@echo off

takeown /F %SYSTEMROOT%\System32\drivers\pdvwdf.sys
echo y | cacls %SYSTEMROOT%\System32\drivers\pdvwdf.sys /G %username%:F
del %SYSTEMROOT%\System32\drivers\pdvwdf.sys

