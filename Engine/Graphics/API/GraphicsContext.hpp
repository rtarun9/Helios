#pragma once

#include "Pch.hpp"

#include "RenderTarget.hpp"
#include "PipelineState.hpp"
#include "DepthStencilBuffer.hpp"
#include "Resources.hpp"
#include "Descriptor.hpp"

namespace helios::gfx
{
	class Device;

	// Wrapper class for Graphics CommandList, which provides a set of easy and simple functions to record commands for execution by GPU.
	// The command queue will contain a queue of command list, which can be passed into the GraphicsContext's constructor to create a GraphicsContext object.
	// note (rtarun9) : This design is subject to change.
	class GraphicsContext
	{
	public:
		GraphicsContext(Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList);
		ID3D12GraphicsCommandList* const GetCommandList() { return mCommandList.Get(); }

		// Core functionalities.
		void ResourceBarrier(ID3D12Resource* const resource, D3D12_RESOURCE_STATES previousState, D3D12_RESOURCE_STATES newState);
		
		void ClearRenderTargetView(BackBuffer* const backBuffer, const DirectX::SimpleMath::Color& color);
		void SetDescriptorHeaps(Descriptor* const descriptor) const;

		void SetGraphicsPipelineState(PipelineState* pipelineState) const;
		void SetComputePipelineState(PipelineState* pipelineState) const;
		void SetGraphicsRootSignature(PipelineState* pipelineState) const;
		void SetComputeRootSignature(PipelineState* pipelineState) const;
		void SetPipelineStateObject(PipelineState* pipelineState) const;

		void SetIndexBuffer(Buffer* const buffer);
		void Set32BitGraphicsConstants(void* renderResources);
		void Set32BitComputeConstants(void* renderResources);
		void SetDefaultViewportAndScissor(uint32_t width, uint32_t height) const;
		void SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY primitiveTopology);
		void SetRenderTarget(uint32_t rtvCount, BackBuffer* renderTarget);

		void DrawInstanceIndexed(uint32_t indicesCount);

	private:
		static constexpr uint32_t NUMBER_32_BIT_CONSTANTS = 64;

		Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> mCommandList{};
	};
}


