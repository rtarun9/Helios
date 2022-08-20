#pragma once

namespace helios::utility
{
    // Note : In the future, this will take care of loading some assets (currently models & materials are loaded by scene::Model and the device.
    // For now, will just act as a holder of the assets dir.
	class ResourceManager
    {
    public:
        static void LocateAssetsDirectory();
        static std::wstring GetAssetPath(std::wstring_view assetPath);

    private:
        static inline std::wstring sAssetsDirectory{};
    };
}