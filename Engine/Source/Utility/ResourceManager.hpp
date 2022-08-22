#pragma once
#include "Scene/Scene.hpp"

namespace helios::utility
{
    // Note : In the future, this will take care of loading some assets (currently models & materials are loaded by scene::Model and the device.
    // For now, will just act as a holder of the assets dir and spawns new thread for loading of models and resources.
	class ResourceManager
    {
    public:
        static void LocateAssetsDirectory();
        static std::wstring GetAssetPath(std::wstring_view assetPath);

        static void LoadResourceToScene(scene::Scene* scene, const std::function<void(scene::Scene*)>& function);
        static void WaitForThreads();

    private:
        static inline std::wstring sAssetsDirectory{};
        static inline std::vector<std::thread> sResourceCreationThreads{};
    };
}