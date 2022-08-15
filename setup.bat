powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.602.0 -OutFile agility.zip"
powershell -Command "& {Expand-Archive agility.zip ThirdParty/DirectXAgilitySDK}"

xcopy ThirdParty\DirectXAgilitySDK\build\native\bin\x64\* Bin\Debug\D3D12\
xcopy ThirdParty\DirectXAgilitySDK\build\native\bin\x64\* Bin\Release\D3D12\