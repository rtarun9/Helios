powershell -Command "Invoke-WebRequest -Uri https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/1.602.0 -OutFile agility.zip"
powershell -Command "& {Expand-Archive agility.zip Engine/ThirdParty/DirectXAgilitySDK}"

xcopy Engine\ThirdParty\DirectXAgilitySDK\build\native\bin\x64\* Build\Debug-Bin\D3D12\
xcopy Engine\ThirdParty\DirectXAgilitySDK\build\native\bin\x64\* Build\Release-Bin\D3D12\