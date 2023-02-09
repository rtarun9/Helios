#pragma once

#include "Core/Application.hpp"
#include "Core/Input.hpp"
#include "Core/FileSystem.hpp"

#include "Graphics/CommandQueue.hpp"
#include "Graphics/Context.hpp"
#include "Graphics/CopyContext.hpp"
#include "Graphics/DescriptorHeap.hpp"
#include "Graphics/GraphicsContext.hpp"
#include "Graphics/GraphicsDevice.hpp"
#include "Graphics/MemoryAllocator.hpp"
#include "Graphics/PipelineState.hpp"
#include "Graphics/Resources.hpp"
#include "Graphics/ShaderCompiler.hpp"
#include "Graphics/d3dx12.hpp"

#include "RenderPass/DeferredGeometryPass.hpp"

#include "Scene/Camera.hpp"
#include "Scene/Materials.hpp"
#include "Scene/Mesh.hpp"
#include "Scene/Model.hpp"
#include "Scene/Lights.hpp"
#include "Scene/Scene.hpp"

#include "ShaderInterlop/ConstantBuffers.hlsli"
#include "ShaderInterlop/RenderResources.hlsli"