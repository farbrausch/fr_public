@echo off
set OPTS=/M2 /nologo

setlocal enableextensions

pushd %~dp0
md bin 2>nul
set PATH=%PATH%;%~dp0\bin

for %%t in (asc makeproject wz4ops) do (
  vcbuild %OPTS% tools\%%t\%%t.sln "stripped_blank_shell|Win32"
  if errorlevel 1 goto err
  copy /y tools\%%t\stripped_blank_shell_Win32\%%t.exe bin > nul
)

echo.
echo All OK!
echo.
goto end

:err
echo.
echo An error occured!
echo.

:end
popd