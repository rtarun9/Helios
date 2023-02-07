#pragma once

#include "../Graphics/GraphicsDevice.hpp"
#include "../Graphics/ShaderCompiler.hpp"

#include "../Scene/Model.hpp"

namespace helios::core
{
    // NOTE : This class serves very little purpose (only for getting full path). Might be removed in future versions of helios.

    class ResourceManager
    {
      public:
        static inline std::string getFullPath(const std::string_view assetPath)
        {
            return std::move(s_rootDirectoryPath + assetPath.data());
        }

        static inline std::wstring getFullPath(const std::wstring_view assetPath)
        {
            return std::move(stringToWString(s_rootDirectoryPath) + assetPath.data());
        }
            
        static void locateRootDirectory();

      private:
        static inline std::string s_rootDirectoryPath{};
    };
} // namespace helios::core