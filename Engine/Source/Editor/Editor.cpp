#include "Pch.hpp"
#include "Editor.hpp"

#include "Core/Application.hpp"

#include "Scene/Scene.hpp"

#include "Graphics/RenderPass/DeferredGeometryPass.hpp"

#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

using namespace DirectX;

namespace helios::editor
{
	Editor::Editor(const gfx::Device* device)
	{
		ImGui_ImplWin32_EnableDpiAwareness();

		// Setup ImGui context.
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		auto iniFilePath = utility::ResourceManager::GetProjectRootDirectory() + L"imgui.ini";
		auto relativeIniFilePath = std::filesystem::relative(iniFilePath);
		mIniFilePath = relativeIniFilePath.string();
		io.IniFilename = mIniFilePath.c_str();

		// Reference : https://github.com/ocornut/imgui/blob/docking/examples/example_win32_directx12/main.cpp

		core::ApplicationLog::AddLog(std::string("Ini file path : ") + std::string(ImGui::GetIO().IniFilename), core::LogMessageTypes::Info);

		io.DisplaySize = ImVec2((float)core::Application::GetClientDimensions().x, (float)core::Application::GetClientDimensions().y);
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
		
		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		// Setup platform / renderer backends.

		ImGui_ImplWin32_Init(core::Application::GetWindowHandle());
		gfx::DescriptorHandle srvDescriptorHandle = device->GetSrvCbvUavDescriptor()->GetCurrentDescriptorHandle();

		ImGui_ImplDX12_Init(device->GetDevice(), gfx::Device::NUMBER_OF_FRAMES, gfx::Device::SWAPCHAIN_FORMAT, device->GetSrvCbvUavDescriptor()->GetDescriptorHeap(), srvDescriptorHandle.cpuDescriptorHandle, srvDescriptorHandle.gpuDescriptorHandle);
		device->GetSrvCbvUavDescriptor()->OffsetCurrentHandle();

        mAssetsPath = utility::ResourceManager::GetAssetPath(L"Assets");
		mContentBrowserCurrentPath = mAssetsPath;
	}

	Editor::~Editor()
	{
		ImGui_ImplDX12_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	// This massive class will do all rendering of UI and its settings / configs within in.
	// May seem like lot of code squashed into a single function, but this makes the engine code clean
	void Editor::Render(gfx::Device* const device, scene::Scene* const scene, gfx::DeferredPassRTs* const deferredPassRTs, gfx::ShadowPass* shadowPass, std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData, const gfx::RenderTarget* renderTarget, gfx::GraphicsContext* graphicsContext)
	{
		if (mShowUI)
		{
			ImGui_ImplDX12_NewFrame();
			ImGui_ImplWin32_NewFrame();
			ImGui::NewFrame();

			ImGui::DockSpaceOverViewport(ImGui::GetWindowViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

			// Render main menu bar
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Helios Editor"))
				{
					ImGui::EndMenu();
				}

				ImGui::EndMainMenuBar();
			}

			ImGui::ShowMetricsWindow();

			// Set clear color & other scene properties.
			RendererProperties(clearColor, postProcessBufferData);

			// Render camera UI.
			RenderSceneProperties(scene);

			// Render scene hierarchy UI.
			RenderSceneHierarchy(scene->mModels);

			// Render Light property menu.
			RenderLightProperties(scene->mLights);

			// Render deferred pass rt's.
			RenderDeferredGPass(device, deferredPassRTs);

			// Render shadow pass data.
			RenderShadowPass(device, shadowPass);

			// Render scene viewport (After all post processing).
			// All add model to model list if a path is dragged into scene viewport.
			RenderSceneViewport(device, renderTarget, scene);

			// Render content browser panel.
			RenderContentBrowser();

			// Render Log Window.
			RenderLogWindow();

			// Render and update handles.
			ImGui::Render();
			ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), graphicsContext->GetCommandList());

			ImGuiIO& io = ImGui::GetIO(); (void)io;
			if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
			{
				ImGui::UpdatePlatformWindows();
				ImGui::RenderPlatformWindowsDefault(core::Application::GetWindowHandle(), graphicsContext->GetCommandList());
			}
		}
	}

	void Editor::RenderSceneHierarchy(std::span<std::unique_ptr<helios::scene::Model>> models) const
	{
		ImGui::Begin("Scene Hierarchy");
		
		for (auto& model : models)
		{
			if (ImGui::TreeNode(WstringToString(model->GetName()).c_str()))
			{
				// Scale uniformally along all axises.
				ImGui::SliderFloat("Scale", &model->GetTransform()->data.scale.x, 0.1f, 15.0f);

				model->GetTransform()->data.scale = math::XMFLOAT3(model->GetTransform()->data.scale.x, model->GetTransform()->data.scale.x, model->GetTransform()->data.scale.x);

				ImGui::SliderFloat3("Translate", &model->GetTransform()->data.translate.x, -10.0f, 10.0f);
				ImGui::SliderFloat3("Rotate", &model->GetTransform()->data.rotation.x, DirectX::XMConvertToRadians(-180.0f), DirectX::XMConvertToRadians(180.0f));

				ImGui::TreePop();
			}
		}		

		ImGui::End();
	}

	void Editor::RenderSceneProperties(scene::Scene* scene) const
	{
		ImGui::Begin("Scene Properties");

		if (ImGui::TreeNode("Primary Camera"))
		{
			// Scale uniformally along all axises.
			ImGui::SliderFloat("Movement Speed", &scene->mCamera->mMovementSpeed, 0.1f, 5000.0f);
			ImGui::SliderFloat("Rotation Speed", &scene->mCamera->mRotationSpeed, 0.1f, 250.0f);

			ImGui::SliderFloat("Friction Factor", &scene->mCamera->mFrictionFactor, 0.0f, 1.0f);
			//	ImGui::SliderFloat("Sun Angle Speed", &scene->mSunChangeSpeed, -100.0f, 50.0f);
			ImGui::TreePop();
		}

		if (ImGui::TreeNode("Projection"))
		{
			ImGui::SliderFloat("FOV", &scene->mFov, 0.5f, 90.0f);
			ImGui::SliderFloat("Near Plane", &scene->mNearPlane, 0.1f, 100.0f);
			ImGui::SliderFloat("Far Plane", &scene->mFarPlane, 10.0f, 10000.0f);

			ImGui::TreePop();
		}

	

		ImGui::End();
	}

	void Editor::RendererProperties(std::span<float, 4> clearColor, PostProcessBuffer& postProcessBufferData) const
	{
		ImGui::Begin("Scene Properties");
		ImGui::ColorPicker3("Clear Color", clearColor.data(), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB);
		ImGui::SliderFloat("Exposure", &postProcessBufferData.exposure, 0.0f, 5.0f);
		ImGui::End();
	}

	void Editor::RenderLightProperties(std::vector<std::unique_ptr<helios::scene::Light>>& lights) const
	{
		ImGui::Begin("Light Properties");

		if (ImGui::TreeNode("Point Lights"))
		{
			// Taking advantage of the fact that point lights come before directional lights (see Common/ConstantBuffers.hlsli for more info).
			for (uint32_t pointLightIndex : std::views::iota(0u, TOTAL_POINT_LIGHTS))
			{
				std::string name = "Point Light " + std::to_string(pointLightIndex);
				if (ImGui::TreeNode(name.c_str()))
				{
					ImGui::ColorPicker3("Light Color", &scene::Light::GetLightBufferData()->lightColor[pointLightIndex].x, ImGuiColorEditFlags_PickerHueWheel);

					ImGui::SliderFloat3("Translate", &scene::Light::GetLightBufferData()->lightPosition[pointLightIndex].x, -40.0f, 40.0f);
					
					ImGui::SliderFloat("Radius", &scene::Light::GetLightBufferData()->radiusIntensity[pointLightIndex].x, 0.01f, 10.0f);

					ImGui::TreePop();
				}
			}

			ImGui::TreePop();
		}


		if (ImGui::TreeNode("Directional Lights"))
		{

			for (uint32_t directionalLightIndex : std::views::iota(DIRECTIONAL_LIGHT_OFFSET, DIRECTIONAL_LIGHT_OFFSET + TOTAL_DIRECTIONAL_LIGHTS))
			{
				std::string name = "Directional Light " + std::to_string(directionalLightIndex);

				DirectX::XMFLOAT4 color = scene::Light::GetLightBufferData()->lightColor[directionalLightIndex];

				if (ImGui::TreeNode(name.c_str()))
				{
					ImGui::ColorPicker3("Light Color", &color.x, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_HDR);
					ImGui::SliderFloat("Intensity", &scene::Light::GetLightBufferData()->radiusIntensity[directionalLightIndex].y, 0.0f, 50.0f);
					float intensity = scene::Light::GetLightBufferData()->radiusIntensity[directionalLightIndex].y;
					 
					scene::Light::GetLightBufferData()->lightColor[directionalLightIndex] =
					{
						color.x,
						color.y,
						color.z,
						color.w
					};

					static float sunAngle{ scene::Light::DIRECTIONAL_LIGHT_ANGLE };
					ImGui::SliderFloat("Sun Angle", &sunAngle, -180.0f, 180.0f);
					scene::Light::GetLightBufferData()->lightPosition[directionalLightIndex] = math::XMFLOAT4(0.0f, sin(math::XMConvertToRadians(sunAngle)), cos(math::XMConvertToRadians(sunAngle)), 0.0f);

					ImGui::TreePop();
				}
			}


			ImGui::TreePop();
		}

		ImGui::End();
	}

	void Editor::RenderDeferredGPass(gfx::Device *const device, const gfx::DeferredPassRTs *deferredRTs) const
	{
		const gfx::DescriptorHandle &albedoEmissiveDescriptorHandle =
			device->GetTextureSrvDescriptorHandle(deferredRTs->albedoRT->renderTexture.get());
		const gfx::DescriptorHandle &positionEmissiveDescriptorHandle =
			device->GetTextureSrvDescriptorHandle(deferredRTs->positionEmissiveRT->renderTexture.get());
		const gfx::DescriptorHandle &normalEmissiveDescriptorHandle =
			device->GetTextureSrvDescriptorHandle(deferredRTs->normalEmissiveRT->renderTexture.get());
		const gfx::DescriptorHandle &aoMetalRoughnessDescriptorHandle =
			device->GetTextureSrvDescriptorHandle(deferredRTs->aoMetalRoughnessEmissiveRT->renderTexture.get());

		ImGui::Begin("Albedo RT");
		ImGui::Image((ImTextureID)(albedoEmissiveDescriptorHandle.cpuDescriptorHandle.ptr),
					 ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();

		// Normal can be negative, which the RT cannot display, hence commenting out this code.
		// ImGui::Begin("Normal RT");
		// ImGui::Image((ImTextureID)(normalDescriptorHandle.cpuDescriptorHandle.ptr),
		// ImGui::GetWindowViewport()->WorkSize); ImGui::End();

		/// Position, ao metal roughness rt's have the emissive component in thier fouth comp (.a / .w), so not
		/// visualizing that either.
		// ImGui::Begin("AO Metal Roughness RT");
		// ImGui::Image((ImTextureID)(aoMetalRoughnessDescriptorHandle.cpuDescriptorHandle.ptr),
		// ImGui::GetWindowViewport()->WorkSize); ImGui::End();

		// ImGui::Begin("Position RT");
		// ImGui::Image((ImTextureID)(positionEmissiveDescriptorHandle.cpuDescriptorHandle.ptr),
		// ImGui::GetWindowViewport()->WorkSize); ImGui::End();
	}

	void Editor::RenderShadowPass(gfx::Device* device, gfx::ShadowPass* shadowPass)
	{
		const gfx::DescriptorHandle& depthTextureDescriptorHandle = device->GetTextureSrvDescriptorHandle(shadowPass->mDepthTexture.get());

		ImGui::Begin("Shadow Pass Depth Texture");
		ImGui::Image((ImTextureID)(depthTextureDescriptorHandle.cpuDescriptorHandle.ptr),
		ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();

		ImGui::Begin("Shadow Buffer Data");
		ImGui::SliderFloat("BackOff Distance", &shadowPass->mShadowMappingBufferData.backOffDistance, 0.1f, 300.0f);
		ImGui::SliderFloat("Extents", &shadowPass->mShadowMappingBufferData.extents, 0.5f, 120.0f);
		ImGui::SliderFloat("Near Plane", &shadowPass->mShadowMappingBufferData.nearPlane, 0.1f, 100.0f);
		ImGui::SliderFloat("Far Plane", &shadowPass->mShadowMappingBufferData.farPlane, 100.0f, 1000.0f);
		ImGui::End();
	}

	// note : Removed, as bloom has not been implemented.
	/*
	void Editor::RenderBloomPass(gfx::Device* device, gfx::BloomPass* bloomPass)
	{
		const gfx::DescriptorHandle& bloomTextureDescriptorHandle = device->GetTextureSrvDescriptorHandle(bloomPass->mPreFilterBloomTexture.get());

		ImGui::Begin("Bloom Texture Pre Filtered");
		ImGui::Image((ImTextureID)(bloomTextureDescriptorHandle.cpuDescriptorHandle.ptr),
		ImGui::GetWindowViewport()->WorkSize);
		ImGui::End();

		ImGui::Begin("Bloom Buffer Data");
		ImGui::SliderFloat("ThresholdValue", &bloomPass->mBloomBufferData.threshHoldParams.x, 0.1f, 10.0f);
		ImGui::SliderFloat("Curve Knee", &bloomPass->mBloomBufferData.threshHoldParams.y, 0.05f, 10.0f);
		ImGui::End();
	}
	*/

	void Editor::RenderLogWindow()
	{
		ImGui::Begin("Console Log");

		bool clear = ImGui::Button("Clear");
		ImGui::SameLine();
		
		if (clear)
		{
			core::ApplicationLog::textBuffer.clear();
			core::ApplicationLog::messageTypes.clear();
		}
		ImGui::Separator();
		
		ImGui::BeginChild("Log Window");

		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
	

		for (uint32_t i : std::views::iota(0u, (uint32_t)(core::ApplicationLog::textBuffer.size())))
		{
			core::LogMessageTypes &messageType = core::ApplicationLog::messageTypes[i];
			std::string &message = core::ApplicationLog::textBuffer[i];

			switch (messageType)
			{
			case core::LogMessageTypes::Info: {
				ImGui::TextColored(ImVec4{1.0f, 1.0f, 1.0f, 1.0f}, "%s\n", message.c_str());
			}
			break;

			case core::LogMessageTypes::Warn: {
				ImGui::TextColored(ImVec4{1.0f, 1.0f, 0.0f, 1.0f}, "%s\n", message.c_str());
			}
			break;

			case core::LogMessageTypes::Error: {
				ImGui::TextColored(ImVec4{1.0f, 0.0f, 0.0f, 1.0f}, "%s\n", message.c_str());
			}
			break;
			}
        }

		ImGui::PopStyleVar();

		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}

		ImGui::EndChild();
		ImGui::End();
	};

	void Editor::RenderSceneViewport(const gfx::Device* device, const gfx::RenderTarget* renderTarget, scene::Scene* scene) const
	{
		const gfx::DescriptorHandle& rtvSrvHandle = device->GetSrvCbvUavDescriptor()->GetDescriptorHandleFromIndex(gfx::RenderTarget::GetRenderTextureSRVIndex(renderTarget));

		ImGui::Begin("View Port");
		ImGui::Image((ImTextureID)(rtvSrvHandle.cpuDescriptorHandle.ptr), ImGui::GetWindowViewport()->WorkSize);

		if (ImGui::BeginDragDropTarget())
		{
			if (const ImGuiPayload* payLoad = ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", ImGuiCond_Once))
			{
				// Check if item path dragged in actually belongs to model. If yes, load the model and add to models vector.
				// If yes, let the model name be the name between final \ and .gltf.
				// note(rtarun9) : the / and \\ are platform specific, preferring windows format for now.
				const wchar_t* modelPath = reinterpret_cast<const wchar_t*>(payLoad->Data);
				if (std::wstring modelPathWStr = modelPath; modelPathWStr.find(L".gltf") != std::wstring::npos || modelPathWStr.find(L".glb") != std::wstring::npos)
				{
					// Needed if two models have same name they end up having same transform.
					static int modelNumber{};
					
					size_t lastSlash = modelPathWStr.find_last_of(L"\\");
					size_t lastDot = modelPathWStr.find_last_of(L".");

					// note(rtarun9) : the +1 and -1 are present to get the exact name (example : \\test.gltf should return test.
					scene::ModelCreationDesc modelCreationDesc
					{
						.modelPath = modelPathWStr,
						.modelName = modelPathWStr.substr(lastSlash + 1, lastDot - lastSlash - 1) + std::to_wstring(modelNumber++),
					};

					scene->AddModel(device, modelCreationDesc);
				}
			}


			ImGui::EndDragDropTarget();
		}

		ImGui::End();
	}

	void Editor::RenderContentBrowser()
	{
		ImGui::Begin("Content Browser");

		// We dont want to allow renderer to see paths other than those of the Assets directory.
		// If the current path is not assets (i.e within one of the assets/ files), show a backbutton, else disable it.
		if (mContentBrowserCurrentPath != mAssetsPath)
		{
			if (ImGui::Button("Back"))
			{
				mContentBrowserCurrentPath = mContentBrowserCurrentPath.parent_path();
			}
		}

		// If a directory entry is a path (folder), let it be a button which on pressing will set the current content browser path to go within the folder.
		// Else, just display the file name as text.
		for (auto& directoryEntry : std::filesystem::directory_iterator(mContentBrowserCurrentPath))
		{
			if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
			{
				// If a asset (in our case, model) is being dragged, payload is set to its path.
				// Null size is + 1 because nulltermination char must also be copied.

				auto itemPath = directoryEntry.path().wstring();
				auto itemPathStr = itemPath.c_str();
				ImGui::SetDragDropPayload("CONTENT_BROWSER_ASSET_ITEM", itemPathStr, (wcslen(itemPathStr) + 1) * sizeof(wchar_t));
				ImGui::EndDragDropSource();
			}
		
			if (directoryEntry.is_directory())
			{
				if (ImGui::Button(directoryEntry.path().string().c_str()))
				{
					// Weird C++ operator for concatanating two paths.
					mContentBrowserCurrentPath /= directoryEntry.path().filename();
				}
			}
			else
			{
				ImGui::TextColored({ 0.9f, 0.9f, 0.8f, 1.0f }, "%s\n", directoryEntry.path().string().c_str());
			}
		}

		ImGui::End();
	}

	void Editor::ShowUI(bool value)
	{
		mShowUI = value;
	}

	void Editor::OnResize(Uint2 dimensions) const
	{
		ImGui::GetMainViewport()->WorkSize = ImVec2((float)dimensions.x, (float)dimensions.y);
		ImGui::GetMainViewport()->Size = ImVec2((float)dimensions.x, (float)dimensions.y);
	}
}
