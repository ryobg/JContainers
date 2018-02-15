@echo off
setlocal

set BOOST_ROOT="%~dp0\..\dep\boost"

REM Zero the error level
ver > nul

if not exist "%BOOST_ROOT%\bootstrap.bat" (

    for /f %%p in ('where python3') do set PYTHON_EXE=%%p
    if not defined PYTHON_EXE (
        for /f %%p in ('where python') do set PYTHON_EXE=%%p
        if not defined PYTHON_EXE (
            echo No Python 3 accessible?
            exit /b 1
        )
    )

    echo Download Boost...
    REM https://sourceforge.net/projects/boost/files/boost/1.66.0/
    REM https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.7z
    %PYTHON_EXE% "%~dp0\download.py" ^
        https://dl.bintray.com/boostorg/release/1.66.0/source/boost_1_66_0.7z ^
        "%~dp0\boost.7z"
    if errorlevel 1 (
        echo Unable to download Boost, exiting...
        exit /b 1
    )

    echo Rename Boost...
    "%~dp0\7za.exe" rn "%~dp0\boost.7z" boost_1_66_0 boost
    if errorlevel 1 (
        echo Unable to rename Boost, exiting...
        exit /b 1
    )

    echo Unpack Boost...
    "%~dp0\7za.exe" -aoa -spe x "%~dp0\boost.7z" -o"%BOOST_ROOT%"
    if errorlevel 1 (
        echo Unable to unpack Boost, exiting...
        exit /b 1
    )
)

pushd "%BOOST_ROOT%"

if not exist b2.exe ( 
    echo b2 not found, bootstrapping Boost...
    call bootstrap.bat
    if errorlevel 1 (
        echo Unable to bootstrap, exiting...
        popd
        exit /b 2
    )
)

b2 variant=release,debug link=static threading=multi runtime-link=static address-model=64 ^
 --with-serialization --with-date_time --with-filesystem --with-thread ^
 --build-dir=garbage --stagedir=boost_lib stage

if errorlevel 1 (
    echo Boost build failed, exiting...
    popd
    exit /b 3
)
    
REM RMDIR /Q /S garbage

popd

