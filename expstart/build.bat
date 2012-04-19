@echo off

:: This builds using MinGW-w64 for 32 and 64 bit (http://mingw-w64.sourceforge.net/)
:: Make sure both mingw-w32\bin and mingw-w64\bin are in the PATH

set FLAGS=-mwindows -Wall -static-libgcc -O3 -s -D UNICODE -D _UNICODE
set FILES=expstart.c GetBaseAddress.c functions.c Memory.c

echo Compiling 32-bit...
i686-w64-mingw32-windres resources.rc resources.o
i686-w64-mingw32-gcc %FLAGS% -o expstart.exe %FILES% resources.o
echo.

echo Compiling 64-bit...
x86_64-w64-mingw32-windres.exe resources.rc resources64.o
x86_64-w64-mingw32-gcc %FLAGS% -o expstart64.exe %FILES% resources64.o
pause
