#include "Pch.hpp"

#include "StructuredBuffer.hpp"

namespace helios::gfx
{
	ID3D12Resource* StructuredBuffer::GetResource()
	{
		return m_DestinationResource.Get();
	}
}
