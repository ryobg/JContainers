@echo off
setlocal

set SKSE64_ROOT=%~dp0\..\dep\skse64

echo Unpack and merge SKSE 64...

"%~dp0\7za.exe" -aos x "%SKSE64_ROOT%\skse64.7z" -o"%SKSE64_ROOT%"
if errorlevel 1 (
    echo Unable to unpack SKSE, exiting...
    exit /b 1
)

set SKSEGOG_ROOT=%~dp0\..\dep\sksegog

echo Unpack and merge SKSE GOG...

"%~dp0\7za.exe" -aos x "%SKSEGOG_ROOT%\sksegog.7z" -o"%SKSEGOG_ROOT%"
if errorlevel 1 (
    echo Unable to unpack SKSE, exiting...
    exit /b 1
)

set SKSEVR_ROOT=%~dp0\..\dep\sksevr

echo Unpack and merge SKSE VR...

"%~dp0\7za.exe" -aos x "%SKSEVR_ROOT%\sksevr.7z" -o"%SKSEVR_ROOT%"
if errorlevel 1 (
    echo Unable to unpack SKSE VR, exiting...
    exit /b 1
)

