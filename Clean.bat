rd  /S /Q ".vs\GRT-MXLoader\v17\ipch"
del /S /Q ".vs\GRT-MXLoader\v17\Browse.VC.db"
del /S /Q ".vs\GRT-MXLoader\v17\Solution.VC.db"

rd /S /Q "Debug"
rd /S /Q "Release"
rd /S /Q "x64"
rd /S /Q "x86"

rd /S /Q "builder\cs-beacon\Debug"
rd /S /Q "builder\cs-beacon\Release"
rd /S /Q "builder\cs-beacon\x64"
rd /S /Q "builder\cs-beacon\x86"

rd /S /Q "builder\dotnet\Debug"
rd /S /Q "builder\dotnet\Release"
rd /S /Q "builder\dotnet\x64"
rd /S /Q "builder\dotnet\x86"

rd /S /Q "tool\test_stage\Debug"
rd /S /Q "tool\test_stage\Release"
rd /S /Q "tool\test_stage\x64"
rd /S /Q "tool\test_stage\x86"
