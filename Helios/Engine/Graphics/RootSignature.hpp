#pragma once

#include "Pch.hpp"

namespace helios::gfx
{
	enum class ParameterType : uint32_t
	{
		ShaderResourceView,
		ConstantBuffer,
		RootConstant,
		DescriptorTable
	};

	class RootSignature
	{
	public:
		static CD3DX12_ROOT_PARAMETER1 CreateRootParameter(ParameterType& paramterType, uint32_t registerBase, D3D12_SHADER_VISIBILITY shaderVisibility);
	private:
	};
}


