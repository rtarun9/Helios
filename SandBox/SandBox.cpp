#include "Pch.hpp"
#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli" 
#include "Common/ConstantBuffers.hlsli"

using namespace helios;
using namespace DirectX;

SandBox::SandBox(core::Config& config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();

	// Load scene and its data.
	mScene = std::make_unique<scene::Scene>(mDevice.get());

	scene::ModelCreationDesc cubeCreationDesc
	{
		.modelPath = L"Assets/Models/Cube/glTF/Cube.gltf",
		.modelName = L"Cube",
	};
	
	auto cube = std::make_unique<scene::Model>(mDevice.get(), cubeCreationDesc);
	cube->GetTransform()->data.translate = { 0.0f, 5.0f, 0.0f };
	mScene->AddModel(std::move(cube));
	
	scene::ModelCreationDesc DamagedHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf",
		.modelName = L"DamagedHelmet",
	};
	
	auto damagedHelmet = std::make_unique<scene::Model>(mDevice.get(), DamagedHelmetCreationDesc);
	damagedHelmet->GetTransform()->data.rotation = { math::XMConvertToRadians(63.0f), 0.0f, 0.0f };
	mScene->AddModel(std::move(damagedHelmet));
	
	scene::ModelCreationDesc sciFiHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf",
		.modelName = L"SciFiHelmet",
	};
	
	auto sciFiHelmet = std::make_unique<scene::Model>(mDevice.get(), sciFiHelmetCreationDesc);
	sciFiHelmet->GetTransform()->data.translate = { 5.0f, 0.0f, 0.0f };
	mScene->AddModel(std::move(sciFiHelmet));
	
	scene::ModelCreationDesc metalRoughSpheresCreationDesc
	{
		.modelPath = L"Assets/Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
		.modelName = L"MetalRoughSpheres",
	};
	
	auto metalRoughSpheres = std::make_unique<scene::Model>(mDevice.get(), metalRoughSpheresCreationDesc);
	metalRoughSpheres->GetTransform()->data.translate = { -15.0f, 0.0f, 0.0f };
	mScene->AddModel(std::move(metalRoughSpheres));

	scene::ModelCreationDesc sponzaCreationDesc
	{
		.modelPath = L"Assets/Models/Sponza/glTF/Sponza.gltf",
		.modelName = L"Sponza Scene",
	};
	
	auto sponza = std::make_unique<scene::Model>(mDevice.get(), sponzaCreationDesc);
	sponza->GetTransform()->data.scale = { 0.1f, 0.1f, 0.1f};
	mScene->AddModel(std::move(sponza));

	// Random generator.
	std::mt19937_64 rng{};
    
	uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	
	std::seed_seq ss{ uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed >> 32) };
	
	rng.seed(ss);

	std::uniform_real_distribution<double> uniformDistribution(0, 1);

	// Load lights.
	
	for (uint32_t i : std::views::iota(0u, TOTAL_POINT_LIGHTS))
	{
		scene::LightCreationDesc pointLightcreationDesc2
		{
			.lightNumber = i,
			.lightType = scene::LightTypes::PointLightData
		};

		mScene->AddLight(mDevice.get(), pointLightcreationDesc2);

		scene::Light::GetLightBufferData()->lightColor[i] = DirectX::XMFLOAT4(1.0f - (float)i/ TOTAL_POINT_LIGHTS, 1.0f, (float)i / TOTAL_POINT_LIGHTS, 1.0f);

		scene::Light::GetLightBufferData()->lightPosition[i] = DirectX::XMFLOAT4(12.0f * (i % 10) - 22.0f, 2.0f, 6.0f * (i / 10) - 18.0f, 1.0f);
	}

	scene::LightCreationDesc directionalLightCreationDesc
	{
		.lightNumber = DIRECTIONAL_LIGHT_OFFSET + 0u,
		.lightType = scene::LightTypes::DirectionalLightData
	};

	mScene->AddLight(mDevice.get(), directionalLightCreationDesc);

	// Load skybox.
	scene::SkyBoxCreationDesc skyBoxCreationDesc
	{
		.equirectangularTexturePath = L"Assets/Textures/Environment.hdr",
		.format = DXGI_FORMAT_R32G32B32A32_FLOAT,
		.name = L"Sky Box"
	};

	mScene->AddSkyBox(mDevice.get(), skyBoxCreationDesc);

	// Create post process buffer.
	gfx::BufferCreationDesc postProcessBufferCreationDesc
	{
		.usage = gfx::BufferUsage::ConstantBuffer,
		.name = L"Post Process Buffer",
	};

	mPostProcessBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<PostProcessBuffer>(postProcessBufferCreationDesc, std::span<PostProcessBuffer, 0u>{}));
	mPostProcessBufferData =
	{
		.exposure = 1.0f
	};

	CreatePipelineStates();

	// Load depth stencil texture.
	gfx::TextureCreationDesc depthStencilTextureCreationDesc
	{
		.usage = gfx::TextureUsage::DepthStencil,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R32_FLOAT,
		.name = L"Depth Stencil Texture"
	};

	mDepthStencilTexture = std::make_unique<gfx::Texture>(mDevice->CreateTexture(depthStencilTextureCreationDesc));
	
	gfx::TextureCreationDesc fowardDepthStencilTextureCreationDesc
	{
		.usage = gfx::TextureUsage::DepthStencil,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R32_FLOAT,
		.name = L"Forward Depth Stencil Texture"
	};

	mForwardRenderingDepthStencilTexture = std::make_unique<gfx::Texture>(mDevice->CreateTexture(fowardDepthStencilTextureCreationDesc));

	// Load render targets.
	gfx::TextureCreationDesc offscreenRenderTargetTextureCreationDesc
	{
		.usage = gfx::TextureUsage::RenderTarget,
		.dimensions = mDimensions,
		.format = DXGI_FORMAT_R16G16B16A16_FLOAT,
		.name = L"Offscreen Render Texture"
	};

	mOffscreenRT = std::make_unique<gfx::RenderTarget>(mDevice->CreateRenderTarget(offscreenRenderTargetTextureCreationDesc));

	gfx::TextureCreationDesc postProcessRenderTargetsTextureCreationDesc
	{
		.usage = gfx::TextureUsage::RenderTarget,
		.dimensions = mDimensions,
		.format = gfx::Device::SWAPCHAIN_FORMAT,
		.name = L"Post Process Render Texture"
	};

	mPostProcessingRT = std::make_unique<gfx::RenderTarget>(mDevice->CreateRenderTarget(postProcessRenderTargetsTextureCreationDesc));
	
	gfx::TextureCreationDesc finalRenderTargetsTextureCreationDesc
	{
		.usage = gfx::TextureUsage::RenderTarget,
		.dimensions = mDimensions,
		.format = gfx::Device::SWAPCHAIN_FORMAT,
		.name = L"Final Render Texture"
	};

	mFinalRT = std::make_unique<gfx::RenderTarget>(mDevice->CreateRenderTarget(finalRenderTargetsTextureCreationDesc));

	// Init render passes.
	mDeferredGPass = std::make_unique<gfx::DeferredGeometryPass>(mDevice.get(), mDimensions);

	// Init other scene objects.
	mEditor = std::make_unique<editor::Editor>(mDevice.get());
}

void SandBox::OnUpdate()
{
	mScene->Update(mAspectRatio);

	mPostProcessBuffer->Update(&mPostProcessBufferData);
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> graphicsContext = mDevice->GetGraphicsContext();
	gfx::BackBuffer* backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();

	// Configure offscreen render target's.
	std::array<const gfx::RenderTarget*, 1u> renderTargets
	{
		mOffscreenRT.get()
	};

	static std::array<float, 4> clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

	// Renderpass 0 : Deferred Geometry pass
	{
		mDeferredGPass->Render(mScene.get(), graphicsContext.get(), mDepthStencilTexture.get());

		// Copying the depth stencil texture so it can be used for rendering lights (which is forward rendering).
		graphicsContext->AddResourceBarrier(mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		graphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->CopyResource(mDepthStencilTexture->GetResource(), mForwardRenderingDepthStencilTexture->GetResource());

		graphicsContext->AddResourceBarrier(mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		graphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		graphicsContext->ExecuteResourceBarriers();
	}

	// RenderPass 1 : Do shading on offscreen RT (deferred lighting pass) and then render lights using forward rendering.
	{
		graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->SetGraphicsPipelineState(mBlinnPhongPipelineState.get());
		graphicsContext->SetRenderTarget(renderTargets, mDepthStencilTexture.get());
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearRenderTargetView(renderTargets, clearColor);
		graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		DeferredLightingPassRenderResources deferredLightingPassRenderResources
		{
			.lightBufferIndex = scene::Light::GetCbvIndex(),
			.sceneBufferIndex = mScene->mSceneBuffer->cbvIndex,
			
			.albedoGBufferIndex = mDeferredGPass->mDeferredPassRTs.albedoRT->GetRenderTextureSRVIndex(),
			.normalGBufferIndex = mDeferredGPass->mDeferredPassRTs.normalRT->GetRenderTextureSRVIndex(),
			.positionGBufferIndex = mDeferredGPass->mDeferredPassRTs.positionRT->GetRenderTextureSRVIndex()
		};

		gfx::RenderTarget::Render(graphicsContext.get(), deferredLightingPassRenderResources);
		
		// Render lights and skybox using forward rendering.

		graphicsContext->SetGraphicsPipelineState(mLightPipelineState.get());
		graphicsContext->SetRenderTarget(renderTargets, mForwardRenderingDepthStencilTexture.get());

		mScene->RenderLights(graphicsContext.get());

		graphicsContext->SetGraphicsPipelineState(mSkyBoxPipelineState.get());

		mScene->RenderSkyBox(graphicsContext.get());

		graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
	}
	
	// Render pass 2 : Render offscreen rt to post processed RT (after all processing has occured).
	{
		graphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->SetGraphicsPipelineState(mPostProcessingPipelineState.get());
		graphicsContext->SetRenderTarget(mPostProcessingRT.get(), mDepthStencilTexture.get());
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearRenderTargetView(mPostProcessingRT.get(), std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = mOffscreenRT->renderTexture->srvIndex,
			.postProcessBufferIndex = mPostProcessBuffer->cbvIndex
		};

		gfx::RenderTarget::Render(graphicsContext.get(), rtvRenderResources);
	}

	// Render pass 3 : The RT that is to be displayed to swapchain is processed. For now, UI is rendered in this RT as well.
	{
		graphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		graphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->SetGraphicsPipelineState(mFinalPipelineState.get());
		graphicsContext->SetRenderTarget(mFinalRT.get(), mDepthStencilTexture.get());
		graphicsContext->SetDefaultViewportAndScissor();
		graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		graphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = mPostProcessingRT->renderTexture->srvIndex,
		};

		gfx::RenderTarget::Render(graphicsContext.get(), rtvRenderResources);

		mEditor->Render(mDevice.get(), mScene.get(), &mDeferredGPass->mDeferredPassRTs, clearColor, mPostProcessBufferData, mDevice->GetTextureSrvDescriptorHandle(mPostProcessingRT->renderTexture.get()), graphicsContext.get());
	}

	// Render pass 3 : Copy the final RT to the swapchain
	{
		graphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		graphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		graphicsContext->ExecuteResourceBarriers();

		graphicsContext->CopyResource(mFinalRT->GetResource(), backBuffer->GetResource());

		graphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		graphicsContext->ExecuteResourceBarriers();
	}


	mDevice->EndFrame();

	mDevice->ExecuteContext(std::move(graphicsContext));

	mDevice->Present();

	mFrameIndex++;
}

void SandBox::OnDestroy()
{
	scene::Light::DestroyLightResources();
	mEditor.reset();
}

void SandBox::OnKeyAction(uint8_t keycode, bool isKeyDown) 
{
	if (isKeyDown && keycode == VK_SPACE)
	{
		mEditor->ShowUI(false);
	}
	else if (isKeyDown && keycode == VK_SHIFT)
	{
		mEditor->ShowUI(true);
	}

	if (isKeyDown && keycode == VK_F5)
	{
		mDevice->EnableVSync();
	}
	else if (isKeyDown && keycode == VK_F6)
	{
		mDevice->DisableVSync();
	}

	if (isKeyDown && keycode == 'R')
	{
		mDevice->GetComputeCommandQueue()->FlushQueue();
		mDevice->GetGraphicsCommandQueue()->FlushQueue();

		CreatePipelineStates();
	}

	mScene->mCamera->HandleInput(keycode, isKeyDown);
}

void SandBox::OnResize() 
{
	if (mDimensions != core::Application::GetClientDimensions())
	{
		mDevice->ResizeBuffers();

		mDimensions = core::Application::GetClientDimensions();

		mAspectRatio = static_cast<float>(mDimensions.x) / static_cast<float>(mDimensions.y);

		// Recreate RTV and SRV of all render targets.
		mDevice->ResizeRenderTarget(mFinalRT.get());
		mDevice->ResizeRenderTarget(mOffscreenRT.get());
		mDevice->ResizeRenderTarget(mPostProcessingRT.get());

		mDeferredGPass->Resize(mDevice.get(), mDimensions);

		mEditor->OnResize(core::Application::GetClientDimensions());
	}
}

void SandBox::CreatePipelineStates()
{
	// Load pipeline states.

	gfx::GraphicsPipelineStateCreationDesc postProcessGraphicsPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/RenderPass/PostProcessRenderPassVS.cso",
			.psShaderPath = L"Shaders/RenderPass/PostProcessRenderPassPS.cso",
		},
		.rtvFormat = gfx::Device::SWAPCHAIN_FORMAT,
		.pipelineName = L"Post Process RenderPass Pipeline"
	};

	mPostProcessingPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(postProcessGraphicsPipelineStateCreationDesc));

	gfx::GraphicsPipelineStateCreationDesc blinnPhongPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/Shading/BlinnPhongVS.cso",
			.psShaderPath = L"Shaders/Shading/BlinnPhongPS.cso",
		},
		.pipelineName = L"Blinn Phong Pipeline"
	};

	mBlinnPhongPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(blinnPhongPipelineStateCreationDesc));

	gfx::GraphicsPipelineStateCreationDesc lightPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/Light/LightVS.cso",
			.psShaderPath = L"Shaders/Light/LightPS.cso",
		},
		.pipelineName = L"Light Pipeline"
	};

	mLightPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(lightPipelineStateCreationDesc));

	gfx::GraphicsPipelineStateCreationDesc finalRenderPassPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/RenderPass/FinalRenderPassVS.cso",
			.psShaderPath = L"Shaders/RenderPass/FinalRenderPassPS.cso",
		},
		.rtvFormat = gfx::Device::SWAPCHAIN_FORMAT,
		.depthFormat = DXGI_FORMAT_D32_FLOAT,
		.pipelineName = L"Final Render Target Pipeline"
	};

	mFinalPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(finalRenderPassPipelineStateCreationDesc));

	gfx::GraphicsPipelineStateCreationDesc skyBoxPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/SkyBox/SkyBoxVS.cso",
			.psShaderPath = L"Shaders/SkyBox/SkyBoxPS.cso",
		},
		.rtvFormat = DXGI_FORMAT_R16G16B16A16_FLOAT,
		.depthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		.frontFaceWindingOrder = gfx::FrontFaceWindingOrder::CounterClockWise,
		.pipelineName = L"Sky Box Pipeline"
	};


	mSkyBoxPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(skyBoxPipelineStateCreationDesc));
}
