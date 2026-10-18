@echo off
setlocal
cd /d "%~dp0"
echo Building HYPNO GROOVE VST3 for Windows x64...
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
if errorlevel 1 goto failed
cmake --build build --config Release --parallel 4
if errorlevel 1 goto failed
echo.
echo Build complete! Find HYPNO GROOVE.vst3 under build\HypnoGroove_artefacts\Release\VST3\
echo Copy its .vst3 folder into C:\Program Files\Common Files\VST3\, then rescan FL Studio.
pause
exit /b 0
:failed
echo.
echo BUILD FAILED. Check that Visual Studio 2022 C++ tools, CMake and Git are installed.
pause
exit /b 1
