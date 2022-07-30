#pragma once


#include "Pch.hpp"
/*
#include "Graphics/API/ConstantBuffer.hpp"

#include "Graphics/Model/Model.hpp"

namespace helios::gfx
{
	struct PointLightData
	{
		float radius{0.05f};
		DirectX::XMFLOAT4 lightPosition{};
		DirectX::XMFLOAT4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
	};

	struct DirectionalLightData
	{
		float sunAngle{33.0f};
		DirectX::XMFLOAT4 lightColor{1.0f, 1.0f, 1.0f, 1.0f};
	};

	enum class LightTypes : uint8_t
	{
		PointLightData,
		DirectionalLightData
	};

	static constexpr uint8_t UNSPECIFIED_LIGHT_NUMBER = -1;
	static constexpr const wchar_t *SPHERE_MODEL_PATH = L"Assets/Models/Sphere/scene.gltf";

	// This common light is used for all types of Light (Point, directional as of now and in future (area, punctual etc)) used in the Engine.
	// It is made common for simplicity (similar to how the Texture Class is common for TextureUAV, Texture to be loaded using stbi, etc).
	// This is also why there is a common constant buffer for all light types.
	// Class inherits from Model class just for visualization of punctual lights. 
	class Light : public Model
	{
	public:
		static void InitLightDataCBuffer(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor);
		// -1 is default light number. Stands for unspecified.
		void Init(ID3D12Device* const device, ID3D12GraphicsCommandList* const commandList, gfx::Descriptor& srvCbDescriptor, std::variant<PointLightData, DirectionalLightData> lightData, uint32_t lightNumber = UNSPECIFIED_LIGHT_NUMBER);

		void UpdateData();

		// Only point lights will have a transform component.
		void UpdateTransformData(ID3D12GraphicsCommandList* const commandList, DirectX::XMMATRIX& projectionMatrix, DirectX::XMMATRIX& viewMatrix);

		uint32_t GetLightIndex() const { return m_LightNumber; };
		uint32_t GetLightType() const { return m_LightType; };

		static LightData& GetLightData() { return s_LightData.GetBufferData(); }
		static uint32_t GetLightDataCBufferIndex() { return s_LightData.GetBufferIndex(); }

	private:
		uint32_t m_LightNumber{};
		
		// This is used as certain functions are only applicable for certain light types.
		// ex : UpdateTransformData are to be used only if the light type is a punctual light.
		uint32_t m_LightType{};
		std::variant<PointLightData, DirectionalLightData> m_LightData{};
		
		static inline gfx::ConstantBuffer<LightData> s_LightData{};
	};
}
*/