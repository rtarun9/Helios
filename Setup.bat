@echo off
echo Enter the build directory (the DirectXAgilitySDK and ShaderCompiler will be setup here).
echo The build directory should math 'Cmake --build BuildDirectory'
set /p BuildDirectory= Enter Build Directory Path (Relative to project root directory) : 
echo Build Directory is: %BuildDirectory%

powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.711.3-preview -OutFile agility.zip"
powershell -Command "& {Expand-Archive agility.zip External/DirectXAgilitySDK}"

xcopy External\DirectXAgilitySDK\build\native\bin\x64\* %BuildDirectory%\Bin\Debug\D3D12\
xcopy External\DirectXAgilitySDK\build\native\bin\x64\* %BuildDirectory%\Bin\Release\D3D12\

xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Debug\Bin\Debug\D3D12\
xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Release\Bin\Release\D3D12\

powershell -Command "Invoke-WebRequest -Uri https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212/dxc_2022_12_16.zip -OutFile dxc.zip"
powershell -Command "& {Expand-Archive dxc.zip External/DirectXShaderCompiler}"

xcopy External\DirectXShaderCompiler\bin\x64\* %BuildDirectory%\Bin\Debug\
xcopy External\DirectXShaderCompiler\bin\x64\* %BuildDirectory%\Bin\Release\

xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Debug\Bin\Debug\
xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Release\Bin\Release\



xcopy External\DirectXShaderCompiler\lib\x64\* %BuildDirectory%\Bin\Debug\
xcopy External\DirectXShaderCompiler\lib\x64\* %BuildDirectory%\Bin\Release\

xcopy External\DirectXShaderCompiler\lib\x64\* out\build\x64-Debug\Bin\Debug\
xcopy External\DirectXShaderCompiler\lib\x64\* out\build\x64-Release\Bin\Release\
