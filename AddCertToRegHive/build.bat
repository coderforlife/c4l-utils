@echo off

:: This builds using MinGW 32 and 64 bit
:: You may have to update the locations of things before
:: This assume that 32-bit MinGW is in the path


echo Compiling 32-bit...

gcc -mconsole AddCertToRegHive.c -o AddCertToRegHive.exe -lCrypt32 -O3 -s -D UNICODE -D _UNICODE



echo.
echo Compiling 64-bit...

set P64=C:\Program Files\MinGW\mingw64
set M64=%P64%\x86_64-w64-mingw32

set PATH=%P64%\libexec\gcc\x86_64-w64-mingw32\4.4.4;%PATH%
set PATH=%M64%\bin;%PATH%

set CPATH=%M64%\include;%P64%\include
set CPATH=%M64%\include\c++\4.4.4;%CPATH%
set CPATH=%M64%\include\c++\4.4.4\x86_64-w64-mingw32;%CPATH%
set CPATH=%P64%\lib\gcc\x86_64-w64-mingw32\4.4.4\include;%CPATH%

copy /Y "%P64%\x86_64-w64-mingw32\lib\crt2.o" . >nul
copy /Y "%P64%\x86_64-w64-mingw32\lib\crtbegin.o" . >nul
copy /Y "%P64%\x86_64-w64-mingw32\lib\crtend.o" . >nul



gcc -mconsole AddCertToRegHive.c -o AddCertToRegHive64.exe -lCrypt32 -O3 -s -D UNICODE -D _UNICODE ^
	-L "%M64%\lib64" -L "%M64%\lib" -L "%P64%\lib" -L "%P64%\lib\gcc\x86_64-w64-mingw32\4.4.4"

del crt2.o
del crtbegin.o
del crtend.o

pause
