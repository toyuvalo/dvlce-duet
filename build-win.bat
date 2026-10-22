@echo off
REM Build Dvlce Duet on Windows: Visual Studio 2022 (or Build Tools) + CMake + Ninja.
REM   build-win.bat            core + tests + plugin (VST3, Standalone; AAX when AAX_SDK is set)
REM   build-win.bat core       core + tests only (no JUCE)
REM Environment (all optional): CMAKE, NINJA, BUILD_DIR, JUCE_DIR (a local JUCE 8.0.13 checkout), AAX_SDK.
setlocal
if not defined VSVARS set VSVARS=C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvarsall.bat
if not exist "%VSVARS%" set VSVARS=C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat
call "%VSVARS%" x64 >nul
if errorlevel 1 (
    echo vcvarsall failed: set VSVARS to your vcvarsall.bat
    exit /b 1
)
cd /d "%~dp0"
if not defined CMAKE set CMAKE=cmake
if not defined NINJA set NINJA=ninja
if not defined BUILD_DIR set BUILD_DIR=%~dp0build
set PLUGIN=ON
if /i "%1"=="core" (
    set BUILD_DIR=%BUILD_DIR%-core
    set PLUGIN=OFF
)
set EXTRA=
if defined JUCE_DIR set EXTRA=-DFETCHCONTENT_SOURCE_DIR_JUCE=%JUCE_DIR%
if defined AAX_SDK set EXTRA=%EXTRA% -DDVLCE_DUET_AAX_SDK=%AAX_SDK%
if not exist "%BUILD_DIR%\build.ninja" (
    "%CMAKE%" -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=Release ^
        -DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl -DCMAKE_MAKE_PROGRAM=%NINJA% ^
        -DDVLCE_DUET_BUILD_PLUGIN=%PLUGIN% %EXTRA%
    if errorlevel 1 exit /b 1
)
"%CMAKE%" --build "%BUILD_DIR%" -j 2
if errorlevel 1 exit /b 1
"%BUILD_DIR%\dvlce_duet_core_tests.exe"
exit /b %errorlevel%
