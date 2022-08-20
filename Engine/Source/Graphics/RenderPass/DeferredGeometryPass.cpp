

#include "DeferredGeometryPass.hpp"

#include "../API/Device.hpp"

// For reference :
// float4 albedo : SV_Target0;
// float4 positionEmissive : SV_Target1;
// float4 normalEmissive : SV_Target2;
// float4 aoMetalRoughnessEmissive : SV_Target3;

namespace helios::gfx
{
    DeferredGeometryPass::DeferredGeometryPass(const gfx::Device *device, const Uint2 &dimensions)
    {
        // Create pipeline state.

        gfx::GraphicsPipelineStateCreationDesc deferredPassPipelineStateCreationDesc{
            .shaderModule{
                .vsShaderPath = (L"Shaders/RenderPass/DeferredGeometryPassVS.cso"),
                .psShaderPath = (L"Shaders/RenderPass/DeferredGeometryPassPS.cso"),
            },
            .rtvFormats = {DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_FORMAT_R16G16B16A16_FLOAT, DXGI_FORMAT_R16G16B16A16_FLOAT,
                           DXGI_FORMAT_R16G16B16A16_FLOAT},
            .rtvCount = 4,
            .pipelineName = L"Deferred Geometry Pass Pipeline"};

        mDeferredPassPipelineState =
            std::make_unique<gfx::PipelineState>(device->GetDevice(), deferredPassPipelineStateCreationDesc);

        // Create MRT's for GPass.

        gfx::TextureCreationDesc albedoRenderTargetTextureCreationDesc{.usage = gfx::TextureUsage::RenderTarget,
                                                                       .dimensions = dimensions,
                                                                       .format = DXGI_FORMAT_R8G8B8A8_UNORM,
                                                                       .name = L"Deferred Pass Albedo Texture"};

        mDeferredPassRTs.albedoRT =
            std::make_unique<gfx::RenderTarget>(device->CreateRenderTarget(albedoRenderTargetTextureCreationDesc));

        gfx::TextureCreationDesc positionEmissiveRenderTargetTextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .dimensions = dimensions,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .name = L"Deferred Pass Position Emissive Texture"};

        mDeferredPassRTs.positionEmissiveRT = std::make_unique<gfx::RenderTarget>(
            device->CreateRenderTarget(positionEmissiveRenderTargetTextureCreationDesc));

        gfx::TextureCreationDesc normalEmissiveRenderTargetTextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .dimensions = dimensions,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .name = L"Deferred Pass Normal Emissive Texture"};

        mDeferredPassRTs.normalEmissiveRT = std::make_unique<gfx::RenderTarget>(
            device->CreateRenderTarget(normalEmissiveRenderTargetTextureCreationDesc));

        gfx::TextureCreationDesc aoMetalRoughnessEmissiveRenderTargetTextureCreationDesc{
            .usage = gfx::TextureUsage::RenderTarget,
            .dimensions = dimensions,
            .format = DXGI_FORMAT_R16G16B16A16_FLOAT,
            .name = L"Deferred Pass AO Metal Roughness Emissive Texture"};

        mDeferredPassRTs.aoMetalRoughnessEmissiveRT = std::make_unique<gfx::RenderTarget>(
            device->CreateRenderTarget(aoMetalRoughnessEmissiveRenderTargetTextureCreationDesc));
    }

    void DeferredGeometryPass::Render(scene::Scene *scene, gfx::GraphicsContext *graphicsContext,
                                      gfx::Texture *depthBuffer)
    {
        std::array<const gfx::RenderTarget *, 4u> renderTargets{
            mDeferredPassRTs.albedoRT.get(),
            mDeferredPassRTs.positionEmissiveRT.get(),
            mDeferredPassRTs.normalEmissiveRT.get(),
            mDeferredPassRTs.aoMetalRoughnessEmissiveRT.get(),
        };

        graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                                            D3D12_RESOURCE_STATE_RENDER_TARGET);
        graphicsContext->ExecuteResourceBarriers();

        graphicsContext->SetGraphicsPipelineState(mDeferredPassPipelineState.get());
        graphicsContext->SetRenderTarget(renderTargets, depthBuffer);
        graphicsContext->SetDefaultViewportAndScissor();
        graphicsContext->SetPrimitiveTopologyLayout(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        graphicsContext->ClearRenderTargetView(renderTargets, std::array<float, 4u>{0.0f, 0.0f, 0.0f, 1.0f});
        graphicsContext->ClearDepthStencilView(depthBuffer, 1.0f);

        scene->RenderModels(graphicsContext);

        graphicsContext->AddResourceBarrier(renderTargets, D3D12_RESOURCE_STATE_RENDER_TARGET,
                                            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        graphicsContext->ExecuteResourceBarriers();
    }

    void DeferredGeometryPass::Resize(gfx::Device *device, const Uint2 &dimensions)
    {
        device->ResizeRenderTarget(mDeferredPassRTs.albedoRT.get());
        device->ResizeRenderTarget(mDeferredPassRTs.positionEmissiveRT.get());
        device->ResizeRenderTarget(mDeferredPassRTs.normalEmissiveRT.get());
        device->ResizeRenderTarget(mDeferredPassRTs.aoMetalRoughnessEmissiveRT.get());
    }
} // namespace helios::gfx
