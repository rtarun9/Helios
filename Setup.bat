powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.602.0 -OutFile agility.zip"
powershell -Command "& {Expand-Archive agility.zip External/DirectXAgilitySDK}"

xcopy External\DirectXAgilitySDK\build\native\bin\x64\* Build\Bin\Debug\D3D12\
xcopy External\DirectXAgilitySDK\build\native\bin\x64\* Build\Bin\Release\D3D12\

xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Debug\Bin\Debug\D3D12\
xcopy External\DirectXAgilitySDK\build\native\bin\x64\* out\build\x64-Release\Bin\Release\D3D12\

powershell -Command "Invoke-WebRequest -Uri https://github.com/microsoft/DirectXShaderCompiler/releases/download/v1.7.2212/dxc_2022_12_16.zip -OutFile dxc.zip"
powershell -Command "& {Expand-Archive dxc.zip External/DirectXShaderCompiler}"

xcopy External\DirectXShaderCompiler\bin\x64\* Build\Bin\Debug\
xcopy External\DirectXShaderCompiler\bin\x64\* Build\Bin\Release\

xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Debug\Bin\Debug\
xcopy External\DirectXShaderCompiler\bin\x64\* out\build\x64-Release\Bin\Release\



xcopy External\DirectXShaderCompiler\lib\x64\* Build\Bin\Debug\
xcopy External\DirectXShaderCompiler\lib\x64\* Build\Bin\Release\

xcopy External\DirectXShaderCompiler\lib\x64\* out\build\x64-Debug\Bin\Debug\
xcopy External\DirectXShaderCompiler\lib\x64\* out\build\x64-Release\Bin\Release\
