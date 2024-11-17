@echo off
mkdir build
copy ".\external\lib\*.dll"  ".\build\"
rem set enviroment vars and requred stuff for the msvc compiler
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

rem Set paths for include directories, libraries, and output directories
set INCLUDE_DIRS=/I..\inc\ /I..\external\inc\ /I..\external\inc\IMGUI\ /I..\external\inc\GLFW\ /I..\external\inc\GL\
set LIBRARY_DIRS=/LIBPATH:..\external\lib\
set LIBRARIES=opengl32.lib glfw3.lib glew32.lib user32.lib gdi32.lib shell32.lib kernel32.lib Comdlg32.lib

pushd .\build
cl /Zi /EHsc /O2 /D_M_AMD64 /W4 /MD /nologo %INCLUDE_DIRS% ..\Main.cpp ..\external\src\*.cpp ..\src\*.cpp /link %LIBRARY_DIRS% %LIBRARIES%
.\Main.exe
popd
