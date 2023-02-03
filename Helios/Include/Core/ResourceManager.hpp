#pragma once

#include "../Graphics/GraphicsDevice.hpp"
#include "../Graphics/ShaderCompiler.hpp"

namespace helios::core
{
    // Purely static class that handles creation of all types of resources in a multi threaded way (by default, for models). 
    // Use the name (usually passed into the XCreationDesc) to index and get the resource in the unordered_map
    // of any resource type.

    class ResourceManager
    {
      public:
        static inline std::string getAssetsPath(const std::string_view assetPath) 
        {
            return std::move(s_rootDirectoryPath + assetPath.data());
        }

        static inline std::wstring getAssetsPath(const std::wstring_view assetPath) 
        {
            return std::move(stringToWString(s_rootDirectoryPath) + assetPath.data());
        }

        static void locateRootDirectory();

        static gfx::Shader compileShader(const gfx::ShaderTypes& shaderType, const std::wstring_view shaderPath,
                                         const bool extractRootSignature = false);

      private:
        static inline std::string s_rootDirectoryPath{};
    };
} // namespace helios::core