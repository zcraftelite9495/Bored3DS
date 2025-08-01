@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: ====== Configurable Project Name ======
set "projectName=Bored3DS"
echo Starting build for project: %projectName%
echo +========== BUILD START ==========+

:: ====== Banner Audio Builder ======
echo.
echo ===== BANNER AUDIO CONVERSION =====
IF EXIST ".\assets\bannerSource.wav" ( set "BannerAudioSource=bannerSource.wav" )
IF EXIST ".\assets\bannerSource.mp3" ( set "BannerAudioSource=bannerSource.mp3" )

.\utils\ffmpeg\ffmpeg -y -i ".\assets\%BannerAudioSource%" -t 3 -ar 44100 -ac 2 ".\assets\banner.wav" >nul 2>&1

IF %ERRORLEVEL% NEQ 0 (
    echo Banner audio conversion failed.
    EXIT /B %ERRORLEVEL%
)

echo Generated ... banner.wav

:: ====== Banner Builder ======
echo.
echo ======== BANNER GENERATION ========

.\utils\bannertool\bannertool makebanner ^
  -i ".\assets\banner.png" ^
  -a ".\assets\banner.wav" ^
  -o ".\assets\banner.bnr"


IF %ERRORLEVEL% NEQ 0 (
    echo Banner creation failed.
    EXIT /B %ERRORLEVEL%
)

echo Generated ... banner.bnr

:: ====== Build Project ======
echo.
echo ========== PROJECT BUILD ==========

@make >nul 2>&1

IF %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    EXIT /B %ERRORLEVEL%
)

:: ====== Create Output Folder ======
IF NOT EXIST output mkdir output >nul 2>&1
sleep 1

:: ====== Copy build files ======
@move "%projectName%.3dsx" ".\output\%projectName%.3dsx" >nul 2>&1
@move "%projectName%.elf"  ".\output\%projectName%.elf" >nul 2>&1
@move "%projectName%.smdh" ".\output\%projectName%.smdh" >nul 2>&1

echo Built ... %projectName%.3dsx
echo Built ... %projectName%.elf
echo Built ... %projectName%.smdh

:: ====== Build CXI ======
echo.
echo ============ CXI BUILD ============

.\utils\cxitool\cxitool ^
    ".\output\%projectName%.3dsx" ^
    ".\output\%projectName%.cxi" ^
    --name="%projectName%" ^
    --banner=.\assets\banner.bnr

IF %ERRORLEVEL% NEQ 0 (
    echo CXI build failed.
    EXIT /B %ERRORLEVEL%
)

echo Built ... %projectName%.cxi

:: ====== Build CIA ======
echo.
echo ============ CIA BUILD ============

.\utils\makerom\makerom ^
    -f cia ^
    -o ".\output\%projectName%.cia" ^
    -i ".\output\%projectName%.cxi:0:0" ^
    -ignoresign

IF %ERRORLEVEL% NEQ 0 (
    echo CIA build failed.
    EXIT /B %ERRORLEVEL%
)

echo Built ... %projectName%.cia

:: ====== Select Testing Method ======
if "%1"=="--useEmulator" (
    set "testdevice=2"
) else if "%1"=="--use3DS" (
    set "testdevice=1"
) else if "%1"=="" (
    echo.
    echo ========== TEST SOFTWARE ==========
    echo 1 - Send to physical 3DS via servefile
    echo 2 - Run in Lime3DS emulator
    set /p testdevice="Choose a test device [1/2]: "
) else if "%1%"=="-h" (
    echo Usage: make3ds.bat [--useEmulator | --use3DS]
    echo.
    echo --useEmulator: Run the build in Lime3DS emulator.
    echo --use3DS: Send the build to a physical 3DS.
    exit /b 0
)

if "%testdevice%"=="1" (
    echo Open FBI on your 3DS and choose 'Remote Install' -> 'Receive URLs over the network'
    ".\utils\servefile\start.bat" ".\output\%projectName%.cia"
) else if "%testdevice%"=="2" (
    echo Launching Lime3DS...
    taskkill /f /im lime3ds-gui.exe >nul 2>&1
    start "" "D:\Software\Lime3DS\lime3ds-gui.exe" ".\output\%projectName%.3dsx"
) else (
    echo Invalid choice. Skipping test launch.
)

:: ====== Exit Message ======
echo.
echo +========= BUILD COMPLETE =========+

ENDLOCAL
