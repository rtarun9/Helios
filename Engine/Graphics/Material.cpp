#include "Pch.hpp"

#include "Material.hpp"

namespace helios::gfx
{
	void Material::CreateBindlessRootSignature(ID3D12Device* device, std::wstring_view shaderPath) 
	{
		wrl::ComPtr<ID3DBlob> shaderBlob;
		::D3DReadFileToBlob(shaderPath.data(), &shaderBlob);

		if (!shaderBlob.Get())
		{
			ErrorMessage(shaderPath.data() + std::wstring(L" Not found"));
		}

		ThrowIfFailed(device->CreateRootSignature(0u, shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
		auto rootSignatureName = std::wstring(L"Bindless Root Signature");
		rootSignature->SetName(rootSignatureName.c_str());
	}
}
