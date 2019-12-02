@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2017\Enterprise\VC\Auxiliary\Build\vcvarsall.bat" x64     
set compilerFlags=/MTd /Od /Oi /EHsc /fp:fast /nologo /GR- /WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /FC /Z7
set linkerFlags=/OUT:scratcher.exe /INCREMENTAL:NO /OPT:REF user32.lib gdi32.lib winmm.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build
del *.pdb > NUL 2> NUL

cl.exe %compilerFlags% ../source/*.cpp /link %linkerFlags%
popd