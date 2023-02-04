#pragma once

#include "../Graphics/GraphicsDevice.hpp"
#include "../Graphics/ShaderCompiler.hpp"

namespace helios::core
{
    // NOTE : IS NOT RESPONSIBLE FOR CREATION OF TEXTURES / BUFFERS ETC. Those are handled by the GraphicsDevice as they are considered to be
    // 'GPU Resources'. This static class is more for Model loading / path resolution, etc.
    // Purely static class that handles creation of all types of resources in a multi threaded way (by default, for
    // models). Use the name (usually passed into the XCreationDesc) to index and get the resource in the unordered_map
    // of any resource type.

    class ResourceManager
    {
      public:
        static inline std::string getRootDirectoryPath(const std::string_view assetPath)
        {
            return std::move(s_rootDirectoryPath + assetPath.data());
        }

        static inline std::wstring getRootDirectoryPath(const std::wstring_view assetPath)
        {
            return std::move(stringToWString(s_rootDirectoryPath) + assetPath.data());
        }

        static void locateRootDirectory();

        static gfx::Shader compileShader(const gfx::ShaderTypes& shaderType, const std::wstring_view shaderPath,
                                         const std::wstring_view entryPoint, const bool extractRootSignature = false);

      private:
        static inline std::string s_rootDirectoryPath{};
    };
} // namespace helios::core