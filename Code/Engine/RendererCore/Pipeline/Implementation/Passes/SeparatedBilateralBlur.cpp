#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Pipeline/Passes/SeparatedBilateralBlur.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/BilateralBlurConstants.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSeparatedBilateralBlurPass, 2, plRTTIDefaultAllocator<plSeparatedBilateralBlurPass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("BlurSource", m_PinBlurSourceInput),
    PL_MEMBER_PROPERTY("Depth", m_PinDepthInput),
    PL_MEMBER_PROPERTY("Output", m_PinOutput),
    PL_ACCESSOR_PROPERTY("BlurRadius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(7)),
      // Should we really expose that? This gives the user control over the error compared to a perfect gaussian.
      // In theory we could also compute this for a given error from the blur radius. See http://dev.theomader.com/gaussian-kernel-calculator/ for visualization.
    PL_ACCESSOR_PROPERTY("GaussianSigma", GetGaussianSigma, SetGaussianSigma)->AddAttributes(new plDefaultValueAttribute(4.0f)),
    PL_ACCESSOR_PROPERTY("Sharpness", GetSharpness, SetSharpness)->AddAttributes(new plDefaultValueAttribute(120.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSeparatedBilateralBlurPass::plSeparatedBilateralBlurPass()
  : plRenderPipelinePass("SeparatedBilateral")

{
  {
    // Load shader.
    m_hShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/SeparatedBilateralBlur.plShader");
    PL_ASSERT_DEV(m_hShader.IsValid(), "Could not load blur shader!");
  }

  {
    m_hBilateralBlurCB = plRenderContext::CreateConstantBufferStorage<plBilateralBlurConstants>();
  }
}

plSeparatedBilateralBlurPass::~plSeparatedBilateralBlurPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hBilateralBlurCB);
  m_hBilateralBlurCB.Invalidate();
}

bool plSeparatedBilateralBlurPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  PL_ASSERT_DEBUG(inputs.GetCount() == 2, "Unexpected number of inputs for plSeparatedBilateralBlurPass.");

  // Color
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex])
  {
    plLog::Error("No blur target connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    plLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }

  // Depth
  if (!inputs[m_PinDepthInput.m_uiInputIndex])
  {
    plLog::Error("No depth connected to bilateral blur pass!");
    return false;
  }
  if (!inputs[m_PinDepthInput.m_uiInputIndex]->m_bAllowShaderResourceView)
  {
    plLog::Error("All bilateral blur pass inputs must allow shader resoure view.");
    return false;
  }
  if (inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiWidth != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiWidth || inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_uiHeight != inputs[m_PinDepthInput.m_uiInputIndex]->m_uiHeight)
  {
    plLog::Error("Blur target and depth buffer for bilateral blur pass need to have the same dimensions.");
    return false;
  }


  // Output format maches input format.
  outputs[m_PinOutput.m_uiOutputIndex] = *inputs[m_PinBlurSourceInput.m_uiInputIndex];

  return true;
}

void plSeparatedBilateralBlurPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  if (outputs[m_PinOutput.m_uiOutputIndex])
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
    plGALPass* pGALPass = pDevice->BeginPass(GetName());
    PL_SCOPE_EXIT(pDevice->EndPass(pGALPass));

    // Setup input view and sampler
    plGALResourceViewCreationDescription rvcd;
    rvcd.m_hTexture = inputs[m_PinBlurSourceInput.m_uiInputIndex]->m_TextureHandle;
    plGALResourceViewHandle hBlurSourceInputView = plGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);
    rvcd.m_hTexture = inputs[m_PinDepthInput.m_uiInputIndex]->m_TextureHandle;
    plGALResourceViewHandle hDepthInputView = plGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    // Get temp texture for horizontal target / vertical source.
    plGALTextureCreationDescription tempTextureDesc = outputs[m_PinBlurSourceInput.m_uiInputIndex]->m_Desc;
    tempTextureDesc.m_bAllowShaderResourceView = true;
    tempTextureDesc.m_bCreateRenderTarget = true;
    plGALTextureHandle tempTexture = plGPUResourcePool::GetDefaultInstance()->GetRenderTarget(tempTextureDesc);
    rvcd.m_hTexture = tempTexture;
    plGALResourceViewHandle hTempTextureRView = plGALDevice::GetDefaultDevice()->CreateResourceView(rvcd);

    plGALRenderingSetup renderingSetup;

    // Bind shader and inputs
    renderViewContext.m_pRenderContext->BindShader(m_hShader);
    renderViewContext.m_pRenderContext->BindMeshBuffer(plGALBufferHandle(), plGALBufferHandle(), nullptr, plGALPrimitiveTopology::Triangles, 1);
    renderViewContext.m_pRenderContext->BindTexture2D("DepthBuffer", hDepthInputView);
    renderViewContext.m_pRenderContext->BindConstantBuffer("plBilateralBlurConstants", m_hBilateralBlurCB);

    // Horizontal
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(tempTexture));
      auto pCommandEncoder = plRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_HORIZONTAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hBlurSourceInputView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Vertical
    {
      renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(outputs[m_PinOutput.m_uiOutputIndex]->m_TextureHandle));
      auto pCommandEncoder = plRenderContext::BeginRenderingScope(pGALPass, renderViewContext, renderingSetup, "", renderViewContext.m_pCamera->IsStereoscopic());

      renderViewContext.m_pRenderContext->SetShaderPermutationVariable("BLUR_DIRECTION", "BLUR_DIRECTION_VERTICAL");
      renderViewContext.m_pRenderContext->BindTexture2D("BlurSource", hTempTextureRView);
      renderViewContext.m_pRenderContext->DrawMeshBuffer().IgnoreResult();
    }

    // Give back temp texture.
    plGPUResourcePool::GetDefaultInstance()->ReturnRenderTarget(tempTexture);
  }
}

plResult plSeparatedBilateralBlurPass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_uiRadius;
  inout_stream << m_fGaussianSigma;
  inout_stream << m_fSharpness;
  return PL_SUCCESS;
}

plResult plSeparatedBilateralBlurPass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_uiRadius;
  inout_stream >> m_fGaussianSigma;
  inout_stream >> m_fSharpness;
  return PL_SUCCESS;
}

void plSeparatedBilateralBlurPass::SetRadius(plUInt32 uiRadius)
{
  m_uiRadius = uiRadius;

  plBilateralBlurConstants* cb = plRenderContext::GetConstantBufferData<plBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->BlurRadius = m_uiRadius;
}

plUInt32 plSeparatedBilateralBlurPass::GetRadius() const
{
  return m_uiRadius;
}

void plSeparatedBilateralBlurPass::SetGaussianSigma(const float fSigma)
{
  m_fGaussianSigma = fSigma;

  plBilateralBlurConstants* cb = plRenderContext::GetConstantBufferData<plBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->GaussianFalloff = 1.0f / (2.0f * m_fGaussianSigma * m_fGaussianSigma);
}

float plSeparatedBilateralBlurPass::GetGaussianSigma() const
{
  return m_fGaussianSigma;
}

void plSeparatedBilateralBlurPass::SetSharpness(const float fSharpness)
{
  m_fSharpness = fSharpness;

  plBilateralBlurConstants* cb = plRenderContext::GetConstantBufferData<plBilateralBlurConstants>(m_hBilateralBlurCB);
  cb->Sharpness = m_fSharpness;
}

float plSeparatedBilateralBlurPass::GetSharpness() const
{
  return m_fSharpness;
}



//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

class plSeparatedBilateralBlurPassPatch_1_2 : public plGraphPatch
{
public:
  plSeparatedBilateralBlurPassPatch_1_2()
    : plGraphPatch("plSeparatedBilateralBlurPass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Blur Radius", "BlurRadius");
    pNode->RenameProperty("Gaussian Standard Deviation", "GaussianSigma");
    pNode->RenameProperty("Bilateral Sharpness", "Sharpness");
  }
};

plSeparatedBilateralBlurPassPatch_1_2 g_plSeparatedBilateralBlurPassPatch_1_2;



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SeparatedBilateralBlur);
