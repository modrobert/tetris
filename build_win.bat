@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

SET CompilerFlags=-GR- -EHsc -DAUDIO -O2 -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 /std:c++latest /nologo

SET LinkerFlags=-opt:ref SDL2main.lib SDL2.lib SDL2_ttf.lib /LIBPATH:C:\sdl\SDL2-2.0.12\lib\x64 /LIBPATH:C:\sdl\SDL2_ttf-2.0.15\lib\x64 /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\lib\10.0.10240.0\ucrt\x64" /SUBSYSTEM:windows /ENTRY:mainCRTStartup

SET IncludeDirectories=/I "C:\sdl\SDL2-2.0.12\include" /I "C:\sdl\SDL2_ttf-2.0.15\include"

cl %CompilerFlags% %IncludeDirectories% tetris.cc audio.cc /link %LinkerFlags%

