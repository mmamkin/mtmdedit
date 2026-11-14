@echo off
set "DEST=%APPDATA%\Autodesk\ApplicationPlugins\MtmdEdit.bundle"
if exist "%DEST%" rmdir /s /q "%DEST%"
echo Removed %DEST%
