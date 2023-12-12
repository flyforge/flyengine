#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/ApplyOnlyToMessage.h>
#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Decals/DecalAtlasResource.h>
#include <RendererCore/Decals/DecalComponent.h>
#include <RendererCore/Decals/DecalResource.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/ShaderUtils.h>

#include <RendererCore/../../../Data/Base/Shaders/Common/LightData.h>

plDecalComponentManager::plDecalComponentManager(plWorld* pWorld)
  : plComponentManager<plDecalComponent, plBlockStorageType::Compact>(pWorld)
{
}

void plDecalComponentManager::Initialize()
{
  m_hDecalAtlas = plDecalAtlasResource::GetDecalAtlasResource();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDecalRenderData, 1, plRTTIDefaultAllocator<plDecalRenderData>)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plDecalComponent, 8, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_ACCESSOR_PROPERTY("Decals", DecalFile_GetCount, DecalFile_Get, DecalFile_Set, DecalFile_Insert, DecalFile_Remove)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Decal")),
    PLASMA_ENUM_ACCESSOR_PROPERTY("ProjectionAxis", plBasisAxis, GetProjectionAxis, SetProjectionAxis),
    PLASMA_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plDefaultValueAttribute(plVec3(1.0f)), new plClampValueAttribute(plVec3(0.01f), plVariant(25.0f))),
    PLASMA_ACCESSOR_PROPERTY("SizeVariance", GetSizeVariance, SetSizeVariance)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f)),
    PLASMA_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plExposeColorAlphaAttribute()),
    PLASMA_ACCESSOR_PROPERTY("EmissiveColor", GetEmissiveColor, SetEmissiveColor)->AddAttributes(new plDefaultValueAttribute(plColor::Black)),
    PLASMA_ACCESSOR_PROPERTY("SortOrder", GetSortOrder, SetSortOrder)->AddAttributes(new plClampValueAttribute(-64.0f, 64.0f)),
    PLASMA_ACCESSOR_PROPERTY("WrapAround", GetWrapAround, SetWrapAround),
    PLASMA_ACCESSOR_PROPERTY("MapNormalToGeometry", GetMapNormalToGeometry, SetMapNormalToGeometry)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_ACCESSOR_PROPERTY("InnerFadeAngle", GetInnerFadeAngle, SetInnerFadeAngle)->AddAttributes(new plClampValueAttribute(plAngle::Degree(0.0f), plAngle::Degree(89.0f)), new plDefaultValueAttribute(plAngle::Degree(50.0f))),
    PLASMA_ACCESSOR_PROPERTY("OuterFadeAngle", GetOuterFadeAngle, SetOuterFadeAngle)->AddAttributes(new plClampValueAttribute(plAngle::Degree(0.0f), plAngle::Degree(89.0f)), new plDefaultValueAttribute(plAngle::Degree(80.0f))),
    PLASMA_MEMBER_PROPERTY("FadeOutDelay", m_FadeOutDelay),
    PLASMA_MEMBER_PROPERTY("FadeOutDuration", m_FadeOutDuration),
    PLASMA_ENUM_MEMBER_PROPERTY("OnFinishedAction", plOnComponentFinishedAction, m_OnFinishedAction),
    PLASMA_ACCESSOR_PROPERTY("ApplyToDynamic", DummyGetter, SetApplyToRef)->AddAttributes(new plGameObjectReferenceAttribute()),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
    new plColorAttribute(plColorScheme::Effects),
    new plDirectionVisualizerAttribute("ProjectionAxis", 0.5f, plColorScheme::LightUI(plColorScheme::Blue)),
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxVisualizerAttribute("Extents"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PLASMA_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
    PLASMA_MESSAGE_HANDLER(plMsgDeleteGameObject, OnMsgDeleteGameObject),
    PLASMA_MESSAGE_HANDLER(plMsgOnlyApplyToObject, OnMsgOnlyApplyToObject),
    PLASMA_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plDecalComponent::plDecalComponent() {}

plDecalComponent::~plDecalComponent() = default;

void plDecalComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_Color;
  s << m_EmissiveColor;
  s << m_InnerFadeAngle;
  s << m_OuterFadeAngle;
  s << m_fSortOrder;
  s << m_FadeOutDelay.m_Value;
  s << m_FadeOutDelay.m_fVariance;
  s << m_FadeOutDuration;
  s << m_StartFadeOutTime;
  s << m_fSizeVariance;
  s << m_OnFinishedAction;
  s << m_bWrapAround;
  s << m_bMapNormalToGeometry;

  // version 5
  s << m_ProjectionAxis;

  // version 6
  inout_stream.WriteGameObjectHandle(m_hApplyOnlyToObject);

  // version 7
  s << m_uiRandomDecalIdx;
  s.WriteArray(m_Decals).IgnoreResult();
}

void plDecalComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;

  if (uiVersion >= 4)
  {
    s >> m_Color;
    s >> m_EmissiveColor;
  }
  else
  {
    plColor tmp;
    s >> tmp;
    m_Color = tmp;
  }

  s >> m_InnerFadeAngle;
  s >> m_OuterFadeAngle;
  s >> m_fSortOrder;

  if (uiVersion <= 7)
  {
    plUInt32 dummy;
    s >> dummy;
  }

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  if (uiVersion < 7)
  {
    m_Decals.SetCount(1);
    s >> m_Decals[0];
  }

  s >> m_FadeOutDelay.m_Value;
  s >> m_FadeOutDelay.m_fVariance;
  s >> m_FadeOutDuration;
  s >> m_StartFadeOutTime;
  s >> m_fSizeVariance;
  s >> m_OnFinishedAction;

  if (uiVersion >= 3)
  {
    s >> m_bWrapAround;
  }

  if (uiVersion >= 4)
  {
    s >> m_bMapNormalToGeometry;
  }

  if (uiVersion >= 5)
  {
    s >> m_ProjectionAxis;
  }

  if (uiVersion >= 6)
  {
    SetApplyOnlyTo(inout_stream.ReadGameObjectHandle());
  }

  if (uiVersion >= 7)
  {
    s >> m_uiRandomDecalIdx;
    s.ReadArray(m_Decals).IgnoreResult();
  }
}

plResult plDecalComponent::GetLocalBounds(plBoundingBoxSphere& bounds, bool& bAlwaysVisible, plMsgUpdateLocalBounds& msg)
{
  if (m_Decals.IsEmpty())
    return PLASMA_FAILURE;

  m_uiRandomDecalIdx = (GetOwner()->GetStableRandomSeed() % m_Decals.GetCount()) & 0xFF;

  const plUInt32 uiDecalIndex = plMath::Min<plUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero())
    return PLASMA_FAILURE;

  float fAspectRatio = 1.0f;

  {
    auto hDecalAtlas = GetWorld()->GetComponentManager<plDecalComponentManager>()->m_hDecalAtlas;
    plResourceLock<plDecalAtlasResource> pDecalAtlas(hDecalAtlas, plResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const plUInt32 decalIdx = atlas.m_Items.Find(plHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != plInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      fAspectRatio = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  plVec3 vAspectCorrection = plVec3(1.0f);
  if (!plMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      vAspectCorrection.z /= fAspectRatio;
    }
    else
    {
      vAspectCorrection.y *= fAspectRatio;
    }
  }

  const plQuat axisRotation = plBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);
  plVec3 vHalfExtents = (axisRotation * vAspectCorrection).Abs().CompMul(m_vExtents * 0.5f);

  bounds = plBoundingBox(-vHalfExtents, vHalfExtents);
  return PLASMA_SUCCESS;
}

void plDecalComponent::SetExtents(const plVec3& value)
{
  m_vExtents = value.CompMax(plVec3::ZeroVector());

  TriggerLocalBoundsUpdate();
}

const plVec3& plDecalComponent::GetExtents() const
{
  return m_vExtents;
}

void plDecalComponent::SetSizeVariance(float fVariance)
{
  m_fSizeVariance = plMath::Clamp(fVariance, 0.0f, 1.0f);
}

float plDecalComponent::GetSizeVariance() const
{
  return m_fSizeVariance;
}

void plDecalComponent::SetColor(plColorGammaUB color)
{
  m_Color = color;
}

plColorGammaUB plDecalComponent::GetColor() const
{
  return m_Color;
}

void plDecalComponent::SetEmissiveColor(plColor color)
{
  m_EmissiveColor = color;
}

plColor plDecalComponent::GetEmissiveColor() const
{
  return m_EmissiveColor;
}

void plDecalComponent::SetInnerFadeAngle(plAngle spotAngle)
{
  m_InnerFadeAngle = plMath::Clamp(spotAngle, plAngle::Degree(0.0f), m_OuterFadeAngle);
}

plAngle plDecalComponent::GetInnerFadeAngle() const
{
  return m_InnerFadeAngle;
}

void plDecalComponent::SetOuterFadeAngle(plAngle spotAngle)
{
  m_OuterFadeAngle = plMath::Clamp(spotAngle, m_InnerFadeAngle, plAngle::Degree(90.0f));
}

plAngle plDecalComponent::GetOuterFadeAngle() const
{
  return m_OuterFadeAngle;
}

void plDecalComponent::SetSortOrder(float fOrder)
{
  m_fSortOrder = fOrder;
}

float plDecalComponent::GetSortOrder() const
{
  return m_fSortOrder;
}

void plDecalComponent::SetWrapAround(bool bWrapAround)
{
  m_bWrapAround = bWrapAround;
}

bool plDecalComponent::GetWrapAround() const
{
  return m_bWrapAround;
}

void plDecalComponent::SetMapNormalToGeometry(bool bMapNormal)
{
  m_bMapNormalToGeometry = bMapNormal;
}

bool plDecalComponent::GetMapNormalToGeometry() const
{
  return m_bMapNormalToGeometry;
}

void plDecalComponent::SetDecal(plUInt32 uiIndex, const plDecalResourceHandle& hDecal)
{
  m_Decals[uiIndex] = hDecal;

  TriggerLocalBoundsUpdate();
}

const plDecalResourceHandle& plDecalComponent::GetDecal(plUInt32 uiIndex) const
{
  return m_Decals[uiIndex];
}

void plDecalComponent::SetProjectionAxis(plEnum<plBasisAxis> projectionAxis)
{
  m_ProjectionAxis = projectionAxis;

  TriggerLocalBoundsUpdate();
}

plEnum<plBasisAxis> plDecalComponent::GetProjectionAxis() const
{
  return m_ProjectionAxis;
}

void plDecalComponent::SetApplyOnlyTo(plGameObjectHandle hObject)
{
  if (m_hApplyOnlyToObject != hObject)
  {
    m_hApplyOnlyToObject = hObject;
    UpdateApplyTo();
  }
}

plGameObjectHandle plDecalComponent::GetApplyOnlyTo() const
{
  return m_hApplyOnlyToObject;
}

void plDecalComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't extract decal render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  if (m_Decals.IsEmpty())
    return;

  const plUInt32 uiDecalIndex = plMath::Min<plUInt32>(m_uiRandomDecalIdx, m_Decals.GetCount() - 1);

  if (!m_Decals[uiDecalIndex].IsValid() || m_vExtents.IsZero() || GetOwner()->GetLocalScaling().IsZero())
    return;

  float fFade = 1.0f;

  const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();
  if (tNow > m_StartFadeOutTime)
  {
    fFade -= plMath::Min<float>(1.0f, (float)((tNow - m_StartFadeOutTime).GetSeconds() / m_FadeOutDuration.GetSeconds()));
  }

  plColor finalColor = m_Color;
  finalColor.a *= fFade;

  if (finalColor.a <= 0.0f)
    return;

  const bool bNoFade = m_InnerFadeAngle == plAngle::Radian(0.0f) && m_OuterFadeAngle == plAngle::Radian(0.0f);
  const float fCosInner = plMath::Cos(m_InnerFadeAngle);
  const float fCosOuter = plMath::Cos(m_OuterFadeAngle);
  const float fFadeParamScale = bNoFade ? 0.0f : (1.0f / plMath::Max(0.001f, (fCosInner - fCosOuter)));
  const float fFadeParamOffset = bNoFade ? 1.0f : (-fCosOuter * fFadeParamScale);

  auto hDecalAtlas = GetWorld()->GetComponentManager<plDecalComponentManager>()->m_hDecalAtlas;
  plVec4 baseAtlasScaleOffset = plVec4(0.5f);
  plVec4 normalAtlasScaleOffset = plVec4(0.5f);
  plVec4 ormAtlasScaleOffset = plVec4(0.5f);
  plUInt32 uiDecalFlags = 0;

  float fAspectRatio = 1.0f;

  {
    plResourceLock<plDecalAtlasResource> pDecalAtlas(hDecalAtlas, plResourceAcquireMode::BlockTillLoaded);

    const auto& atlas = pDecalAtlas->GetAtlas();
    const plUInt32 decalIdx = atlas.m_Items.Find(plHashingUtils::StringHashTo32(m_Decals[uiDecalIndex].GetResourceIDHash()));

    if (decalIdx != plInvalidIndex)
    {
      const auto& item = atlas.m_Items.GetValue(decalIdx);
      uiDecalFlags = item.m_uiFlags;

      auto layerRectToScaleOffset = [](plRectU32 layerRect, plVec2U32 vTextureSize) {
        plVec4 result;
        result.x = (float)layerRect.width / vTextureSize.x * 0.5f;
        result.y = (float)layerRect.height / vTextureSize.y * 0.5f;
        result.z = (float)layerRect.x / vTextureSize.x + result.x;
        result.w = (float)layerRect.y / vTextureSize.y + result.y;
        return result;
      };

      baseAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[0], pDecalAtlas->GetBaseColorTextureSize());
      normalAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[1], pDecalAtlas->GetNormalTextureSize());
      ormAtlasScaleOffset = layerRectToScaleOffset(item.m_LayerRects[2], pDecalAtlas->GetORMTextureSize());

      fAspectRatio = (float)item.m_LayerRects[0].width / item.m_LayerRects[0].height;
    }
  }

  auto pRenderData = plCreateRenderDataForThisFrame<plDecalRenderData>(GetOwner());

  plUInt32 uiSortingId = (plUInt32)(plMath::Min(m_fSortOrder * 512.0f, 32767.0f) + 32768.0f);
  pRenderData->m_uiSortingKey = (uiSortingId << 16) | (m_uiInternalSortKey & 0xFFFF);

  const plQuat axisRotation = plBasisAxis::GetBasisRotation_PosX(m_ProjectionAxis);

  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_GlobalTransform.m_vScale = (axisRotation * (pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents * 0.5f))).Abs();
  pRenderData->m_GlobalTransform.m_qRotation = pRenderData->m_GlobalTransform.m_qRotation * axisRotation;

  if (!plMath::IsEqual(fAspectRatio, 1.0f, 0.001f))
  {
    if (fAspectRatio > 1.0f)
    {
      pRenderData->m_GlobalTransform.m_vScale.z /= fAspectRatio;
    }
    else
    {
      pRenderData->m_GlobalTransform.m_vScale.y *= fAspectRatio;
    }
  }

  pRenderData->m_uiApplyOnlyToId = m_uiApplyOnlyToId;
  pRenderData->m_uiFlags = uiDecalFlags;
  pRenderData->m_uiFlags |= (m_bWrapAround ? DECAL_WRAP_AROUND : 0);
  pRenderData->m_uiFlags |= (m_bMapNormalToGeometry ? DECAL_MAP_NORMAL_TO_GEOMETRY : 0);
  pRenderData->m_uiAngleFadeParams = plShaderUtils::Float2ToRG16F(plVec2(fFadeParamScale, fFadeParamOffset));
  pRenderData->m_BaseColor = finalColor;
  pRenderData->m_EmissiveColor = m_EmissiveColor;
  plShaderUtils::Float4ToRGBA16F(baseAtlasScaleOffset, pRenderData->m_uiBaseColorAtlasScale, pRenderData->m_uiBaseColorAtlasOffset);
  plShaderUtils::Float4ToRGBA16F(normalAtlasScaleOffset, pRenderData->m_uiNormalAtlasScale, pRenderData->m_uiNormalAtlasOffset);
  plShaderUtils::Float4ToRGBA16F(ormAtlasScaleOffset, pRenderData->m_uiORMAtlasScale, pRenderData->m_uiORMAtlasOffset);

  plRenderData::Caching::Enum caching = (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0) ? plRenderData::Caching::Never : plRenderData::Caching::IfStatic;
  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Decal, caching);
}

void plDecalComponent::SetApplyToRef(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  plGameObjectHandle hTarget = resolver(szReference, GetHandle(), "ApplyTo");

  if (m_hApplyOnlyToObject == hTarget)
    return;

  m_hApplyOnlyToObject = hTarget;

  if (IsActiveAndInitialized())
  {
    UpdateApplyTo();
  }
}

void plDecalComponent::UpdateApplyTo()
{
  plUInt32 uiPrevId = m_uiApplyOnlyToId;

  m_uiApplyOnlyToId = 0;

  if (!m_hApplyOnlyToObject.IsInvalidated())
  {
    m_uiApplyOnlyToId = plInvalidIndex;

    plGameObject* pObject = nullptr;
    if (GetWorld()->TryGetObject(m_hApplyOnlyToObject, pObject))
    {
      plRenderComponent* pRenderComponent = nullptr;
      if (pObject->TryGetComponentOfBaseType(pRenderComponent))
      {
        // this only works for dynamic objects, for static ones we must use ID 0
        if (pRenderComponent->GetOwner()->IsDynamic())
        {
          m_uiApplyOnlyToId = pRenderComponent->GetUniqueIdForRendering();
        }
      }
    }
  }

  if (uiPrevId != m_uiApplyOnlyToId && GetOwner()->IsStatic())
  {
    InvalidateCachedRenderData();
  }
}

static plHashedString s_sSuicide = plMakeHashedString("Suicide");

void plDecalComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plWorld* pWorld = GetWorld();

  // no fade out -> fade out pretty late
  m_StartFadeOutTime = plTime::Hours(24.0 * 365.0 * 100.0); // 100 years should be enough for everybody (ignoring leap years)

  if (m_FadeOutDelay.m_Value.GetSeconds() > 0.0 || m_FadeOutDuration.GetSeconds() > 0.0)
  {
    const plTime tFadeOutDelay = plTime::Seconds(pWorld->GetRandomNumberGenerator().DoubleVariance(m_FadeOutDelay.m_Value.GetSeconds(), m_FadeOutDelay.m_fVariance));
    m_StartFadeOutTime = pWorld->GetClock().GetAccumulatedTime() + tFadeOutDelay;

    if (m_OnFinishedAction != plOnComponentFinishedAction::None)
    {
      plMsgComponentInternalTrigger msg;
      msg.m_sMessage = s_sSuicide;

      const plTime tKill = tFadeOutDelay + m_FadeOutDuration;

      PostMessage(msg, tKill);
    }
  }

  if (m_fSizeVariance > 0)
  {
    const float scale = (float)pWorld->GetRandomNumberGenerator().DoubleVariance(1.0, m_fSizeVariance);
    m_vExtents *= scale;

    TriggerLocalBoundsUpdate();

    InvalidateCachedRenderData();
  }
}

void plDecalComponent::OnActivated()
{
  SUPER::OnActivated();

  m_uiInternalSortKey = GetOwner()->GetStableRandomSeed();
  m_uiInternalSortKey = (m_uiInternalSortKey >> 16) ^ (m_uiInternalSortKey & 0xFFFF);

  UpdateApplyTo();
}

void plDecalComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != s_sSuicide)
    return;

  plOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);
}

void plDecalComponent::OnMsgDeleteGameObject(plMsgDeleteGameObject& msg)
{
  plOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
}

void plDecalComponent::OnMsgOnlyApplyToObject(plMsgOnlyApplyToObject& msg)
{
  SetApplyOnlyTo(msg.m_hObject);
}

void plDecalComponent::OnMsgSetColor(plMsgSetColor& msg)
{
  msg.ModifyColor(m_Color);
}

plUInt32 plDecalComponent::DecalFile_GetCount() const
{
  return m_Decals.GetCount();
}

const char* plDecalComponent::DecalFile_Get(plUInt32 uiIndex) const
{
  if (!m_Decals[uiIndex].IsValid())
    return "";

  return m_Decals[uiIndex].GetResourceID();
}

void plDecalComponent::DecalFile_Set(plUInt32 uiIndex, const char* szFile)
{
  plDecalResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plDecalResource>(szFile);
  }

  SetDecal(uiIndex, hResource);
}

void plDecalComponent::DecalFile_Insert(plUInt32 uiIndex, const char* szFile)
{
  m_Decals.Insert(plDecalResourceHandle(), uiIndex);
  DecalFile_Set(uiIndex, szFile);
}

void plDecalComponent::DecalFile_Remove(plUInt32 uiIndex)
{
  m_Decals.RemoveAtAndCopy(uiIndex);
}

//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plDecalComponent_6_7 : public plGraphPatch
{
public:
  plDecalComponent_6_7()
    : plGraphPatch("plDecalComponent", 7)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    auto* pDecal = pNode->FindProperty("Decal");
    if (pDecal && pDecal->m_Value.IsA<plString>())
    {
      plVariantArray ar;
      ar.PushBack(pDecal->m_Value.Get<plString>());
      pNode->AddProperty("Decals", ar);
    }
  }
};

plDecalComponent_6_7 g_plDecalComponent_6_7;

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Decals_Implementation_DecalComponent);
