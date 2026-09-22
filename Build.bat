@echo off

echo ========== initialize Visual Studio environment ==========
if "%VisualStudio%" == "" (
    echo environment variable "VisualStudio" is not set
    exit /b 1
)
call "%VisualStudio%\VC\Auxiliary\Build\vcvars64.bat"

echo ================== clean outdated dist ===================
del /S /Q dist

echo =============== clean outdated build files ===============
rd /S /Q "Release"
rd /S /Q "x64"
rd /S /Q "builder\cs-beacon\Release"
rd /S /Q "builder\cs-beacon\x64"
rd /S /Q "builder\dotnet\Release"
rd /S /Q "builder\dotnet\x64"

echo ==================== generate builder ====================
MSBuild.exe GRT-MXLoader.sln /t:builder\cs-beacon /p:Configuration=Release /p:Platform=x86
MSBuild.exe GRT-MXLoader.sln /t:builder\cs-beacon /p:Configuration=Release /p:Platform=x64
MSBuild.exe GRT-MXLoader.sln /t:builder\dotnet    /p:Configuration=Release /p:Platform=x86
MSBuild.exe GRT-MXLoader.sln /t:builder\dotnet    /p:Configuration=Release /p:Platform=x64

echo ================ extract loader template =================
cd builder\cs-beacon
echo --------extract cs-beacon template for x86--------
"..\..\Release\cs-beacon.exe"
echo --------extract cs-beacon template for x64--------
"..\..\x64\Release\cs-beacon.exe"
cd ..\..

cd builder\dotnet
echo --------extract dotnet template for x86--------
"..\..\Release\dotnet.exe"
echo --------extract dotnet template for x64--------
"..\..\x64\Release\dotnet.exe"
cd ..\..

echo ================= copy standard template =================
copy /Y dist\standard\CSBeacon_*.bin loader\cs-beacon\template
copy /Y dist\standard\DotNET_*.bin   loader\dotnet\template

echo =================== clean build files ====================
rd /S /Q "Release"
rd /S /Q "x64"
rd /S /Q "builder\cs-beacon\Release"
rd /S /Q "builder\cs-beacon\x64"
rd /S /Q "builder\dotnet\Release"
rd /S /Q "builder\dotnet\x64"

echo =================== test loader package ==================
call test.bat
if errorlevel 1 (
    echo.
    echo failed to test loader package!
    pause
    exit /b %ERRORLEVEL%
)

echo ==========================================================
echo                 build template finish!
echo ==========================================================
pause
