@echo off
setlocal

REM ============================================================
REM STM32CubeIDE Project Clone Utility
REM ============================================================

if "%~1"=="" goto usage
if "%~2"=="" goto usage

set SRC=%~1
set DST=%~2

echo.
echo [INFO] Using Python : python3.exe
echo [INFO] Source       : %SRC%
echo [INFO] New name     : %DST%
echo.

python stm32cubeide_clone.py "%SRC%" "%DST%"

if errorlevel 1 (
    echo.
    echo [FAILED]
    pause
    exit /b 1
)

echo.
echo [SUCCESS]
pause
exit /b 0

:usage

echo.
echo Usage:
echo.
echo   stm32cubeide_clone.bat SOURCE_PROJECT TARGET_PROJECT
echo.
echo Example:
echo.
echo   stm32cubeide_clone.bat l433_spi_master_test l433_spi_master_poc
echo.

pause