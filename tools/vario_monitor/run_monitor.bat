@echo off
setlocal
cd /d "%~dp0"

echo ========================================================
echo   Lancement de VarioUSB Monitor (Télémétrie & Enregistreur)
echo ========================================================
echo.

py -3 main.py

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo Une erreur est survenue lors de l'execution avec 'py'.
    echo Essai avec 'python'...
    python main.py
)

pause
