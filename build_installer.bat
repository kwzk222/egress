@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   EchoValhalla SuperPlugin Build & Installer Script
echo ===================================================

:: 1. Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake not found in PATH.
    echo [*] Attempting to install CMake via winget...
    winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Failed to install CMake automatically. Please install CMake manually and re-run.
        goto ERROR_EXIT
    )
)

:: 2. Check for Inno Setup (ISCC.exe)
set "ISCC_PATH=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "!ISCC_PATH!" (
    where ISCC.exe >nul 2>nul
    if %ERRORLEVEL% EQU 0 (
        set "ISCC_PATH=ISCC.exe"
    ) else (
        echo [!] Inno Setup compiler ISCC not found.
        echo [*] Attempting to install Inno Setup via winget...
        winget install -e --id JRSoftware.InnoSetup --accept-package-agreements --accept-source-agreements
        if %ERRORLEVEL% NEQ 0 (
            echo [!] Failed to install Inno Setup automatically. Please install Inno Setup manually and re-run.
            goto ERROR_EXIT
        )
    )
)

:: 3. Configure CMake
echo.
echo [*] Configuring CMake Build System in Release mode...
if not exist "build" mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake Configuration Failed!
    goto ERROR_EXIT_CD
)

:: 4. Build VST3 in Release Mode
echo.
echo [*] Compiling VST3 in Release Mode...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] Compilation Failed!
    goto ERROR_EXIT_CD
)

cd ..

:: 5. Run Inno Setup Compiler
echo.
echo [*] Packaging Installer using Inno Setup Compiler...
if not exist installer.iss (
    echo [!] installer.iss file not found in current directory!
    goto ERROR_EXIT
)

"!ISCC_PATH!" installer.iss
if %ERRORLEVEL% NEQ 0 (
    echo [!] Inno Setup Packaging Failed!
    goto ERROR_EXIT
)

echo.
echo ===================================================
echo   BUILD & PACKAGING SUCCESSFUL!
echo   Installer setup .exe has been created.
echo ===================================================
pause
exit /b 0

:ERROR_EXIT_CD
cd ..

:ERROR_EXIT
echo.
echo ===================================================
echo   BUILD FAILED - SEE ERROR MESSAGES ABOVE.
echo ===================================================
echo Press any key to close this window...
pause >nul
exit /b 1
