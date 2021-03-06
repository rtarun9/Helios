#pragma once

#include "Pch.hpp"

#include "Descriptor.hpp"

namespace helios::gfx
{
	// Constant buffer abstraction : requires the data to be 256 byte aligned. The buffer is mapped once and unmapped at the end.
	template <typename T>
	class ConstantBuffer
	{
	public:
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, const T& data, Descriptor& cbDescriptor, std::wstring_view cbufferName)
		{
			CD3DX12_HEAP_PROPERTIES uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
			CD3DX12_RESOURCE_DESC cbufferUploadDesc = CD3DX12_RESOURCE_DESC::Buffer(sizeof(T));

			ThrowIfFailed(device->CreateCommittedResource(&uploadHeapProperties, D3D12_HEAP_FLAG_NONE, &cbufferUploadDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_Buffer)));
			m_Buffer->SetName(cbufferName.data());

			m_BufferView =
			{
				.BufferLocation = m_Buffer->GetGPUVirtualAddress(),
				.SizeInBytes = sizeof(T)
			};

			device->CreateConstantBufferView(&m_BufferView, cbDescriptor.GetCurrentDescriptorHandle().cpuDescriptorHandle);
			m_BufferIndexInDescriptorHeap = cbDescriptor.GetCurrentDescriptorIndex();

			cbDescriptor.OffsetCurrentHandle();
		
			CD3DX12_RANGE readRange(0, 0);       
			ThrowIfFailed(m_Buffer->Map(0, &readRange, reinterpret_cast<void**>(&m_Data)));
			void* dataPtr = (void*)(&data);
			memcpy(m_Data, dataPtr, sizeof(T));

			m_BufferData = data;
		}

		ID3D12Resource* const GetResource() const
		{
			return m_Buffer.Get();
		}

		D3D12_CONSTANT_BUFFER_VIEW_DESC GetBufferView() const
		{
			return m_BufferView;
		}

		uint32_t GetBufferIndex() const
		{
			return m_BufferIndexInDescriptorHeap;
		}

		void Update() const
		{
			memcpy(m_Data, &m_BufferData, sizeof(T));
		}

		T& GetBufferData()
		{
			return m_BufferData;
		}

	// Purpose for void* data : The buffer data is mapped once and left bound for the entire application. 
	// Buffer is mapped during the Init function and left mapped.
	private:
		Microsoft::WRL::ComPtr<ID3D12Resource> m_Buffer;
		D3D12_CONSTANT_BUFFER_VIEW_DESC m_BufferView{};
 
		uint32_t m_BufferIndexInDescriptorHeap{};

		T m_BufferData{};

		void* m_Data{ nullptr };
	};
}