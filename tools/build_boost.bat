@echo off
setlocal

set BOOST_ROOT=%~dp0\..\dep\boost

REM Zero the error level
ver > nul

if not exist "%BOOST_ROOT%\bootstrap.bat" (

    if not exist "%~dp0\boost.7z" (

        echo Download Boost...
        python "%~dp0\download.py" ^
            https://archives.boost.io/release/1.67.0/source/boost_1_67_0.7z ^
            "%~dp0\boost.7z"
        if errorlevel 1 (
            echo Unable to download Boost, exiting...
            exit /b 1
        )

        echo Rename Boost folder...
        "%~dp0\7za.exe" rn "%~dp0\boost.7z"  boost_1_67_0 boost
        if errorlevel 1 (
            echo Unable to rename Boost, exiting...
            exit /b 1
        )
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
    call bootstrap.bat %1
    if errorlevel 1 (
        echo Unable to bootstrap, exiting...
        popd
        exit /b 2
    )
)

b2 architecture=x86 address-model=64 variant=release,debug ^
  link=static runtime-link=static threading=multi ^
 --with-serialization --with-date_time --with-filesystem --with-thread ^
 --build-dir=garbage --stagedir=boost_lib stage

if errorlevel 1 (
    echo Boost build failed, exiting...
    popd
    exit /b 3
)
    
REM RMDIR /Q /S garbage

popd

