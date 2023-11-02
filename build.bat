@echo off

call cl_setup_generic.bat x64

set build_dir=bin
if not exist "%build_dir%" mkdir "%build_dir%"

set exe_name=gl_win32_minimal.exe
set libs=user32.lib gdi32.lib shell32.lib ole32.lib Winmm.lib opengl32.lib

set compilation_command=cl ..\main.c /nologo /Od /FC /Zi /Fd /Fm /Fo /Wall /link /out:%exe_name% %libs%
echo %compilation_command%

pushd %build_dir%
%compilation_command%
popd