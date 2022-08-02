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

	// This common light is used for all types of Light (Point, directional, area, punctual etc sed in the Engine.
	// This is also why there is a common buffer for all light types.
	// NOTE : For any data updating, just use the Transform object this class provides, which in turn will update data into the common
	// static LightBuffer object.
	class Light 
	{
	public:
		static void CreateLightResources(const gfx::Device* device);
		static void DestroyLightResources();

		Light(const gfx::Device* device, const LightCreationDesc& lightCreationDesc);
		uint32_t GetLightNumber() const { return mLightNumber; }
		LightTypes GetLightType() const { return mLightType; }

		// This function is overrided as the base (Model) class also derives from thsi.
		Transform* GetTransform() { return &mTransform; };

		static LightBuffer& GetLightBufferData() { return sLightBufferData; }
		static uint32_t GetCbvIndex() { return sLightBuffer->cbvIndex; }
		
		// Update light transform (the light position).
		void Update();
		
		// Update the static member variables (light buffer).
		static void UpdateLightBuffer();

	public:
		static constexpr float DIRECTIONAL_LIGHT_ANGLE{ -153.0f };
	
	private:
		static constexpr inline const wchar_t*  LIGHT_MODEL_PATH = L"Assets/Models/Sphere/scene.gltf";

		LightTypes mLightType{};
		uint32_t mLightNumber{};

		static inline LightBuffer sLightBufferData{};
		static inline std::unique_ptr<gfx::Buffer> sLightBuffer{};

		Transform mTransform;

		// All lights (point) will use a same mesh, which is static.
		// However, note that each light has its own transform, as instanced rendering will be used for light visualization.
		static inline std::unique_ptr<Model> sLightModel;
	};
}