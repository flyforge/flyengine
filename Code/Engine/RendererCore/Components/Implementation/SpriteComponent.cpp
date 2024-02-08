#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/SpriteComponent.h>
#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/View.h>
#include <RendererCore/Textures/Texture2DResource.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plSpriteBlendMode, 1)
  PL_ENUM_CONSTANTS(plSpriteBlendMode::Masked, plSpriteBlendMode::Transparent, plSpriteBlendMode::Additive)
PL_END_STATIC_REFLECTED_ENUM;
// clang-format on

// static
plTempHashedString plSpriteBlendMode::GetPermutationValue(Enum blendMode)
{
  switch (blendMode)
  {
    case plSpriteBlendMode::Masked:
    case plSpriteBlendMode::ShapeIcon:
      return "BLEND_MODE_MASKED";
    case plSpriteBlendMode::Transparent:
      return "BLEND_MODE_TRANSPARENT";
    case plSpriteBlendMode::Additive:
      return "BLEND_MODE_ADDITIVE";
  }

  return "";
}

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpriteRenderData, 1, plRTTIDefaultAllocator<plSpriteRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSpriteRenderData::FillBatchIdAndSortingKey()
{
  // ignore upper 32 bit of the resource ID hash
  const plUInt32 uiTextureIDHash = static_cast<plUInt32>(m_hTexture.GetResourceIDHash());

  // Generate batch id from mode and texture
  plUInt32 data[] = {(plUInt32)m_BlendMode, uiTextureIDHash};
  m_uiBatchId = plHashingUtils::xxHash32(data, sizeof(data));

  // Sort by mode and then by texture
  m_uiSortingKey = (m_BlendMode << 30) | (uiTextureIDHash & 0x3FFFFFFF);
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSpriteComponent, 3, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Texture", GetTextureFile, SetTextureFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Texture_2D")),
    PL_ENUM_MEMBER_PROPERTY("BlendMode", plSpriteBlendMode, m_BlendMode),
    PL_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PL_ACCESSOR_PROPERTY("Size", GetSize, SetSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f), new plSuffixAttribute(" m")),
    PL_ACCESSOR_PROPERTY("MaxScreenSize", GetMaxScreenSize, SetMaxScreenSize)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(64.0f), new plSuffixAttribute(" px")),
    PL_MEMBER_PROPERTY("AspectRatio", m_fAspectRatio)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plSpriteComponent::plSpriteComponent() = default;
plSpriteComponent::~plSpriteComponent() = default;

plResult plSpriteComponent::GetLocalBounds(plBoundingBoxSphere& ref_bounds, bool& ref_bAlwaysVisible, plMsgUpdateLocalBounds& ref_msg)
{
  ref_bounds = plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fSize * 0.5f);
  return PL_SUCCESS;
}

void plSpriteComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't render in shadow views
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow)
    return;

  if (!m_hTexture.IsValid())
    return;

  plSpriteRenderData* pRenderData = plCreateRenderDataForThisFrame<plSpriteRenderData>(GetOwner());
  {
    pRenderData->m_LastGlobalTransform = GetOwner()->GetLastGlobalTransform();
    pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
    pRenderData->m_GlobalBounds = GetOwner()->GetGlobalBounds();
    pRenderData->m_hTexture = m_hTexture;
    pRenderData->m_fSize = m_fSize;
    pRenderData->m_fMaxScreenSize = m_fMaxScreenSize;
    pRenderData->m_fAspectRatio = m_fAspectRatio;
    pRenderData->m_BlendMode = m_BlendMode;
    pRenderData->m_color = m_Color;
    pRenderData->m_texCoordScale = plVec2(1.0f);
    pRenderData->m_texCoordOffset = plVec2(0.0f);
    pRenderData->m_uiUniqueID = GetUniqueIdForRendering();

    pRenderData->FillBatchIdAndSortingKey();
  }

  // Determine render data category.
  plRenderData::Category category = plDefaultRenderDataCategories::LitTransparent;
  if (m_BlendMode == plSpriteBlendMode::Masked)
  {
    category = plDefaultRenderDataCategories::LitMasked;
  }

  msg.AddRenderData(pRenderData, category, plRenderData::Caching::IfStatic);
}

void plSpriteComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_hTexture;
  s << m_fSize;
  s << m_fMaxScreenSize;

  // Version 3
  s << m_Color; // HDR now
  s << m_fAspectRatio;
  s << m_BlendMode;
}

void plSpriteComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_hTexture;

  if (uiVersion < 3)
  {
    plColorGammaUB color;
    s >> color;
    m_Color = color;
  }

  s >> m_fSize;
  s >> m_fMaxScreenSize;

  if (uiVersion >= 3)
  {
    s >> m_Color;
    s >> m_fAspectRatio;
    s >> m_BlendMode;
  }
}

void plSpriteComponent::SetTexture(const plTexture2DResourceHandle& hTexture)
{
  m_hTexture = hTexture;
}

const plTexture2DResourceHandle& plSpriteComponent::GetTexture() const
{
  return m_hTexture;
}

void plSpriteComponent::SetTextureFile(const char* szFile)
{
  plTexture2DResourceHandle hTexture;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hTexture = plResourceManager::LoadResource<plTexture2DResource>(szFile);
  }

  SetTexture(hTexture);
}

const char* plSpriteComponent::GetTextureFile() const
{
  if (!m_hTexture.IsValid())
    return "";

  return m_hTexture.GetResourceID();
}

void plSpriteComponent::SetColor(plColor color)
{
  m_Color = color;
}

plColor plSpriteComponent::GetColor() const
{
  return m_Color;
}

void plSpriteComponent::SetSize(float fSize)
{
  m_fSize = fSize;

  TriggerLocalBoundsUpdate();
}

float plSpriteComponent::GetSize() const
{
  return m_fSize;
}

void plSpriteComponent::SetMaxScreenSize(float fSize)
{
  m_fMaxScreenSize = fSize;
}

float plSpriteComponent::GetMaxScreenSize() const
{
  return m_fMaxScreenSize;
}

void plSpriteComponent::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_Color);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plSpriteComponentPatch_1_2 : public plGraphPatch
{
public:
  plSpriteComponentPatch_1_2()
    : plGraphPatch("plSpriteComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override { pNode->RenameProperty("Max Screen Size", "MaxScreenSize"); }
};

plSpriteComponentPatch_1_2 g_plSpriteComponentPatch_1_2;



PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_SpriteComponent);
