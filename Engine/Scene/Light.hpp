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

	// This common light is used for all types of Light (Point, directional, area, punctual etc in the Engine.
	// This is also why there is a common buffer for all light types.
	class Light 
	{
	public:
		static void CreateLightResources(const gfx::Device* device);
		static void DestroyLightResources();

		Light(const gfx::Device* device, const LightCreationDesc& lightCreationDesc);
		uint32_t GetLightNumber() const { return mLightNumber; }
		LightTypes GetLightType() const { return mLightType; }

		static LightBuffer* GetLightBufferData() { return &sLightBufferData; }
		static uint32_t GetCbvIndex() { return gfx::Buffer::GetCbvIndex(sLightBuffer.get()); }
		
		// Update light position (form the static b).
		void Update();
		
		// Update the static member variables (light buffer).
		static void UpdateLightBuffer();

		// note(rtarun9) Its a bit frustrating to use this LightRenderResources as in each relevant function call a new struct is created which copies some data into it and passes on the struct 
		// to another function (and so on). For this, desisgnated initializers will not be used here. Also, the function will take a reference to the struct and not a const ref, which is a exception.
		static void Render(const gfx::GraphicsContext* graphicsContext, LightRenderResources& lightRenderResources);

	public:
		static constexpr float DIRECTIONAL_LIGHT_ANGLE{ -153.0f };
	
	private:
		static constexpr inline const wchar_t*  LIGHT_MODEL_PATH = L"Assets/Models/Sphere/scene.gltf";

		LightTypes mLightType{};
		uint32_t mLightNumber{};

		static inline LightBuffer sLightBufferData{};
		static inline std::unique_ptr<gfx::Buffer> sLightBuffer{};
		
		// Currently stores the model matrices required for instanced rendering.
		static inline std::unique_ptr<gfx::Buffer> sLightInstanceBuffer{};
		static inline InstanceLightBuffer sLightInstanceData{};

		// All lights (point) will use a same mesh, which is static.
		// However, note that each light has its own model matrices, as instanced rendering will be used for light visualization.
		static inline std::unique_ptr<Model> sLightModel;
	};
}