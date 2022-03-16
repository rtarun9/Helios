#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	struct alignas(16) GenerateMipsBuffer
	{
		uint32_t sourceMipLevel{};
		uint32_t numberMipLevels{};
		uint32_t sourceDimensionType{};
		uint32_t isSRGB{};
		DirectX::XMFLOAT2 texelSize{};
	};

	enum class MipRootParameters
	{
		GenerateMipsCBuffer,
		SourceMip,
		OutputMip,
		TotalRootParamters
	};

	class GenerateMipsPSO
	{
	public:
		void Init(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE uavHandle);

	private:
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PSO;
		Microsoft::WRL::ComPtr<ID3D12RootSignature> m_RootSignature;
	};
}
