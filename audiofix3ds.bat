@ECHO OFF
SETLOCAL ENABLEEXTENSIONS

:: ====== 3DS Audio Formatter (OGG Output) ======
set file=%~1
set outputFile=%~2

.\utils\ffmpeg\ffmpeg -y -i "%file%" -ar 44100 -ac 2 -c:a libvorbis "%outputFile%" >nul 2>&1

IF %ERRORLEVEL% NEQ 0 (
    echo Audio conversion failed.
    EXIT /B %ERRORLEVEL%
)

echo Audio conversion successful.
