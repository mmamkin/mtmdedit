@echo off
rem Installs the bundle built by build.cmd for the current Windows user.
rem AutoCAD 2027 loads it on its next start; the command is TE.
setlocal
cd /d "%~dp0"
set "SRC=Out\MtmdEdit.bundle"
if not exist "%SRC%\Contents\MtmdEdit2027.arx" (
  echo Run build.cmd first.
  exit /b 1
)
set "DEST=%APPDATA%\Autodesk\ApplicationPlugins\MtmdEdit.bundle"
if exist "%DEST%" rmdir /s /q "%DEST%"
xcopy /e /i /y "%SRC%" "%DEST%" >nul || exit /b 1
echo Installed to %DEST%
echo Restart AutoCAD 2027 and run the TE command.
