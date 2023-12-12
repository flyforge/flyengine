#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/OpaqueDeferredGBufferPass.h>
#include <RendererCore/Pipeline/RenderPipeline.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/Textures/Texture2DResource.h>

#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOpaqueDeferredGBufferPass, 1, plRTTIDefaultAllocator<plOpaqueDeferredGBufferPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Albedo", m_PinAlbedo),
    PLASMA_MEMBER_PROPERTY("Normal", m_PinNormal),
    PLASMA_MEMBER_PROPERTY("Material", m_PinMaterial),
    PLASMA_MEMBER_PROPERTY("MaterialAdvanced", m_PinMaterialAdvanced),
    PLASMA_MEMBER_PROPERTY("Velocity", m_PinVelocity),
    PLASMA_MEMBER_PROPERTY("ClearCoat", m_PinClearCoat),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinDepthStencil),
    PLASMA_MEMBER_PROPERTY("WriteDepth", m_bWriteDepth),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plOpaqueDeferredGBufferPass::plOpaqueDeferredGBufferPass(const char* szName)
  : plRenderPipelinePass(szName, true)
  , m_bWriteDepth(false)
{
}

plOpaqueDeferredGBufferPass::~plOpaqueDeferredGBufferPass() = default;

bool plOpaqueDeferredGBufferPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Albedo
  if (inputs[m_PinAlbedo.m_uiInputIndex])
  {
    outputs[m_PinAlbedo.m_uiOutputIndex] = *inputs[m_PinAlbedo.m_uiInputIndex];
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

void plOpaqueDeferredGBufferPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    // Rendering Pass
    {
      // Setup render targets
      plGALRenderingSetup renderingSetup;

      if (inputs[m_PinAlbedo.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(inputs[m_PinAlbedo.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinVelocity.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(1, pDevice->GetDefaultRenderTargetView(inputs[m_PinVelocity.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinNormal.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(2, pDevice->GetDefaultRenderTargetView(inputs[m_PinNormal.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinMaterial.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(3, pDevice->GetDefaultRenderTargetView(inputs[m_PinMaterial.m_uiInputIndex]->m_TextureHandle));
      }


      if (inputs[m_PinMaterialAdvanced.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(4, pDevice->GetDefaultRenderTargetView(inputs[m_PinMaterialAdvanced.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinClearCoat.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetRenderTarget(5, pDevice->GetDefaultRenderTargetView(inputs[m_PinClearCoat.m_uiInputIndex]->m_TextureHandle));
      }

      if (inputs[m_PinDepthStencil.m_uiInputIndex])
      {
        renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(inputs[m_PinDepthStencil.m_uiInputIndex]->m_TextureHandle));
      }

      renderViewContext.m_pRenderContext->BeginRendering(pPass, renderingSetup, renderViewContext.m_pViewData->m_ViewPortRect, "Rendering", renderViewContext.m_pCamera->IsStereoscopic());

      SetupPermutationVars(renderViewContext);

      RenderObjects(renderViewContext);

      renderViewContext.m_pRenderContext->EndRendering();
    }
  }
  pDevice->EndPass(pPass);
}

void plOpaqueDeferredGBufferPass::SetupPermutationVars(const plRenderViewContext& renderViewContext)
{
  plTempHashedString sRenderPass("RENDER_PASS_DEFERRED");
  if (renderViewContext.m_pViewData->m_ViewRenderMode != plViewRenderMode::None)
  {
    sRenderPass = plViewRenderMode::GetPermutationValue(renderViewContext.m_pViewData->m_ViewRenderMode);
  }

  renderViewContext.m_pRenderContext->SetShaderPermutationVariable("RENDER_PASS", sRenderPass);

  plStringBuilder sDebugText;
  plViewRenderMode::GetDebugText(renderViewContext.m_pViewData->m_ViewRenderMode, sDebugText);
  if (!sDebugText.IsEmpty())
  {
    plDebugRenderer::Draw2DText(*renderViewContext.m_pViewDebugContext, sDebugText, plVec2I32(10, 10), plColor::White);
  }

  if (m_bWriteDepth)
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "TRUE");
  }
  else
  {
    renderViewContext.m_pRenderContext->SetShaderPermutationVariable("FORWARD_PASS_WRITE_DEPTH", "FALSE");
  }
}

void plOpaqueDeferredGBufferPass::RenderObjects(const plRenderViewContext& renderViewContext)
{
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitOpaque);
  RenderDataWithCategory(renderViewContext, plDefaultRenderDataCategories::LitMasked);
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_DeferredGBuffer);
