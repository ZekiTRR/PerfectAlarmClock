@echo off
setlocal
call "E:\Visual Studio\Product\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
cd /d "%~dp0"
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=E:\Qt\6.12.0\msvc2022_64 -DCMAKE_MAKE_PROGRAM=E:\Qt\Tools\Ninja\ninja.exe
if errorlevel 1 exit /b 1
cmake --build build --parallel
