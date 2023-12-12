#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/TAAPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include "../../../../../../Data/Base/Shaders/Pipeline/CopyConstants.h"
#include "../../../../../../Data/Base/Shaders/Pipeline/TAAConstants.h"

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTAAPass, 1, plRTTIDefaultAllocator<plTAAPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Input", m_PinInputColor),
    PLASMA_MEMBER_PROPERTY("Velocity", m_PinInputVelocity),
    PLASMA_MEMBER_PROPERTY("DepthStencil", m_PinInputDepth),
    PLASMA_MEMBER_PROPERTY("Output", m_PinOutput),
    PLASMA_MEMBER_PROPERTY("UpscaleEnabled", m_bUpsample)->AddAttributes(new plDefaultValueAttribute(false)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


plTAAPass::plTAAPass()
  : plRenderPipelinePass("TAAPass", true)
  , m_bUpsample(false)
{
  // Load shader
  {
    m_hTAAShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/TAA.plShader");
    PLASMA_ASSERT_DEV(m_hTAAShader.IsValid(), "Could not load TAA Pass shader!");

    m_hCopyShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/Copy_CS.plShader");
    PLASMA_ASSERT_DEV(m_hCopyShader.IsValid(), "Could not load the Copy texture shader required for TAA!");
  }

  // Load resources
  {
    m_hTAAConstantBuffer = plRenderContext::CreateConstantBufferStorage<plTAAConstants>();
    m_hCopyConstantBuffer = plRenderContext::CreateConstantBufferStorage<plCopyConstants>();
  }
}

plTAAPass::~plTAAPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hCopyConstantBuffer);
  m_hCopyConstantBuffer.Invalidate();

  plRenderContext::DeleteConstantBufferStorage(m_hTAAConstantBuffer);
  m_hTAAConstantBuffer.Invalidate();

  if (!m_hHistory.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    pDevice->DestroyTexture(m_hHistory);
    m_hHistory.Invalidate();
  }

  if (!m_hPreviousVelocity.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    pDevice->DestroyTexture(m_hPreviousVelocity);
    m_hPreviousVelocity.Invalidate();
  }
}

bool plTAAPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  // Color
  if (inputs[m_PinInputColor.m_uiInputIndex])
  {
    if (!inputs[m_PinInputColor.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Color input must allow shader resource view.", GetName());
      return false;
    }

    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_bAllowUAV = true;
    desc.m_bCreateRenderTarget = true;

    if (m_bUpsample)
    {
      desc.m_uiWidth = plMath::Max(static_cast<plUInt32>(view.GetTargetViewport().width), desc.m_uiWidth);
      desc.m_uiHeight = plMath::Max(static_cast<plUInt32>(view.GetTargetViewport().height), desc.m_uiHeight);
    }

    outputs[m_PinOutput.m_uiOutputIndex] = std::move(desc);
  }
  else
  {
    plLog::Error("No Color input connected to '{0}'!", GetName());
    return false;
  }

  // Velocity
  if (inputs[m_PinInputVelocity.m_uiInputIndex])
  {
    if (!inputs[m_PinInputVelocity.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Velocity input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No Velocity input connected to '{0}'!", GetName());
    return false;
  }

  // Depth - Stencil
  if (inputs[m_PinInputDepth.m_uiInputIndex])
  {
    if (!inputs[m_PinInputDepth.m_uiInputIndex]->m_bAllowShaderResourceView)
    {
      plLog::Error("'{0}' Depth input must allow shader resource view.", GetName());
      return false;
    }
  }
  else
  {
    plLog::Error("No depth/stencil input connected to pass '{0}'.", GetName());
    return false;
  }

  
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputColor.m_uiInputIndex];
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hPreviousVelocity = pDevice->CreateTexture(desc);
  }

  {
    plGALTextureCreationDescription desc = *inputs[m_PinInputVelocity.m_uiInputIndex];
    desc.m_bAllowShaderResourceView = true;
    desc.m_bAllowUAV = true;
    m_hHistory = pDevice->CreateTexture(desc);
  }

  return true;
}

void plTAAPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInputColor = inputs[m_PinInputColor.m_uiInputIndex];
  const auto* const pInputVelocity = inputs[m_PinInputVelocity.m_uiInputIndex];
  const auto* const pInputDepth = inputs[m_PinInputDepth.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInputColor == nullptr || pInputVelocity == nullptr || pInputDepth == nullptr || pOutput == nullptr)
  {
    return;
  }

//  if(!plRenderWorld::GetTAAEnabled())
//  {
//    ExecuteInactive(renderViewContext, inputs, outputs);
//    return;
//  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plResourceManager::ForceLoadResourceNow(m_hTAAShader);

  const bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pPass = pDevice->BeginPass(GetName());
  {
    // TAA Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "TAA");

      renderViewContext.m_pRenderContext->BindShader(m_hTAAShader);

      plGALUnorderedAccessViewHandle hOutput;
      {
        plGALUnorderedAccessViewCreationDescription desc;
        desc.m_hTexture = pOutput->m_TextureHandle;
        desc.m_uiMipLevelToUse = 0;
        hOutput = pDevice->CreateUnorderedAccessView(desc);
      }

      renderViewContext.m_pRenderContext->BindUAV("OutputTexture", hOutput);
      renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInputColor->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("HistoryTexture", pDevice->GetDefaultResourceView(m_hHistory));
      renderViewContext.m_pRenderContext->BindTexture2D("VelocityTexture", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindTexture2D("PreviousVelocityTexture", pDevice->GetDefaultResourceView(m_hPreviousVelocity));
      renderViewContext.m_pRenderContext->BindTexture2D("SceneDepth", pDevice->GetDefaultResourceView(pInputDepth->m_TextureHandle));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plTAAConstants", m_hTAAConstantBuffer);

      const plUInt32 uiWidth = pOutput->m_Desc.m_uiWidth;
      const plUInt32 uiHeight = pOutput->m_Desc.m_uiHeight;

      const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
      const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

      UpdateTAAConstantBuffer();

      renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
    }

    // Accumulate Pass
    {
      auto pCommandEncoder = plRenderContext::BeginComputeScope(pPass, renderViewContext, "Accumulate");

      renderViewContext.m_pRenderContext->BindShader(m_hCopyShader);

      // History
      {
        plGALUnorderedAccessViewHandle hOutput;
        {
          plGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = m_hHistory;
          desc.m_uiMipLevelToUse = 0;
          hOutput = pDevice->CreateUnorderedAccessView(desc);
        }

        renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pOutput->m_TextureHandle));
        renderViewContext.m_pRenderContext->BindConstantBuffer("plCopyConstants", m_hCopyConstantBuffer);
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("COPY_TEXTURE_FILTERING", "COPY_TEXTURE_FILTERING_POINT");

        const plUInt32 uiWidth = pInputColor->m_Desc.m_uiWidth;
        const plUInt32 uiHeight = pInputColor->m_Desc.m_uiHeight;

        const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
        const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

        UpdateCopyConstantBuffer(plVec2I32::ZeroVector(), plVec2U32(uiWidth, uiHeight));

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      }

      // Velocity
      {
        plGALUnorderedAccessViewHandle hOutput;
        {
          plGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = m_hPreviousVelocity;
          desc.m_uiMipLevelToUse = 0;
          hOutput = pDevice->CreateUnorderedAccessView(desc);
        }

        renderViewContext.m_pRenderContext->BindUAV("Output", hOutput);
        renderViewContext.m_pRenderContext->BindTexture2D("InputTexture", pDevice->GetDefaultResourceView(pInputVelocity->m_TextureHandle));
        renderViewContext.m_pRenderContext->BindConstantBuffer("plCopyConstants", m_hCopyConstantBuffer);
        renderViewContext.m_pRenderContext->SetShaderPermutationVariable("COPY_TEXTURE_FILTERING", "COPY_TEXTURE_FILTERING_POINT");

        const plUInt32 uiWidth = pInputVelocity->m_Desc.m_uiWidth;
        const plUInt32 uiHeight = pInputVelocity->m_Desc.m_uiHeight;

        const plUInt32 uiDispatchX = (uiWidth + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;
        const plUInt32 uiDispatchY = (uiHeight + POSTPROCESS_BLOCKSIZE  - 1) / POSTPROCESS_BLOCKSIZE ;

        UpdateCopyConstantBuffer(plVec2I32::ZeroVector(), plVec2U32(uiWidth, uiHeight));

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 1).IgnoreResult();
      }
    }
  }
  pDevice->EndPass(pPass);

  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading);
}

void plTAAPass::ExecuteInactive(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  const auto* const pInput = inputs[m_PinInputColor.m_uiInputIndex];
  const auto* const pOutput = outputs[m_PinOutput.m_uiOutputIndex];

  if (pInput == nullptr || pOutput == nullptr)
  {
    return;
  }

  const plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  const plGALTexture* pDest = pDevice->GetTexture(pOutput->m_TextureHandle);

  if (const plGALTexture* pSource = pDevice->GetTexture(pInput->m_TextureHandle); pDest->GetDescription().m_Format != pSource->GetDescription().m_Format)
  {
    // TODO: use a shader when the format doesn't match exactly

    plLog::Error("Copying textures of different formats is not implemented");
  }
  else
  {
    auto pCommandEncoder = plRenderContext::BeginPassAndComputeScope(renderViewContext, GetName());

    pCommandEncoder->CopyTexture(pOutput->m_TextureHandle, pInput->m_TextureHandle);
  }
}

void plTAAPass::UpdateTAAConstantBuffer() const
{
  auto* constants = plRenderContext::GetConstantBufferData<plTAAConstants>(m_hTAAConstantBuffer);
  constants->UpsampleEnabled = m_bUpsample;
}

void plTAAPass::UpdateCopyConstantBuffer(plVec2I32 offset, plVec2U32 size) const
{
  auto* constants = plRenderContext::GetConstantBufferData<plCopyConstants>(m_hCopyConstantBuffer);
  constants->Offset = offset;
  constants->OutputSize = size;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_TAAPass);
