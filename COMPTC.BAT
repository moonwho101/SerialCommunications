echo off
echo Assembling SERASM.ASM
tasm /mx serasm;
echo Compling SERIAL.C, PROTOCOL.C, ANSI.C, and ANSI.CPP
tcc -ml -c serial.c protocol.c ansi.c ansicpp.cpp
echo Creating TSERIAL.LIB
tlib tserial.lib +serasm +serial +protocol +ansi +ansicpp
echo Creating SIMPLE.EXE
tcc -ml simple.c tserial.lib
echo Creating SIMCPP.EXE
tcc -ml simcpp.cpp tserial.lib

