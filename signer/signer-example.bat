:: Set this to your locale
@set locale=en-US

:: Set this to the name of the certificate to use (is generated if necessary)
@set name=My Name

:: Set this this to the root of the installation (e.g. C:\ or the mounted boot.wim / install.wim folder)
:: %1 means the first argument to this script
@set root=%1


::::::::::::::::::::::::::::::::::::::::::::::
:: Don't need to mess with the rest of this
@echo off

:: Remove " from the path
for /f "useback tokens=*" %%a in ('%root%') do set root=%%~a

set win=%root%\Windows
set sys=%win%\System32
set sysl=%sys%\%locale%

signer /nologo /sign "%name%" "%sys%\bootres.dll"
signer /nologo /sign "%name%" "%sys%\winload.exe"
signer /nologo /sign "%name%" "%sysl%\winload.exe.mui"
signer /nologo /sign "%name%" "%sys%\winresume.exe"
signer /nologo /sign "%name%" "%sysl%\winresume.exe.mui"

:: Cannot be signed this way and I don't think it matters
:: signer /nologo /sign "%name%" "%win%\Boot\PCAT\bootmgr"

signer /nologo /install "%name%" "%sys%\config\SOFTWARE"
