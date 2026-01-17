echo off
echo Assembling SERASM.ASM
masm /mx serasm;
echo Compling SIMPLE.C, SERIAL.C, PROTOCOL.C, and ANSI.C
qcl /AL /D__MSC__ /Od /Gs /c simple.c serial.c protocol.c ansi.c
echo Creating QSERIAL.LIB
lib qserial.lib +serasm +serial +protocol +ansi;
echo Creating SIMPLE.EXE
qlink /NOI /INCR simple,,,qserial.lib;

