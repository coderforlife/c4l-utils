@echo off

:: This builds using MinGW-w64 for 32 and 64 bit (http://mingw-w64.sourceforge.net/)
:: Make sure both mingw-w32\bin and mingw-w64\bin are in the PATH

set FLAGS=-mconsole -municode -static-libgcc -O3 -s -D UNICODE -D _UNICODE

echo Compiling 32-bit...
i686-w64-mingw32-gcc %FLAGS% -o security.exe security.c
echo.

echo Compiling 64-bit...
x86_64-w64-mingw32-gcc %FLAGS% -o security64.exe security.c
pause
