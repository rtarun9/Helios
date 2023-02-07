#include "Core/ResourceManager.hpp"

namespace helios::core
{
    void ResourceManager::locateRootDirectory()
    {
        auto currentDirectory = std::filesystem::current_path();

        // The asset directory is one folder within the root directory.
        while (!std::filesystem::exists(currentDirectory / "Assets"))
        {
            if (currentDirectory.has_parent_path())
            {
                currentDirectory = currentDirectory.parent_path();
            }
            else
            {
                fatalError("Assets Directory not found!");
            }
        }

        auto assetsDirectory = currentDirectory / "Assets";

        if (!std::filesystem::is_directory(assetsDirectory))
        {
            fatalError("Assets Directory that was located is not a directory!");
        }

        s_rootDirectoryPath = currentDirectory.string() + "/";

        log(std::format("Detected root directory at path : {}.", s_rootDirectoryPath));
    }
} // namespace helios::core