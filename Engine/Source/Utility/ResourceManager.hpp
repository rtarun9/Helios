#pragma once

#include "Graphics/API/Device.hpp"

#include "Scene/Scene.hpp"

namespace helios::utility
{
    // Purely static class that handles creation of all types of resources in a multithreaded way (by default, for now atleast).
    // Use the name (usually passed into the XCreationDesc) to index and get the resource in the unordered_map of any resource type.
	class ResourceManager
    {
    public:
        static void LocateAssetsDirectory();
        static std::wstring GetAssetPath(std::wstring_view assetPath);

        static void LoadModel(const gfx::Device* device, const scene::ModelCreationDesc& modelCreationDesc);
        static std::unique_ptr<scene::Model> GetLoadedModel(std::wstring_view modelName);

		static void LoadSkyBox(gfx::Device* const device, const scene::SkyBoxCreationDesc& skyBoxCreationDesc);
		static std::unique_ptr<scene::SkyBox> GetLoadedSkyBox(std::wstring_view skyBoxName);

    private:
        static std::unique_ptr<scene::Model> CreateModel(const gfx::Device* device, const scene::ModelCreationDesc& modelCreationDesc);
        static std::unique_ptr<scene::SkyBox> CreateSkyBox(gfx::Device* const device, const scene::SkyBoxCreationDesc& skyBoxCreationDesc);

    private:
        static inline std::wstring sAssetsDirectory{};

        // Resources.
        static inline std::unordered_map<std::wstring, std::future<std::unique_ptr<scene::Model>>> sLoadedModels{};
        static inline std::unordered_map<std::wstring, std::future<std::unique_ptr<scene::SkyBox>>> sLoadedSkyBox{};
    };
}