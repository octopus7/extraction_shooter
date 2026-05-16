@echo off
setlocal

set "SCRIPT_DIR=%~dp0"
set "FFMPEG=%SCRIPT_DIR%ffmpeg\bin\ffmpeg.exe"
set "INPUT=%SCRIPT_DIR%intro04b.MP4"
set "OUTPUT=%SCRIPT_DIR%intro04b_0_2_5s.mp4"

if not exist "%FFMPEG%" (
    echo ffmpeg.exe not found: "%FFMPEG%"
    exit /b 1
)

if not exist "%INPUT%" (
    echo Input file not found: "%INPUT%"
    exit /b 1
)

"%FFMPEG%" -hide_banner -y -ss 0 -t 2.5 -i "%INPUT%" -map 0:v:0 -an -c:v libx264 -crf 18 -preset medium -movflags +faststart "%OUTPUT%"

if errorlevel 1 (
    echo Failed to cut video.
    exit /b 1
)

echo Created: "%OUTPUT%"
endlocal
