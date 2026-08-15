@echo off
echo ===================================================
echo   EchoValhalla SuperPlugin Build & Installer Script
echo ===================================================

:: 1. Check for CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake not found. Attempting to install via winget...
    winget install -e --id Kitware.CMake --accept-package-agreements --accept-source-agreements
)

:: 2. Check for Inno Setup (ISCC.exe)
set "ISCC_PATH=C:\Program Files (x86)\Inno Setup 6\ISCC.exe"
if not exist "%ISCC_PATH%" (
    where ISCC.exe >nul 2>nul
    if %ERRORLEVEL% NEQ 0 (
        echo [!] Inno Setup compiler ISCC not found. Attempting to install via winget...
        winget install -e --id JRSoftware.InnoSetup --accept-package-agreements --accept-source-agreements
    ) else (
        set "ISCC_PATH=ISCC.exe"
    )
)

:: 3. Configure CMake
echo.
echo [*] Configuring CMake Build System...
if not exist "build" mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
if %ERRORLEVEL% NEQ 0 (
    echo [!] CMake Configuration Failed!
    exit /b %ERRORLEVEL%
)

:: 4. Build VST3 in Release Mode
echo.
echo [*] Compiling VST3 in Release Mode...
cmake --build . --config Release
if %ERRORLEVEL% NEQ 0 (
    echo [!] Compilation Failed!
    exit /b %ERRORLEVEL%
)

cd ..

:: 5. Run Inno Setup Compiler
echo.
echo [*] Packaging Installer using Inno Setup Compiler...
"%ISCC_PATH%" installer.iss
if %ERRORLEVEL% NEQ 0 (
    echo [!] Inno Setup Packaging Failed!
    exit /b %ERRORLEVEL%
)

echo.
echo ===================================================
echo   BUILD SUCCESSFUL!
echo   Installer created in output directory.
echo ===================================================
pause
