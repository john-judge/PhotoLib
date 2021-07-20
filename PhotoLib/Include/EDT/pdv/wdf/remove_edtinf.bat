
SETLOCAL ENABLEDELAYEDEXPANSION

@echo off

:Loop
FOR /F "tokens=1-7" %%a IN ('pnputil -e') DO (
    IF %%a == Published ( set oemfile=%%d )
    IF %%c == provider (
        set devstr=%%e%%f%%g
        IF !devstr! == EDT (pnputil -f -d !oemfile!) 
        IF !devstr! == EngineeringDesignTeam (pnputil -f -d !oemfile!) 
    )
)


