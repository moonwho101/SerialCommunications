echo off
echo Assembling SERASM.ASM
masm /mx serasm;
echo Compling SIMPLE.C, SERIAL.C, PROTOCOL.C, and ANSI.C
cl /AL /D__MSC__ /Od /Gs /c simple.c serial.c protocol.c ansi.c
echo Creating MSERIAL.LIB
lib mserial.lib +serasm +serial +protocol +ansi;
echo Creating SIMPLE.EXE
link /NOI /INCR simple,,,mserial.lib graphics.lib;

