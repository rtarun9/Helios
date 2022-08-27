#pragma once

#include "Core/Application.hpp"
#include "Core/Engine.hpp"
#include "Core/Timer.hpp"

#include "Utility/Helpers.hpp"
#include "Utility/ResourceManager.hpp"

#include "Editor/Editor.hpp"
#include "Editor/Log.hpp"

#include "Graphics/API/CommandQueue.hpp"
#include "Graphics/API/ComputeContext.hpp"
#include "Graphics/API/Descriptor.hpp"
#include "Graphics/API/Device.hpp"
#include "Graphics/API/GraphicsContext.hpp"
#include "Graphics/API/MemoryAllocator.hpp"
#include "Graphics/API/MipMapGenerator.hpp"
#include "Graphics/API/PipelineState.hpp"
#include "Graphics/API/Resources.hpp"

#include "Graphics/RenderPass/DeferredGeometryPass.hpp"

#include "Scene/Camera.hpp"
#include "Scene/Light.hpp"
#include "Scene/Model.hpp"
#include "Scene/Scene.hpp"
#include "Scene/SkyBox.hpp"

// File with all constant buffer structs shared between C++ and HLSL.
#include "Common/ConstantBuffers.hlsli"