@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64
   
set compilerFlags=/std:c++17 /MTd /MP /Od /Oi /EHsc /fp:fast /nologo /GR- /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /FC /Z7 /openmp /DUNITY_BUILD=1
set linkerFlags=/OUT:scratcher.exe /INCREMENTAL:NO /OPT:REF /CGTHREADS:6 user32.lib gdi32.lib winmm.lib ole32.lib 

IF NOT EXIST .\build mkdir .\build
pushd .\build
del *.pdb > NUL 2> NUL

REM Unity build
cl.exe %compilerFlags% ../source/Win32EntryPoint.cpp /link %linkerFlags% && .\scratcher.exe

REM non-unity build
REM cl.exe %compilerFlags% ../source/*.cpp /link %linkerFlags% && .\scratcher.exe

popd