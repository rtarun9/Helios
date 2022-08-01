#pragma once

#include "Pch.hpp"

#include "Graphics/API/Resources.hpp"
#include "Graphics/API/Device.hpp"

#include "Model.hpp"

#include "Common/ConstantBuffers.hlsli"

namespace helios::scene
{
	enum class LightTypes : uint8_t
	{
		PointLightData,
		DirectionalLightData
	};

	struct LightCreationDesc
	{
		uint32_t lightNumber{};
		LightTypes lightType{};
	};

	static constexpr uint8_t UNSPECIFIED_LIGHT_NUMBER = -1;
	static constexpr const wchar_t *SPHERE_MODEL_PATH = L"Assets/Models/Sphere/scene.gltf";

	// This common light is used for all types of Light (Point, directional as of now and in future (area, punctual etc)) used in the Engine.
	// This is also why there is a common buffer for all light types.
	// Class inherits from Model class just for visualization of punctual lights. 
	// note(rtarun9) : No support for point lights for now, simply because they wont be used and require differnet ways of representing the lights.
	class Light : public Model
	{
	public:
		static void CreateLightDataBuffer(const gfx::Device* device);
		static void DestroyLightDataBuffer();

		Light(const LightCreationDesc& lightCreationDesc);
		uint32_t GetLightNumber() const { return mLightNumber; }
		LightTypes GetLightType() const { return mLightType; }

		static LightBuffer& GetLightBufferData() { return sLightBufferData; }
		static uint32_t GetCbvIndex() { return sLightBuffer->cbvIndex; }
		static void Update();

	private:
		LightTypes mLightType{};
		uint32_t mLightNumber{};

		static inline LightBuffer sLightBufferData{};
		static inline std::unique_ptr<gfx::Buffer> sLightBuffer{};
	};
}