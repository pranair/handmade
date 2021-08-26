@echo off
mkdir build
pushd build
set CurrentTime=%time:~0,2%%time:~3,2%%time:~6,2%%time:~9,6%
rem echo %CurrentTime%

set CompilerFlags=-MT -nologo -Gm- -GR- -EHa- -Od -Oi -WX -W4 -wd4100 -wd4189 -wd4201 -wd4505 -DHANDMADE_SLOW=1 -DHANDMADE_INTERNAL=1 -FC -Zi
set LinkerFlags=-opt:ref user32.lib gdi32.lib Ole32.lib winmm.lib Avrt.lib
del handmade_*.pdb

cl  %CompilerFlags% ..\code\handmade.cpp /LD /link /PDB:handmade_%CurrentTime%.pdb /EXPORT:GameUpdateAndRender
cl %CompilerFlags% ..\code\win32_handmade.cpp /link %LinkerFlags%
popd

