#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/OpaqueDeferredLightingPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOpaqueDeferredLightingPass, 1, plRTTIDefaultAllocator<plOpaqueDeferredLightingPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("Albedo", m_PinAlbedo),
    PLASMA_MEMBER_PROPERTY("Normal", m_PinNormal),
    PLASMA_MEMBER_PROPERTY("Material", m_PinMaterial),
    PLASMA_MEMBER_PROPERTY("MaterialAdvanced", m_PinMaterialAdvanced),
    PLASMA_MEMBER_PROPERTY("Velocity", m_PinVelocity),
    PLASMA_MEMBER_PROPERTY("ClearCoat", m_PinClearCoat),
    PLASMA_MEMBER_PROPERTY("SSAO", m_PinSSAO),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plOpaqueDeferredLightingPass::plOpaqueDeferredLightingPass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
  // Load shaders
  {
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/DeferredLightning.plShader");
    PLASMA_ASSERT_DEV(m_hShader.IsValid(), "Could not load DeferredLightning shader!");
  }
}

plOpaqueDeferredLightingPass::~plOpaqueDeferredLightingPass() = default;

bool plOpaqueDeferredLightingPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Albedo
  if (inputs[m_PinAlbedo.m_uiInputIndex])
  {
    outputs[m_PinAlbedo.m_uiOutputIndex] = *inputs[m_PinAlbedo.m_uiInputIndex];

    plGALTextureCreationDescription desc = *inputs[m_PinAlbedo.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_Format = plGALResourceFormat::RGBAHalf;
    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }
  else
  {
    plLog::Error("No albedo input connected to pass '{0}'.", GetName());
    return false;
  }

  // Normal
  if (inputs[m_PinNormal.m_uiInputIndex])
  {
    outputs[m_PinNormal.m_uiOutputIndex] = *inputs[m_PinNormal.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No normal input connected to pass '{0}'.", GetName());
    return false;
  }

  // Material
  if (inputs[m_PinMaterial.m_uiInputIndex])
  {
    outputs[m_PinMaterial.m_uiOutputIndex] = *inputs[m_PinMaterial.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No material input connected to pass '{0}'.", GetName());
    return false;
  }

   // Specular
  if (inputs[m_PinMaterialAdvanced.m_uiInputIndex])
  {
    outputs[m_PinMaterialAdvanced.m_uiOutputIndex] = *inputs[m_PinMaterialAdvanced.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No advanced material input connected to pass '{0}'.", GetName());
    return false;
  }

  // Velocity
  if (inputs[m_PinVelocity.m_uiInputIndex])
  {
    outputs[m_PinVelocity.m_uiOutputIndex] = *inputs[m_PinVelocity.m_uiInputIndex];
  }

  // Clearcoat
  if (inputs[m_PinClearCoat.m_uiInputIndex])
  {
    outputs[m_PinClearCoat.m_uiOutputIndex] = *inputs[m_PinClearCoat.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No clear coat input connected to pass '{0}'.", GetName());
    return false;
  }

  // Depth - Stencil
  if (inputs[m_PinDepthStencil.m_uiInputIndex])
  {
    outputs[m_PinDepthStencil.m_uiOutputIndex] = *inputs[m_PinDepthStencil.m_uiInputIndex];
  }
  else
  {
    plLog::Error("No depth/stencil input connected to pass '{0}'.", GetName());
    return false;
  }

  return true;
}

void plOpaqueDeferredLightingPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    // Lightning Pass
    if (const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex]; pOutput != nullptr)
    {
      // Setup render targets
      plGALRenderingSetup renderingSetup;
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));

      renderViewContext.m_pRenderContext->BeginRendering(pPass, renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, "Lightning", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->BindShader(m_hShader);

      if (inputs[m_PinAlbedo.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("AlbedoTexture", pDevice->GetDefaultResourceView(inputs[m_PinAlbedo.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinNormal.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("NormalTexture", pDevice->GetDefaultResourceView(inputs[m_PinNormal.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinMaterial.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("MaterialTexture", pDevice->GetDefaultResourceView(inputs[m_PinMaterial.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinMaterial.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("MaterialAdvancedTexture", pDevice->GetDefaultResourceView(inputs[m_PinMaterialAdvanced.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinVelocity.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("VelocityTexture", pDevice->GetDefaultResourceView(inputs[m_PinVelocity.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinClearCoat.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("ClearCoatTexture", pDevice->GetDefaultResourceView(inputs[m_PinClearCoat.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinSSAO.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("SSAOTexture", pDevice->GetDefaultResourceView(inputs[m_PinSSAO.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinDepthStencil.m_uiInputIndex])
      {
        renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", pDevice->GetDefaultResourceView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
      }

      const auto pClusteredData = GetPipeline()->GetFrameDataProvider<plClusteredDataProvider>()->GetData(renderViewContext);
      pClusteredData->BindResources(renderViewContext.m_pRenderContext);

      renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);

      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }
  pDevice->EndPass(pPass);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DeferredLightingPass);
