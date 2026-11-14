@echo off
rem Builds MtmdEdit2027.arx and assembles Out\MtmdEdit.bundle.
rem Needs Visual Studio 2026 (or 2022 17.14) with "Desktop development with C++"
rem incl. MFC, and the ObjectARX 2027 SDK pointed to by OBJECTARX2027.
setlocal
cd /d "%~dp0"

if "%OBJECTARX2027%"=="" if not exist "C:\ObjectARX 2027\inc\arxHeaders.h" (
  echo Set OBJECTARX2027 to the ObjectARX 2027 SDK folder, e.g.:
  echo   set "OBJECTARX2027=C:\ObjectARX 2027"
  exit /b 1
)

set "VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe"
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.ATLMFC -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
if not defined MSBUILD (
  echo MSBuild with the C++ MFC component was not found.
  exit /b 1
)

"%MSBUILD%" MtmdEdit.vcxproj /m /nologo /p:Configuration=Release /p:Platform=x64 || exit /b 1

set "BUNDLE=Out\MtmdEdit.bundle"
if exist "%BUNDLE%" rmdir /s /q "%BUNDLE%"
mkdir "%BUNDLE%\Contents"
copy /y bundle\PackageContents.xml "%BUNDLE%\" >nul || exit /b 1
copy /y Out\MtmdEdit2027.arx "%BUNDLE%\Contents\" >nul || exit /b 1
echo.
echo Built %CD%\%BUNDLE%
