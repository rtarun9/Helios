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

#if 0
	scene::ModelCreationDesc cubeCreationDesc
	{
		.modelPath = L"Assets/Models/Cube/glTF/Cube.gltf",
		.modelName = L"Cube",
	};
	
	auto cube = std::make_unique<scene::Model>(mDevice.get(), cubeCreationDesc);
	cube->GetTransform()->data.translate = { 0.0f, 5.0f, 0.0f };
	mScene->AddModel(std::move(cube));
#endif

	scene::ModelCreationDesc sphereCreationDesc
	{
		.modelPath = L"Assets/Models/Sphere/scene.gltf",
		.modelName = L"Sphere",
	};

	auto sphere = std::make_unique<scene::Model>(mDevice.get(), sphereCreationDesc);
	sphere->GetTransform()->data.translate = { 0.0f, 5.0f, 0.0f };
	mScene->AddModel(std::move(sphere));

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

#if 0
	scene::ModelCreationDesc sponzaCreationDesc
	{
		.modelPath = L"Assets/Models/Sponza/glTF/Sponza.gltf",
		.modelName = L"Sponza Scene",
	};

	auto sponza = std::make_unique<scene::Model>(mDevice.get(), sponzaCreationDesc);
	sponza->GetTransform()->data.scale = { 0.1f, 0.1f, 0.1f };
	mScene->AddModel(std::move(sponza));
#endif

	// Load lights.

	for (uint32_t i : std::views::iota(0u, TOTAL_POINT_LIGHTS))
	{
		scene::LightCreationDesc pointLightcreationDesc2
		{
			.lightNumber = i,
			.lightType = scene::LightTypes::PointLightData
		};

		mScene->AddLight(mDevice.get(), pointLightcreationDesc2);


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
	std::unique_ptr<gfx::GraphicsContext> deferredGPassGraphicsContext = mDevice->GetGraphicsContext(mDeferredGPass->mDeferredPassPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> shadingGraphicsContext = mDevice->GetGraphicsContext(mPBRPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> postProcessingGraphicsContext = mDevice->GetGraphicsContext(mPostProcessingPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> finalGraphicsContext = mDevice->GetGraphicsContext(mFinalPipelineState.get());
	std::unique_ptr<gfx::GraphicsContext> finalToSwapChainGraphicsContext = mDevice->GetGraphicsContext(nullptr);

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
		mDeferredGPass->Render(mScene.get(), deferredGPassGraphicsContext.get(), mDepthStencilTexture.get());

		// Copying the depth stencil texture so it can be used for rendering lights (which is forward rendering).
		deferredGPassGraphicsContext->AddResourceBarrier(mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_SOURCE);
		deferredGPassGraphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_DEPTH_WRITE, D3D12_RESOURCE_STATE_COPY_DEST);
		deferredGPassGraphicsContext->ExecuteResourceBarriers();

		deferredGPassGraphicsContext->CopyResource(mDepthStencilTexture->GetResource(), mForwardRenderingDepthStencilTexture->GetResource());

		deferredGPassGraphicsContext->AddResourceBarrier(mDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		deferredGPassGraphicsContext->AddResourceBarrier(mForwardRenderingDepthStencilTexture->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_DEPTH_WRITE);

		deferredGPassGraphicsContext->ExecuteResourceBarriers();
	}

	// RenderPass 1 : Do shading on offscreen RT (deferred lighting pass) and then render lights using forward rendering.
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
			.sceneBufferIndex = mScene->mSceneBuffer->cbvIndex,

			.albedoGBufferIndex = mDeferredGPass->mDeferredPassRTs.albedoRT->GetRenderTextureSRVIndex(),
			.positionEmissiveGBufferIndex = mDeferredGPass->mDeferredPassRTs.positionEmissiveRT->GetRenderTextureSRVIndex(),
			.normalEmissiveGBufferIndex = mDeferredGPass->mDeferredPassRTs.normalEmissiveRT->GetRenderTextureSRVIndex(),
			.aoMetalRoughnessEmissiveGBufferIndex = mDeferredGPass->mDeferredPassRTs.aoMetalRoughnessEmissiveRT->GetRenderTextureSRVIndex(),
		};

		gfx::RenderTarget::Render(shadingGraphicsContext.get(), deferredLightingPassRenderResources);

		// Render lights and skybox using forward rendering.

		shadingGraphicsContext->SetGraphicsPipelineState(mLightPipelineState.get());
		shadingGraphicsContext->SetRenderTarget(renderTargets, mForwardRenderingDepthStencilTexture.get());

		mScene->RenderLights(shadingGraphicsContext.get());

		shadingGraphicsContext->SetGraphicsPipelineState(mSkyBoxPipelineState.get());

		mScene->RenderSkyBox(shadingGraphicsContext.get());

		shadingGraphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		shadingGraphicsContext->ExecuteResourceBarriers();
	}

	// Render pass 2 : Render offscreen rt to post processed RT (after all processing has occured).
	{
		postProcessingGraphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		postProcessingGraphicsContext->ExecuteResourceBarriers();

		postProcessingGraphicsContext->SetRenderTarget(mPostProcessingRT.get(), mForwardRenderingDepthStencilTexture.get());
		postProcessingGraphicsContext->SetDefaultViewportAndScissor();
		postProcessingGraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		postProcessingGraphicsContext->ClearRenderTargetView(mPostProcessingRT.get(), std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f});
		postProcessingGraphicsContext->ClearDepthStencilView(mForwardRenderingDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = mOffscreenRT->renderTexture->srvIndex,
			.postProcessBufferIndex = mPostProcessBuffer->cbvIndex
		};

		gfx::RenderTarget::Render(postProcessingGraphicsContext.get(), rtvRenderResources);
	}

	// Render pass 3 : The RT that is to be displayed to swapchain is processed. For now, UI is rendered in this RT as well.
	{
		finalGraphicsContext->AddResourceBarrier(mPostProcessingRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
		finalGraphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
		finalGraphicsContext->ExecuteResourceBarriers();

		finalGraphicsContext->SetRenderTarget(mFinalRT.get(), mForwardRenderingDepthStencilTexture.get());
		finalGraphicsContext->SetDefaultViewportAndScissor();
		finalGraphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		finalGraphicsContext->ClearDepthStencilView(mForwardRenderingDepthStencilTexture.get(), 1.0f);

		// Note : buffer indices can be set here or in the RenderTarget::Render function. Begin done there for now.
		RenderTargetRenderResources rtvRenderResources
		{
			.textureIndex = mPostProcessingRT->renderTexture->srvIndex,
		};

		gfx::RenderTarget::Render(finalGraphicsContext.get(), rtvRenderResources);

		mEditor->Render(mDevice.get(), mScene.get(), &mDeferredGPass->mDeferredPassRTs, clearColor, mPostProcessBufferData, mPostProcessingRT.get(), finalGraphicsContext.get());
	}

	// Render pass 3 : Copy the final RT to the swapchain
	{
		finalToSwapChainGraphicsContext->AddResourceBarrier(mFinalRT->GetResource(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE);
		finalToSwapChainGraphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST);
		finalToSwapChainGraphicsContext->ExecuteResourceBarriers();

		finalToSwapChainGraphicsContext->CopyResource(mFinalRT->GetResource(), backBuffer->GetResource());

		finalToSwapChainGraphicsContext->AddResourceBarrier(backBuffer->GetResource(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT);
		finalToSwapChainGraphicsContext->ExecuteResourceBarriers();
	}


	mDevice->EndFrame();

	std::array<gfx::GraphicsContext*, 5> graphicsContexts
	{
		deferredGPassGraphicsContext.get(),
		shadingGraphicsContext.get(),
		postProcessingGraphicsContext.get(),
		finalGraphicsContext.get(),
		finalToSwapChainGraphicsContext.get(),
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
	// Load pipeline states.

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
