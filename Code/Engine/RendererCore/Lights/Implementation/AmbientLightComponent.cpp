#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/AmbientLightComponent.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/RenderWorld/RenderWorld.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAmbientLightComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("TopColor", GetTopColor, SetTopColor)->AddAttributes(new plDefaultValueAttribute(plColorGammaUB(plColor(0.2f, 0.2f, 0.3f)))),
    PLASMA_ACCESSOR_PROPERTY("BottomColor", GetBottomColor, SetBottomColor)->AddAttributes(new plDefaultValueAttribute(plColorGammaUB(plColor(0.1f, 0.1f, 0.15f)))),
    PLASMA_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f))
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering/Lighting"),
    new plColorAttribute(plColorScheme::Lighting),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plAmbientLightComponent::plAmbientLightComponent() = default;
plAmbientLightComponent::~plAmbientLightComponent() = default;

void plAmbientLightComponent::Deinitialize()
{
  plRenderWorld::DeleteCachedRenderData(GetOwner()->GetHandle(), GetHandle());

  SUPER::Deinitialize();
}

void plAmbientLightComponent::OnActivated()
{
  GetOwner()->UpdateLocalBounds();

  UpdateSkyIrradiance();
}

void plAmbientLightComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  plReflectionPool::ResetConstantSkyIrradiance(GetWorld());
}

void plAmbientLightComponent::SetTopColor(plColorGammaUB color)
{
  m_TopColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

plColorGammaUB plAmbientLightComponent::GetTopColor() const
{
  return m_TopColor;
}

void plAmbientLightComponent::SetBottomColor(plColorGammaUB color)
{
  m_BottomColor = color;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

plColorGammaUB plAmbientLightComponent::GetBottomColor() const
{
  return m_BottomColor;
}

void plAmbientLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = fIntensity;

  if (IsActiveAndInitialized())
  {
    UpdateSkyIrradiance();
  }
}

float plAmbientLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void plAmbientLightComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(GetOwner()->IsDynamic() ? plDefaultSpatialDataCategories::RenderDynamic : plDefaultSpatialDataCategories::RenderStatic);
}

void plAmbientLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_TopColor;
  s << m_BottomColor;
  s << m_fIntensity;
}

void plAmbientLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_TopColor;
  s >> m_BottomColor;
  s >> m_fIntensity;
}

void plAmbientLightComponent::UpdateSkyIrradiance()
{
  plColor topColor = plColor(m_TopColor) * m_fIntensity;
  plColor bottomColor = plColor(m_BottomColor) * m_fIntensity;
  plColor midColor = plMath::Lerp(bottomColor, topColor, 0.5f);

  plAmbientCube<plColor> ambientLightIrradiance;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::PosX] = midColor;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::NegX] = midColor;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::PosY] = midColor;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::NegY] = midColor;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::PosZ] = topColor;
  ambientLightIrradiance.m_Values[plAmbientCubeBasis::NegZ] = bottomColor;

  plReflectionPool::SetConstantSkyIrradiance(GetWorld(), ambientLightIrradiance);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plAmbientLightComponentPatch_1_2 : public plGraphPatch
{
public:
  plAmbientLightComponentPatch_1_2()
    : plGraphPatch("plAmbientLightComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Top Color", "TopColor");
    pNode->RenameProperty("Bottom Color", "BottomColor");
  }
};

plAmbientLightComponentPatch_1_2 g_plAmbientLightComponentPatch_1_2;



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_AmbientLightComponent);
