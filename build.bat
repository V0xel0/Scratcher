@echo off

where /q cl || (
  echo ERROR: could not find "cl" - run the "build.bat" from the MSVC x64 native tools command prompt.
  exit /b 1
)

set buildDefines=/DUNITY_BUILD=1 /DGAME_ASSERTIONS=1 /DGAME_INTERNAL=1
set warnings=/WX /W4 /wd4201 /wd4100 /wd4189 /wd4505 /wd4701
set compilerFlags=/std:c++20 /MT /MP /Od /arch:AVX2 /Oi /Ob3 /EHsc /fp:fast /fp:except- /nologo /GS- /Gs999999 /GR- /FC /Z7 /openmp %buildDefines% %warnings%
set linkerFlags=/OUT:scratcher.exe /INCREMENTAL:NO /OPT:REF /CGTHREADS:6 /STACK:0x100000,0x100000 user32.lib gdi32.lib winmm.lib

IF NOT EXIST .\build mkdir .\build
pushd .\build
del *.pdb > NUL 2> NUL

REM Hot-Reload build
echo WAITING FOR PDB > lock.tmp
cl.exe %compilerFlags% ../source/Game.cpp /LD /link /EXPORT:gameFullUpdate
del lock.tmp
cl.exe %compilerFlags% ../source/Win32EntryPoint.cpp /link %linkerFlags%

REM non-unity build
REM cl.exe %compilerFlags% ../source/*.cpp /link %linkerFlags% && .\scratcher.exe
popd