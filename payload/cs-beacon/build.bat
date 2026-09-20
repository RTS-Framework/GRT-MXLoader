@echo off

echo ========== initialize Visual Studio environment ==========
if "%VisualStudio%" == "" (
    echo environment variable "VisualStudio" is not set
    exit /b 1
)
call "%VisualStudio%\VC\Auxiliary\Build\vcvars64.bat"

echo ================== generate fake stage ===================
MSBuild.exe ..\..\GRT-MXLoader.sln /t:payload\cs-beacon /p:Configuration=Release /p:Platform=x86
MSBuild.exe ..\..\GRT-MXLoader.sln /t:payload\cs-beacon /p:Configuration=Release /p:Platform=x64

echo ==================== copy fake stage =====================
copy /Y ..\..\Release\cs-beacon.dll     ..\..\loader\cs-beacon\testdata\stage_x86.dat
copy /Y ..\..\x64\Release\cs-beacon.dll ..\..\loader\cs-beacon\testdata\stage_x64.dat

echo =================== clean build files ====================
rd /S /Q "Release"
rd /S /Q "x64"
rd /S /Q "..\..\Release"
rd /S /Q "..\..\x64"

echo ==========================================================
echo              build fake beacon stage finish!
echo ==========================================================
pause
