#include "Pch.hpp"

#include "StructuredBuffer.hpp"

namespace helios::gfx
{
	ID3D12Resource* StructuredBuffer::GetResource() const
	{
		return m_DestinationResource.Get();
	}

	uint32_t StructuredBuffer::GetSRVIndex() const
	{
		return m_SRVIndexInDescriptorHeap;
	}
}
