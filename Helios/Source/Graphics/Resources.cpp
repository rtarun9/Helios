#include "Graphics/Resources.hpp"
#include "Graphics/GraphicsDevice.hpp"

namespace helios::gfx
{
    Allocation::Allocation(const Allocation& other) : resource(other.resource), allocation(other.allocation)
    {
        if (other.mappedPointer.has_value())
        {
            mappedPointer = other.mappedPointer;
        }
    }

    Allocation& Allocation::operator=(const Allocation& other)
    {
        if (&other == this)
        {
            return *this;
        }

        // Clear state of current object.
        resource->Release();
        allocation->Release();

        if (other.mappedPointer.has_value())
        {
            mappedPointer = other.mappedPointer;
        }

        resource = other.resource;
        allocation = other.allocation;

        return *this;
    }

    Allocation::Allocation(Allocation&& other) noexcept
        : resource(std::move(other.resource)), allocation(std::move(other.allocation))
    {
        if (other.mappedPointer.has_value())
        {
            mappedPointer = other.mappedPointer;
        }
    }

    Allocation& Allocation::operator=(Allocation&& other) noexcept
    {
        resource = std::move(other.resource);
        allocation = std::move(other.allocation);

        if (other.mappedPointer.has_value())
        {
            mappedPointer = other.mappedPointer;
        }

        return *this;
    }

    void Allocation::update(const void* data, const size_t size)
    {
        if (!data || !mappedPointer.has_value())
        {
            throw std::exception(
                "Trying to update resource that is not placed in CPU visible memory, or the data is null");
        }

        memcpy(mappedPointer.value(), data, size);
    }

    void Allocation::reset()
    {
        resource.Reset();
        allocation.Reset();
    }

    // To be used primarily for constant buffers.
    void Buffer::update(const void* data)
    {
        // Allocation's update function will take care if data is null or not.
        allocation->update(data, sizeInBytes);
    }
} // namespace helios::gfx