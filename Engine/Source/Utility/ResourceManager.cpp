#include "ResourceManager.hpp"

#include "Editor/Log.hpp"

#include "Scene/Scene.hpp"
#include "Graphics/API/Device.hpp"

namespace helios::utility
{
	void ResourceManager::LocateAssetsDirectory()
    {
        auto currentDirectory = std::filesystem::current_path();

        while (!std::filesystem::exists(currentDirectory / "Assets"))
        {
            if (currentDirectory.has_parent_path())
            {
                currentDirectory = currentDirectory.parent_path();
            }
            else
            {
                ErrorMessage(L"Assets Directory not found!");
            }
        }

        auto assetsDirectory = currentDirectory / "Assets";

        if (!std::filesystem::is_directory(assetsDirectory))
        {
            ErrorMessage(L"Assets Directory that was located is not a directory!");
        }

        sAssetsDirectory = currentDirectory.wstring() + L"/";

        editor::LogMessage(L"Detected assets path directory : " + sAssetsDirectory, editor::LogMessageTypes::Info);
    }

    std::wstring ResourceManager::GetAssetPath(std::wstring_view relativePath)
    {
        return std::move(sAssetsDirectory + relativePath.data());
    }

    void ResourceManager::LoadModel(const gfx::Device* device, const scene::ModelCreationDesc& modelCreationDesc)
    {
        sLoadedModels[modelCreationDesc.modelName] = std::async(utility::ResourceManager::CreateModel, device, modelCreationDesc);
    }

    std::unique_ptr<scene::Model> ResourceManager::GetLoadedModel(std::wstring_view modelName)
    {
        std::future<std::unique_ptr<scene::Model>> model = std::move(sLoadedModels[modelName.data()]);
        model.wait();
        return std::move(model.get());
    }

    void ResourceManager::LoadSkyBox(gfx::Device* const device, const scene::SkyBoxCreationDesc& skyBoxCreationDesc)
	{
		sLoadedSkyBox[skyBoxCreationDesc.name] = std::async(utility::ResourceManager::CreateSkyBox, device, skyBoxCreationDesc);
    }

    std::unique_ptr<scene::SkyBox> ResourceManager::GetLoadedSkyBox(std::wstring_view skyBoxName)
    {
		std::future<std::unique_ptr<scene::SkyBox>> skyBox = std::move(sLoadedSkyBox[skyBoxName.data()]);
        skyBox.wait();
		return std::move(skyBox.get());
    }

    std::unique_ptr<scene::Model> ResourceManager::CreateModel(const gfx::Device* device, const scene::ModelCreationDesc& modelCreationDesc)
    {
        return std::move(std::make_unique<scene::Model>(device, modelCreationDesc));
    }

    std::unique_ptr<scene::SkyBox> ResourceManager::CreateSkyBox(gfx::Device* const device, const scene::SkyBoxCreationDesc& skyBoxCreationDesc)
    {
        return std::move(std::make_unique<scene::SkyBox>(device, skyBoxCreationDesc));
    }
} // namespace helios::utility