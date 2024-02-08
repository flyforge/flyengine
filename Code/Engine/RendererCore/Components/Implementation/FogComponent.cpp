#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/FogComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plFogRenderData, 1, plRTTIDefaultAllocator<plFogRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;

PL_BEGIN_COMPONENT_TYPE(plFogComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Color", GetColor, SetColor)->AddAttributes(new plDefaultValueAttribute(plColorGammaUB(plColor(0.2f, 0.2f, 0.3f)))),
    PL_ACCESSOR_PROPERTY("Density", GetDensity, SetDensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PL_ACCESSOR_PROPERTY("HeightFalloff", GetHeightFalloff, SetHeightFalloff)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(10.0f)),
    PL_ACCESSOR_PROPERTY("ModulateWithSkyColor", GetModulateWithSkyColor, SetModulateWithSkyColor),
    PL_ACCESSOR_PROPERTY("SkyDistance", GetSkyDistance, SetSkyDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1000.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plFogComponent::plFogComponent() = default;
plFogComponent::~plFogComponent() = default;

void plFogComponent::Deinitialize()
{
  plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void plFogComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();
}

void plFogComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();
}

void plFogComponent::SetColor(plColor color)
{
  m_Color = color;
  SetModified(PL_BIT(1));
}

plColor plFogComponent::GetColor() const
{
  return m_Color;
}

void plFogComponent::SetDensity(float fDensity)
{
  m_fDensity = plMath::Max(fDensity, 0.0f);
  SetModified(PL_BIT(2));
}

float plFogComponent::GetDensity() const
{
  return m_fDensity;
}

void plFogComponent::SetHeightFalloff(float fHeightFalloff)
{
  m_fHeightFalloff = plMath::Max(fHeightFalloff, 0.0f);
  SetModified(PL_BIT(3));
}

float plFogComponent::GetHeightFalloff() const
{
  return m_fHeightFalloff;
}

void plFogComponent::SetModulateWithSkyColor(bool bModulate)
{
  m_bModulateWithSkyColor = bModulate;
  SetModified(PL_BIT(4));
}

bool plFogComponent::GetModulateWithSkyColor() const
{
  return m_bModulateWithSkyColor;
}

void plFogComponent::SetSkyDistance(float fDistance)
{
  m_fSkyDistance = fDistance;
  SetModified(PL_BIT(5));
}

float plFogComponent::GetSkyDistance() const
{
  return m_fSkyDistance;
}

void plFogComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic);
}

void plFogComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  auto pRenderData = plCreateRenderDataForThisFrame<plFogRenderData>(GetOwner());

  pRenderData->m_LastGlobalTransform = GetOwner()->GetLastGlobalTransform();
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_Color = m_Color;
  pRenderData->m_fDensity = m_fDensity / 100.0f;
  pRenderData->m_fHeightFalloff = m_fHeightFalloff;
  pRenderData->m_fInvSkyDistance = m_bModulateWithSkyColor ? 1.0f / m_fSkyDistance : 0.0f;

  msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::Light, plRenderData::Caching::IfStatic);
}

void plFogComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_Color;
  s << m_fDensity;
  s << m_fHeightFalloff;
  s << m_fSkyDistance;
  s << m_bModulateWithSkyColor;
}

void plFogComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_Color;
  s >> m_fDensity;
  s >> m_fHeightFalloff;

  if (uiVersion >= 2)
  {
    s >> m_fSkyDistance;
    s >> m_bModulateWithSkyColor;
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_FogComponent);
