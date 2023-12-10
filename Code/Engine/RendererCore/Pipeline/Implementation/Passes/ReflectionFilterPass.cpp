#include <RendererCore/RendererCorePCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/Passes/ReflectionFilterPass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionFilteredSpecularConstants.h>
#include <RendererCore/../../../Data/Base/Shaders/Pipeline/ReflectionIrradianceConstants.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plReflectionFilterPass, 1, plRTTIDefaultAllocator<plReflectionFilterPass>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("FilteredSpecular", m_PinFilteredSpecular),
    PLASMA_MEMBER_PROPERTY("AvgLuminance", m_PinAvgLuminance),
    PLASMA_MEMBER_PROPERTY("IrradianceData", m_PinIrradianceData),
    PLASMA_MEMBER_PROPERTY("Intensity", m_fIntensity)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Saturation", m_fSaturation)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("SpecularOutputIndex", m_uiSpecularOutputIndex),
    PLASMA_MEMBER_PROPERTY("IrradianceOutputIndex", m_uiIrradianceOutputIndex),
    PLASMA_ACCESSOR_PROPERTY("InputCubemap", GetInputCubemap, SetInputCubemap)
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plReflectionFilterPass::plReflectionFilterPass()
  : plRenderPipelinePass("ReflectionFilterPass")

{
  {
    m_hFilteredSpecularConstantBuffer = plRenderContext::CreateConstantBufferStorage<plReflectionFilteredSpecularConstants>();
    m_hFilteredSpecularShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/ReflectionFilteredSpecular.plShader");
    PLASMA_ASSERT_DEV(m_hFilteredSpecularShader.IsValid(), "Could not load ReflectionFilteredSpecular shader!");

    m_hIrradianceConstantBuffer = plRenderContext::CreateConstantBufferStorage<plReflectionIrradianceConstants>();
    m_hIrradianceShader = plResourceManager::LoadResource<plShaderResource>("Shaders/Pipeline/ReflectionIrradiance.plShader");
    PLASMA_ASSERT_DEV(m_hIrradianceShader.IsValid(), "Could not load ReflectionIrradiance shader!");
  }
}

plReflectionFilterPass::~plReflectionFilterPass()
{
  plRenderContext::DeleteConstantBufferStorage(m_hIrradianceConstantBuffer);
  m_hIrradianceConstantBuffer.Invalidate();
}

bool plReflectionFilterPass::GetRenderTargetDescriptions(const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  {
    plGALTextureCreationDescription desc;
    desc.m_uiWidth = plReflectionPool::GetReflectionCubeMapSize();
    desc.m_uiHeight = desc.m_uiWidth;
    desc.m_Format = plGALResourceFormat::RGBAHalf;
    desc.m_Type = plGALTextureType::TextureCube;
    desc.m_bAllowUAV = true;
    desc.m_uiMipLevelCount = plMath::Log2i(desc.m_uiWidth) - 1;
    outputs[m_PinFilteredSpecular.m_uiOutputIndex] = desc;
  }

  return true;
}

void plReflectionFilterPass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs, const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  auto pInputCubemap = pDevice->GetTexture(m_hInputCubemap);
  if (pInputCubemap == nullptr)
  {
    return;
  }

  // We cannot allow the filter to work on fallback resources as the step will not be repeated for static cube maps. Thus, we force loading the shaders and disable async shader loading in this scope.
  plResourceManager::ForceLoadResourceNow(m_hFilteredSpecularShader);
  plResourceManager::ForceLoadResourceNow(m_hIrradianceShader);
  bool bAllowAsyncShaderLoading = renderViewContext.m_pRenderContext->GetAllowAsyncShaderLoading();
  renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(false);

  plGALPass* pGALPass = pDevice->BeginPass(GetName());
  PLASMA_SCOPE_EXIT(
    pDevice->EndPass(pGALPass);
    renderViewContext.m_pRenderContext->SetAllowAsyncShaderLoading(bAllowAsyncShaderLoading));

  if (pInputCubemap->GetDescription().m_bAllowDynamicMipGeneration)
  {
    auto pCommandEncoder = plRenderContext::BeginRenderingScope(pGALPass, renderViewContext, plGALRenderingSetup(), "MipMaps");
    pCommandEncoder->GenerateMipMaps(pDevice->GetDefaultResourceView(m_hInputCubemap));
  }

  {
    auto pFilteredSpecularOutput = outputs[m_PinFilteredSpecular.m_uiOutputIndex];
    if (pFilteredSpecularOutput != nullptr && !pFilteredSpecularOutput->m_TextureHandle.IsInvalidated())
    {
      plUInt32 uiNumMipMaps = pFilteredSpecularOutput->m_Desc.m_uiMipLevelCount;

      plUInt32 uiWidth = pFilteredSpecularOutput->m_Desc.m_uiWidth;
      plUInt32 uiHeight = pFilteredSpecularOutput->m_Desc.m_uiHeight;

      auto pCommandEncoder = plRenderContext::BeginComputeScope(pGALPass, renderViewContext, "ReflectionFilter");
      renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));
      renderViewContext.m_pRenderContext->BindConstantBuffer("plReflectionFilteredSpecularConstants", m_hFilteredSpecularConstantBuffer);
      renderViewContext.m_pRenderContext->BindShader(m_hFilteredSpecularShader);

      for (plUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        plGALUnorderedAccessViewHandle hFilterOutput;
        {
          plGALUnorderedAccessViewCreationDescription desc;
          desc.m_hTexture = pFilteredSpecularOutput->m_TextureHandle;
          desc.m_uiMipLevelToUse = uiMipMapIndex;
          desc.m_uiFirstArraySlice = m_uiSpecularOutputIndex * 6;
          desc.m_uiArraySize = 6;
          hFilterOutput = pDevice->CreateUnorderedAccessView(desc);
        }
        renderViewContext.m_pRenderContext->BindUAV("ReflectionOutput", hFilterOutput);
        UpdateFilteredSpecularConstantBuffer(uiMipMapIndex, uiNumMipMaps);

        constexpr plUInt32 uiThreadsX = 8;
        constexpr plUInt32 uiThreadsY = 8;
        const plUInt32 uiDispatchX = (uiWidth + uiThreadsX - 1) / uiThreadsX;
        const plUInt32 uiDispatchY = (uiHeight + uiThreadsY - 1) / uiThreadsY;

        renderViewContext.m_pRenderContext->Dispatch(uiDispatchX, uiDispatchY, 6).IgnoreResult();

        uiWidth >>= 1;
        uiHeight >>= 1;
      }
    }
  }

  auto pIrradianceOutput = outputs[m_PinIrradianceData.m_uiOutputIndex];
  if (pIrradianceOutput != nullptr && !pIrradianceOutput->m_TextureHandle.IsInvalidated())
  {
    auto pCommandEncoder = plRenderContext::BeginComputeScope(pGALPass, renderViewContext, "Irradiance");

    plGALUnorderedAccessViewHandle hIrradianceOutput;
    {
      plGALUnorderedAccessViewCreationDescription desc;
      desc.m_hTexture = pIrradianceOutput->m_TextureHandle;

      hIrradianceOutput = pDevice->CreateUnorderedAccessView(desc);
    }
    renderViewContext.m_pRenderContext->BindUAV("IrradianceOutput", hIrradianceOutput);

    renderViewContext.m_pRenderContext->BindTextureCube("InputCubemap", pDevice->GetDefaultResourceView(m_hInputCubemap));

    UpdateIrradianceConstantBuffer();

    renderViewContext.m_pRenderContext->BindConstantBuffer("plReflectionIrradianceConstants", m_hIrradianceConstantBuffer);
    renderViewContext.m_pRenderContext->BindShader(m_hIrradianceShader);

    renderViewContext.m_pRenderContext->Dispatch(1).IgnoreResult();
  }
}

plResult plReflectionFilterPass::Serialize(plStreamWriter& inout_stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_fIntensity;
  inout_stream << m_fSaturation;
  inout_stream << m_uiSpecularOutputIndex;
  inout_stream << m_uiIrradianceOutputIndex;
  // inout_stream << m_hInputCubemap; Runtime only property
  return PLASMA_SUCCESS;
}

plResult plReflectionFilterPass::Deserialize(plStreamReader& inout_stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PLASMA_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_fIntensity;
  inout_stream >> m_fSaturation;
  inout_stream >> m_uiSpecularOutputIndex;
  inout_stream >> m_uiIrradianceOutputIndex;
  return PLASMA_SUCCESS;
}

plUInt32 plReflectionFilterPass::GetInputCubemap() const
{
  return m_hInputCubemap.GetInternalID().m_Data;
}

void plReflectionFilterPass::SetInputCubemap(plUInt32 uiCubemapHandle)
{
  m_hInputCubemap = plGALTextureHandle(plGAL::pl18_14Id(uiCubemapHandle));
}

void plReflectionFilterPass::UpdateFilteredSpecularConstantBuffer(plUInt32 uiMipMapIndex, plUInt32 uiNumMipMaps)
{
  auto constants = plRenderContext::GetConstantBufferData<plReflectionFilteredSpecularConstants>(m_hFilteredSpecularConstantBuffer);
  constants->MipLevel = uiMipMapIndex;
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
}

void plReflectionFilterPass::UpdateIrradianceConstantBuffer()
{
  auto constants = plRenderContext::GetConstantBufferData<plReflectionIrradianceConstants>(m_hIrradianceConstantBuffer);
  constants->LodLevel = 6; // TODO: calculate from cubemap size and number of samples
  constants->Intensity = m_fIntensity;
  constants->Saturation = m_fSaturation;
  constants->OutputIndex = m_uiIrradianceOutputIndex;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_ReflectionFilterPass);
