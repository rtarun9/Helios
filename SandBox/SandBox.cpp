#include "SandBox.hpp"

#include "Common/BindlessRS.hlsli"
#include "Common/ConstantBuffers.hlsli"

using namespace helios;
using namespace DirectX;

SandBox::SandBox(core::Config &config)
	: Engine(config)
{
}

void SandBox::OnInit()
{
	mDevice = std::make_unique<gfx::Device>();

	// Load scene and its data.
	mScene = std::make_unique<scene::Scene>(mDevice.get());

	// Load scene resources.
	scene::ModelCreationDesc floorCreationDesc
	{
		.modelPath = L"Assets/Models/Cube/glTF/Cube.gltf",
		.modelName = L"Floor",
	};
	utility::ResourceManager::LoadModel(mDevice.get(), floorCreationDesc);


	scene::ModelCreationDesc damagedHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/DamagedHelmet/glTF/DamagedHelmet.gltf",
		.modelName = L"DamagedHelmet",
	};
	utility::ResourceManager::LoadModel(mDevice.get(), damagedHelmetCreationDesc);
	
	scene::ModelCreationDesc sciFiHelmetCreationDesc
	{
		.modelPath = L"Assets/Models/SciFiHelmet/glTF/SciFiHelmet.gltf",
		.modelName = L"SciFiHelmet",
	};
	utility::ResourceManager::LoadModel(mDevice.get(), sciFiHelmetCreationDesc);
	
	scene::ModelCreationDesc metalRoughSpheresCreationDesc
	{
		.modelPath = L"Assets/Models/MetalRoughSpheres/glTF/MetalRoughSpheres.gltf",
		.modelName = L"MetalRoughSpheres",
	};
	utility::ResourceManager::LoadModel(mDevice.get(), metalRoughSpheresCreationDesc);

#if 1
	scene::ModelCreationDesc sponzaCreationDesc
	{
		.modelPath = L"Assets/Models/sponza-gltf-pbr/sponza.glb",
		.modelName = L"Sponza",
	};
	utility::ResourceManager::LoadModel(mDevice.get(), sponzaCreationDesc);

#endif
	// Load lights.

	for (uint32_t i : std::views::iota(0u, TOTAL_POINT_LIGHTS))
	{
		scene::LightCreationDesc pointLightcreationDesc
		{
			.lightNumber = i,
			.lightType = scene::LightTypes::PointLightData
		};

		mScene->AddLight(mDevice.get(), pointLightcreationDesc);

		scene::Light::GetLightBufferData()->lightPosition[i] = DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
#if 0
		// For deferred lighting visualization
		scene::Light::GetLightBufferData()->lightColor[i] = DirectX::XMFLOAT4(1.0f - (float)i / TOTAL_POINT_LIGHTS, 1.0f, (float)i / TOTAL_POINT_LIGHTS, 1.0f);
		scene::Light::GetLightBufferData()->lightPosition[i] = DirectX::XMFLOAT4(12.0f * (i % 20) - 150.0f, 2.0f, 6.0f * (i / 20) - 50.0f, 1.0f);
#endif
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
		.equirectangularTexturePath = L"Assets/Textures/neon_photostudio_8k.hdr",
		.format = DXGI_FORMAT_R32G32B32A32_FLOAT,
		.name = L"SkyBox"
	};

	utility::ResourceManager::LoadSkyBox(mDevice.get(), skyBoxCreationDesc);

	std::jthread pipelineThread([&]() {CreatePipelineStates(); });

	// Create post process buffer.
	gfx::BufferCreationDesc postProcessBufferCreationDesc
	{
		.usage = gfx::BufferUsage::ConstantBuffer,
		.name = L"Post Process Buffer",
	};

	mPostProcessBuffer = std::make_unique<gfx::Buffer>(mDevice->CreateBuffer<PostProcessBuffer>(postProcessBufferCreationDesc));
	mPostProcessBufferData =
	{
		.exposure = 1.0f
	};


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
	{ .usage = gfx::TextureUsage::RenderTarget,
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
	mShadowPass = std::make_unique<gfx::ShadowPass>(mDevice.get());

	// Init other scene objects.
	mEditor = std::make_unique<editor::Editor>(mDevice.get());

	// Add models to scene.
	auto floor = utility::ResourceManager::GetLoadedModel(L"Floor");
	floor->GetTransform()->data.scale = { 100.0f, 0.1f, 100.0f };
	floor->GetTransform()->data.translate = { 0.0f, -10.0f, 0.0f };
	mScene->AddModel(std::move(floor));

	auto damagedHelmet = utility::ResourceManager::GetLoadedModel(L"DamagedHelmet");
	damagedHelmet->GetTransform()->data.rotation = { math::XMConvertToRadians(63.0f), 0.0f, 0.0f };
	mScene->AddModel(std::move(damagedHelmet));

	auto sciFiHelmet = utility::ResourceManager::GetLoadedModel(L"SciFiHelmet");
	sciFiHelmet->GetTransform()->data.translate = { 5.0f, 0.0f, 0.0f };
	mScene->AddModel(std::move(sciFiHelmet));

	auto metalRoughSpheres = utility::ResourceManager::GetLoadedModel(L"MetalRoughSpheres");
	metalRoughSpheres->GetTransform()->data.translate = { -15.0f, 0.0f, 0.0f };
	mScene->AddModel(std::move(metalRoughSpheres));

#if 1
	auto sponza = utility::ResourceManager::GetLoadedModel(L"Sponza");
	sponza->GetTransform()->data.scale = { 0.1f, 0.1f, 0.1f };
	mScene->AddModel(std::move(sponza));
#endif 

	auto skyBox = utility::ResourceManager::GetLoadedSkyBox(L"SkyBox");
	mScene->AddSkyBox(std::move(skyBox));

	editor::LogMessage(L"SandBox data initialized", editor::LogMessageTypes::Info);
	editor::LogMessage(L"Warn Test", editor::LogMessageTypes::Warn);
	editor::LogMessage(L"Warn Error", editor::LogMessageTypes::Error);
}

void SandBox::OnUpdate()
{
	mScene->Update(mAspectRatio);

	mPostProcessBuffer->Update(&mPostProcessBufferData);
}

void SandBox::OnRender()
{
	std::unique_ptr<gfx::GraphicsContext> shadowPassGraphicsContext = mDevice->GetGraphicsContext(mShadowPass->mShadowPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> deferredGPassGraphicsContext = mDevice->GetGraphicsContext(mDeferredGPass->mDeferredPassPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> shadingGraphicsContext = mDevice->GetGraphicsContext(mPBRPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> postProcessingGraphicsContext = mDevice->GetGraphicsContext(mPostProcessingPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> finalGraphicsContext = mDevice->GetGraphicsContext(mFinalPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> finalToSwapChainGraphicsContext = mDevice->GetGraphicsContext(nullptr);

	gfx::BackBuffer *backBuffer = mDevice->GetCurrentBackBuffer();

	mDevice->BeginFrame();

	// Configure offscreen render target's.
	std::array<const gfx::RenderTarget *, 1u> renderTargets
	{
		mOffscreenRT.get()
	};

	static std::array<float, 4> clearColor{0.0f, 0.0f, 0.0f, 1.0f};

	// Renderpass -1 : Shadow pass.
	{
		mShadowPass->Render(mScene.get(), shadowPassGraphicsContext.get());
	}

	// Renderpass 0 : Deferred Geometry pass
	{
		mDeferredGPass->Render(mScene.get(), deferredGPassGraphicsContext.get(), mDepthStencilTexture.get());

		// Copying the depth stencil texture so it can be used for rendering lights
		// (which is forward rendering).
		deferredGPassGraphicsContext->AddResourceBarrier(mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		deferredGPassGraphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);
		deferredGPassGraphicsContext->ExecuteResourceBarriers();

		deferredGPassGraphicsContext->CopyResource(mDepthStencilTexture->GetResource(), mForwardRenderingDepthStencilTexture->GetResource());

		deferredGPassGraphicsContext->AddResourceBarrier( mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		deferredGPassGraphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		deferredGPassGraphicsContext->ExecuteResourceBarriers();
	}

	// RenderPass 1 : Do shading on offscreen RT (deferred lighting pass) and then
	// render lights using forward rendering.
	{
		shadingGraphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		shadingGraphicsContext->ExecuteResourceBarriers();

		shadingGraphicsContext->SetRenderTarget(renderTargets, mDepthStencilTexture.get());
		shadingGraphicsContext->SetDefaultViewportAndScissor();
		shadingGraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		shadingGraphicsContext->ClearRenderTargetView(renderTargets, std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		shadingGraphicsContext->ClearDepthStencilView(mDepthStencilTexture.get(), 1.0f);

		DeferredLightingPassRenderResources deferredLightingPassRenderResources
		{
			.lightBufferIndex = scene::Light::GetCbvIndex(),
			.sceneBufferIndex = mScene->GetSceneBufferIndex(),

			.albedoGBufferIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mDeferredGPass->mDeferredPassRTs.albedoRT.get()),
			.positionEmissiveGBufferIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mDeferredGPass->mDeferredPassRTs.positionEmissiveRT.get()),
			.normalEmissiveGBufferIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mDeferredGPass->mDeferredPassRTs.normalEmissiveRT.get()),
			.aoMetalRoughnessEmissiveGBufferIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mDeferredGPass->mDeferredPassRTs.aoMetalRoughnessEmissiveRT.get()),

			.shadowMappingBufferIndex = gfx::Buffer::GetCbvIndex(mShadowPass->mShadowMappingBuffer.get()),
			.shadowDepthTextureIndex = gfx::Texture::GetSrvIndex(mShadowPass->mDepthTexture.get()),

			.irradianceMapIndex = gfx::Texture::GetSrvIndex(mScene->mSkyBox->mIrradianceMapTexture.get()),
			.prefilterMapIndex = gfx::Texture::GetSrvIndex(mScene->mSkyBox->mPreFilterTexture.get()),
			.brdfLutIndex = gfx::Texture::GetSrvIndex(mScene->mSkyBox->mBRDFLutTexture.get()),
		};

		gfx::RenderTarget::Render(shadingGraphicsContext.get(), deferredLightingPassRenderResources);

		// Render lights and sky box using forward rendering.

		shadingGraphicsContext->SetGraphicsPipelineState(mLightPipelineState.get());
		shadingGraphicsContext->SetRenderTarget(renderTargets, mForwardRenderingDepthStencilTexture.get());

		mScene->RenderLights(shadingGraphicsContext.get());

		shadingGraphicsContext->SetGraphicsPipelineState(mSkyBoxPipelineState.get());

		mScene->RenderSkyBox(shadingGraphicsContext.get());

		shadingGraphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		shadingGraphicsContext->ExecuteResourceBarriers();
	}

	// Render pass 2 : Render offscreen rt to post processed RT (after all
	// processing has occured).
	{
		postProcessingGraphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		postProcessingGraphicsContext->ExecuteResourceBarriers();

		postProcessingGraphicsContext->SetRenderTarget(mPostProcessingRT.get(), mForwardRenderingDepthStencilTexture.get());
		postProcessingGraphicsContext->SetDefaultViewportAndScissor();
		postProcessingGraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		postProcessingGraphicsContext->ClearRenderTargetView(mPostProcessingRT.get(), std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		postProcessingGraphicsContext->ClearDepthStencilView(mForwardRenderingDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render
		// function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mOffscreenRT.get()),
			.postProcessBufferIndex = gfx::Buffer::GetCbvIndex(mPostProcessBuffer.get())
		};

		gfx::RenderTarget::Render(postProcessingGraphicsContext.get(), rtvRenderResources);
	}

	// Render pass 3 : The RT that is to be displayed to swap chain is processed.
	// For now, UI is rendered in this RT as well.
	{
		finalGraphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		finalGraphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		finalGraphicsContext->ExecuteResourceBarriers();

		finalGraphicsContext->SetRenderTarget(mFinalRT.get(), mForwardRenderingDepthStencilTexture.get());
		finalGraphicsContext->SetDefaultViewportAndScissor();
		finalGraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		finalGraphicsContext->ClearDepthStencilView(mForwardRenderingDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render
		// function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = gfx::RenderTarget::GetRenderTextureSRVIndex(mPostProcessingRT.get()),
		};

		gfx::RenderTarget::Render(finalGraphicsContext.get(), rtvRenderResources);

		mEditor->Render(mDevice.get(), mScene.get(), &mDeferredGPass->mDeferredPassRTs, mShadowPass.get(), clearColor, mPostProcessBufferData, mPostProcessingRT.get(), finalGraphicsContext.get());
	}

	// Render pass 3 : Copy the final RT to the swap chain
	{
		finalToSwapChainGraphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		finalToSwapChainGraphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		finalToSwapChainGraphicsContext->ExecuteResourceBarriers();

		finalToSwapChainGraphicsContext->CopyResource(mFinalRT->GetResource(), backBuffer->GetResource());

		finalToSwapChainGraphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		finalToSwapChainGraphicsContext->ExecuteResourceBarriers();
	}

	mDevice->EndFrame();

	std::array<std::unique_ptr<gfx::GraphicsContext>, 6u> graphicsContexts
	{
		std::move(shadowPassGraphicsContext),
		std::move(deferredGPassGraphicsContext),
		std::move(shadingGraphicsContext),		   
		std::move(postProcessingGraphicsContext),
		std::move(finalGraphicsContext),		
		std::move(finalToSwapChainGraphicsContext),
	};

	mDevice->ExecuteContext(graphicsContexts);

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
	gfx::GraphicsPipelineStateCreationDesc postProcessGraphicsPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/RenderPass/PostProcessRenderPassVS.cso",
			.psShaderPath = L"Shaders/RenderPass/PostProcessRenderPassPS.cso",
		},
		.rtvFormats = {gfx::Device::SWAPCHAIN_FORMAT},
		.pipelineName = L"Post Process RenderPass Pipeline"
	};

	mPostProcessingPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(postProcessGraphicsPipelineStateCreationDesc));

	gfx::GraphicsPipelineStateCreationDesc pbrPipelineStateCreationDesc
	{
		.shaderModule
		{
			.vsShaderPath = L"Shaders/Shading/PBRVS.cso",
			.psShaderPath = L"Shaders/Shading/PBRPS.cso",
		},
		.pipelineName = L"PBR Pipeline"
	};

	mPBRPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(pbrPipelineStateCreationDesc));

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
		.rtvFormats = {gfx::Device::SWAPCHAIN_FORMAT},
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
		.rtvFormats = {DXGI_FORMAT_R16G16B16A16_FLOAT},
		.depthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL,
		.frontFaceWindingOrder = gfx::FrontFaceWindingOrder::CounterClockWise,
		.pipelineName = L"Sky Box Pipeline"
	};

	mSkyBoxPipelineState = std::make_unique<gfx::PipelineState>(mDevice->CreatePipelineState(skyBoxPipelineStateCreationDesc));
}
