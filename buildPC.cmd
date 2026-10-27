@echo off
rem Build NovaRemote on Windows. Closes a running NovaRemote.exe first, because
rem the linker cannot overwrite an exe that is in use.
rem
rem   buildPC              Release build
rem   buildPC run          Release build, then start it
rem   buildPC debug run    Debug build (needs the full wx install, see README), then start it
setlocal
cd /d "%~dp0"

set CONFIG=Release
set RUN=0
for %%a in (%*) do (
    if /i "%%a"=="debug" set CONFIG=Debug
    if /i "%%a"=="run"   set RUN=1
)

taskkill /f /im NovaRemote.exe >nul 2>&1

set MSBUILD=
for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"
if not defined MSBUILD (
    echo MSBuild not found. Install Visual Studio 2022 with the C++ desktop workload.
    exit /b 1
)

"%MSBUILD%" wxui\NovaRemote.sln /p:Configuration=%CONFIG% /p:Platform=Win32 /m /nr:false /v:m
if errorlevel 1 (
    echo.
    echo BUILD FAILED
    exit /b 1
)

echo.
echo Built wxui\bin\%CONFIG%\NovaRemote.exe
if %RUN%==1 start "" "wxui\bin\%CONFIG%\NovaRemote.exe"
exit /b 0
