@echo off

echo ========== initialize Visual Studio environment ==========
if "%VisualStudio%" == "" (
    echo environment variable "VisualStudio" is not set
    exit /b 1
)
call "%VisualStudio%\VC\Auxiliary\Build\vcvars64.bat"

echo ================== generate dotnet exe ===================
MSBuild.exe ..\..\GRT-MXLoader.sln /t:payload\dotnet_exe /p:Configuration=Release /p:Platform="Any CPU"

echo ==================== copy dotnet exe =====================
copy /Y bin\Release\dotnet_exe.exe ..\..\loader\dotnet\testdata\dotnet_exe.dat

echo =================== clean build files ====================
rd /S /Q "bin"
rd /S /Q "obj"

echo ==========================================================
echo                  build dotnet exe finish!
echo ==========================================================
pause
