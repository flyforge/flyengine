#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Foundation/IO/TypeVersionContext.h>
#include <RendererCore/Pipeline/Passes/SourcePass.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSourcePass, 3, plRTTIDefaultAllocator<plSourcePass>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Output", m_PinOutput),
    PL_ENUM_MEMBER_PROPERTY("Format", plSourceFormat, m_Format),
    PL_ENUM_MEMBER_PROPERTY("MSAA_Mode", plGALMSAASampleCount, m_MsaaMode),
    PL_MEMBER_PROPERTY("ClearColor", m_ClearColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PL_MEMBER_PROPERTY("Clear", m_bClear),
  }
  PL_END_PROPERTIES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_STATIC_REFLECTED_ENUM(plSourceFormat, 1)
  PL_ENUM_CONSTANTS(
    plSourceFormat::Color4Channel8BitNormalized_sRGB,
    plSourceFormat::Color4Channel8BitNormalized,
    plSourceFormat::Color4Channel16BitFloat,
    plSourceFormat::Color4Channel32BitFloat,
    plSourceFormat::Color3Channel11_11_10BitFloat,
    plSourceFormat::Depth16Bit,
    plSourceFormat::Depth24BitStencil8Bit,
    plSourceFormat::Depth32BitFloat
  )
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

plSourcePass::plSourcePass(const char* szName)
  : plRenderPipelinePass(szName, true)
{
  m_Format = plSourceFormat::Default;
  m_MsaaMode = plGALMSAASampleCount::None;
  m_bClear = true;
  m_ClearColor = plColor::Black;
}

plSourcePass::~plSourcePass() = default;

bool plSourcePass::GetRenderTargetDescriptions(
  const plView& view, const plArrayPtr<plGALTextureCreationDescription* const> inputs, plArrayPtr<plGALTextureCreationDescription> outputs)
{
  plUInt32 uiWidth = static_cast<plUInt32>(view.GetViewport().width);
  plUInt32 uiHeight = static_cast<plUInt32>(view.GetViewport().height);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  const plGALRenderTargets& renderTargets = view.GetActiveRenderTargets();

  plGALTextureCreationDescription desc;

  // Color
  if (m_Format == plSourceFormat::Color4Channel8BitNormalized || m_Format == plSourceFormat::Color4Channel8BitNormalized_sRGB)
  {
    plGALResourceFormat::Enum preferredFormat = plGALResourceFormat::Invalid;
    if (const plGALTexture* pTexture = pDevice->GetTexture(renderTargets.m_hRTs[0]))
    {
      auto rendertargetDesc = pTexture->GetDescription();

      preferredFormat = rendertargetDesc.m_Format;
    }

    switch (preferredFormat)
    {
      case plGALResourceFormat::RGBAUByteNormalized:
      case plGALResourceFormat::RGBAUByteNormalizedsRGB:
      default:
        if (m_Format == plSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = plGALResourceFormat::RGBAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = plGALResourceFormat::RGBAUByteNormalized;
        }
        break;
      case plGALResourceFormat::BGRAUByteNormalized:
      case plGALResourceFormat::BGRAUByteNormalizedsRGB:
        if (m_Format == plSourceFormat::Color4Channel8BitNormalized_sRGB)
        {
          desc.m_Format = plGALResourceFormat::BGRAUByteNormalizedsRGB;
        }
        else
        {
          desc.m_Format = plGALResourceFormat::BGRAUByteNormalized;
        }
        break;
    }
  }
  else
  {
    switch (m_Format)
    {
      case plSourceFormat::Color4Channel16BitFloat:
        desc.m_Format = plGALResourceFormat::RGBAHalf;
        break;
      case plSourceFormat::Color4Channel32BitFloat:
        desc.m_Format = plGALResourceFormat::RGBAFloat;
        break;
      case plSourceFormat::Color3Channel11_11_10BitFloat:
        desc.m_Format = plGALResourceFormat::RG11B10Float;
        break;
      case plSourceFormat::Depth16Bit:
        desc.m_Format = plGALResourceFormat::D16;
        break;
      case plSourceFormat::Depth24BitStencil8Bit:
        desc.m_Format = plGALResourceFormat::D24S8;
        break;
      case plSourceFormat::Depth32BitFloat:
        desc.m_Format = plGALResourceFormat::DFloat;
        break;
      default:
        PL_ASSERT_NOT_IMPLEMENTED
    }
  }

  desc.m_uiWidth = uiWidth;
  desc.m_uiHeight = uiHeight;
  desc.m_SampleCount = m_MsaaMode;
  desc.m_bCreateRenderTarget = true;
  desc.m_uiArraySize = view.GetCamera()->IsStereoscopic() ? 2 : 1;

  outputs[m_PinOutput.m_uiOutputIndex] = desc;

  return true;
}

void plSourcePass::Execute(const plRenderViewContext& renderViewContext, const plArrayPtr<plRenderPipelinePassConnection* const> inputs,
  const plArrayPtr<plRenderPipelinePassConnection* const> outputs)
{
  if (!m_bClear)
    return;

  auto pOutput = outputs[m_PinOutput.m_uiOutputIndex];
  if (pOutput == nullptr)
  {
    return;
  }

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  // Setup render target
  plGALRenderingSetup renderingSetup;
  renderingSetup.m_ClearColor = m_ClearColor;
  renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;
  renderingSetup.m_bClearDepth = true;
  renderingSetup.m_bClearStencil = true;

  if (plGALResourceFormat::IsDepthFormat(pOutput->m_Desc.m_Format))
  {
    renderingSetup.m_RenderTargetSetup.SetDepthStencilTarget(pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }
  else
  {
    renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->GetDefaultRenderTargetView(pOutput->m_TextureHandle));
  }

  auto pCommandEncoder = plRenderContext::BeginPassAndRenderingScope(renderViewContext, renderingSetup, GetName());
}

plResult plSourcePass::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(SUPER::Serialize(inout_stream));
  inout_stream << m_Format;
  inout_stream << m_MsaaMode;
  inout_stream << m_ClearColor;
  inout_stream << m_bClear;
  return PL_SUCCESS;
}

plResult plSourcePass::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(SUPER::Deserialize(inout_stream));
  const plUInt32 uiVersion = plTypeVersionReadContext::GetContext()->GetTypeVersion(GetStaticRTTI());
  PL_IGNORE_UNUSED(uiVersion);
  inout_stream >> m_Format;
  inout_stream >> m_MsaaMode;
  inout_stream >> m_ClearColor;
  inout_stream >> m_bClear;
  return PL_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Reflection/ReflectionUtils.h>

class plSourcePassPatch_1_2 : public plGraphPatch
{
public:
  plSourcePassPatch_1_2()
    : plGraphPatch("plSourcePass", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("MSAA Mode", "MSAA_Mode");
    pNode->RenameProperty("Clear Color", "ClearColor");
  }
};

plSourcePassPatch_1_2 g_plSourcePassPatch_1_2;

class plSourcePassPatch_2_3 : public plGraphPatch
{
public:
  plSourcePassPatch_2_3()
    : plGraphPatch("plSourcePass", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    plAbstractObjectNode::Property* formatProperty = pNode->FindProperty("Format");
    if (formatProperty == nullptr)
      return;

    auto formatName = formatProperty->m_Value.Get<plString>();
    plEnum<plGALResourceFormat> oldFormat;
    plReflectionUtils::StringToEnumeration<plGALResourceFormat>(formatName.GetData(), oldFormat);

    plEnum<plSourceFormat> newFormat;

    switch (oldFormat)
    {
      case plGALResourceFormat::RGBAHalf:
        newFormat = plSourceFormat::Color4Channel16BitFloat;
        break;
      case plGALResourceFormat::RGBAFloat:
        newFormat = plSourceFormat::Color4Channel32BitFloat;
        break;
      case plGALResourceFormat::RG11B10Float:
        newFormat = plSourceFormat::Color3Channel11_11_10BitFloat;
        break;
      case plGALResourceFormat::D16:
        newFormat = plSourceFormat::Depth16Bit;
        break;
      case plGALResourceFormat::D24S8:
        newFormat = plSourceFormat::Depth24BitStencil8Bit;
        break;
      case plGALResourceFormat::DFloat:
        newFormat = plSourceFormat::Depth32BitFloat;
        break;
      case plGALResourceFormat::RGBAUByteNormalized:
      case plGALResourceFormat::BGRAUByteNormalized:
        newFormat = plSourceFormat::Color4Channel8BitNormalized;
        break;
      case plGALResourceFormat::RGBAUByteNormalizedsRGB:
      case plGALResourceFormat::BGRAUByteNormalizedsRGB:
        newFormat = plSourceFormat::Color4Channel8BitNormalized_sRGB;
        break;
      default:
        newFormat = plSourceFormat::Default;
        break;
    }

    plStringBuilder newFormatName;
    plReflectionUtils::EnumerationToString(newFormat, newFormatName);
    formatProperty->m_Value = newFormatName.GetView();
  }
};

plSourcePassPatch_2_3 g_plSourcePassPatch_2_3;


PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_Passes_SourcePass);
