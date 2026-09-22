@echo off

echo ========== initialize Visual Studio environment ==========
if "%VisualStudio%" == "" (
    echo environment variable "VisualStudio" is not set
    exit /b 1
)
call "%VisualStudio%\VC\Auxiliary\Build\vcvars64.bat"

echo ================== generate dotnet dll ===================
MSBuild.exe ..\..\GRT-MXLoader.sln /t:payload\dotnet_dll /p:Configuration=Release /p:Platform="Any CPU"

echo ==================== copy dotnet dll =====================
copy /Y bin\Release\dotnet_dll.dll ..\..\loader\dotnet\testdata\dotnet_dll.dat

echo =================== clean build files ====================
rd /S /Q "bin"
rd /S /Q "obj"

echo ==========================================================
echo                  build dotnet dll finish!
echo ==========================================================
pause
