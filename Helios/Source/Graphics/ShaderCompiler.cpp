#include "Graphics/ShaderCompiler.hpp"
#include "Core/ResourceManager.hpp"

namespace helios::gfx::ShaderCompiler
{
    // Responsible for the actual compilation of shaders.
    wrl::ComPtr<IDxcCompiler3> compiler{};

    // Used to create include handle and provides interfaces for loading shader to blob, etc.
    wrl::ComPtr<IDxcUtils> utils{};
    wrl::ComPtr<IDxcIncludeHandler> includeHandler{};

    std::wstring shaderDirectory{};

    Shader compile(const ShaderTypes& shaderType, const std::wstring_view shaderPath, const std::wstring_view entryPoint, const bool extractRootSignature)
    {
        Shader shader{};

        if (!utils)
        {
            throwIfFailed(::DxcCreateInstance(CLSID_DxcUtils, IID_PPV_ARGS(&utils)));
            throwIfFailed(::DxcCreateInstance(CLSID_DxcCompiler, IID_PPV_ARGS(&compiler)));
            throwIfFailed(utils->CreateDefaultIncludeHandler(&includeHandler));

            shaderDirectory = core::ResourceManager::getFullPath(L"Shaders");
            log(std::format(L"Shader base directory : {}.", shaderDirectory));
        }

        // Setup compilation arguments.
        const std::wstring targetProfile = [=]() {
            switch (shaderType)
            {
            case ShaderTypes::Vertex: {
                return L"vs_6_6";
            }
            break;

            case ShaderTypes::Pixel: {
                return L"ps_6_6";
            }
            break;

            case ShaderTypes::Compute: {
                return L"cs_6_6";
            }
            break;

            default: {
                return L"";
            }
            break;
            }
        }();

        std::vector<LPCWSTR> compilationArguments{
            L"-E",
            entryPoint.data(),
            L"-T",
            targetProfile.c_str(),
            DXC_ARG_PACK_MATRIX_ROW_MAJOR,
            DXC_ARG_WARNINGS_ARE_ERRORS,
            DXC_ARG_ALL_RESOURCES_BOUND,
            L"-I",
            shaderDirectory.c_str(),
        };

        // Indicate that the shader should be in a debuggable state if in debug mode.
        // Else, set optimization level to 03.
        if constexpr (HELIOS_DEBUG_MODE)
        {
            compilationArguments.push_back(DXC_ARG_DEBUG);
        }
        else
        {
            compilationArguments.push_back(DXC_ARG_OPTIMIZATION_LEVEL3);
        }

        // Load the shader source file to a blob.
        wrl::ComPtr<IDxcBlobEncoding> sourceBlob{nullptr};
        throwIfFailed(utils->LoadFile(shaderPath.data(), nullptr, &sourceBlob));

        const DxcBuffer sourceBuffer = {
            .Ptr = sourceBlob->GetBufferPointer(),
            .Size = sourceBlob->GetBufferSize(),
            .Encoding = 0u,
        };

        // Compile the shader.
        wrl::ComPtr<IDxcResult> compiledShaderBuffer{};
        const HRESULT hr = compiler->Compile(&sourceBuffer, compilationArguments.data(),
                                             static_cast<uint32_t>(compilationArguments.size()), includeHandler.Get(),
                                             IID_PPV_ARGS(&compiledShaderBuffer));
        if (FAILED(hr))
        {
            fatalError(std::format("Failed to compile shader with path : {}", wStringToString(shaderPath)));
        }

        // Get compilation errors (if any).
        wrl::ComPtr<IDxcBlobUtf8> errors{};
        throwIfFailed(compiledShaderBuffer->GetOutput(DXC_OUT_ERRORS, IID_PPV_ARGS(&errors), nullptr));
        if (errors && errors->GetStringLength() > 0)
        {
            const LPCSTR errorMessage = errors->GetStringPointer();
            fatalError(errorMessage);
        }

        wrl::ComPtr<IDxcBlob> compiledShaderBlob{nullptr};
        compiledShaderBuffer->GetOutput(DXC_OUT_OBJECT, IID_PPV_ARGS(&compiledShaderBlob), nullptr);

        shader.shaderBlob = compiledShaderBlob;

        wrl::ComPtr<IDxcBlob> rootSignatureBlob{nullptr};
        if (extractRootSignature)
        {
            compiledShaderBuffer->GetOutput(DXC_OUT_ROOT_SIGNATURE, IID_PPV_ARGS(&rootSignatureBlob), nullptr);
            shader.rootSignatureBlob = rootSignatureBlob;
        }

        return shader;
    }
} // namespace helios::gfx::ShaderCompiler