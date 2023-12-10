#include <GameEngine/GameEnginePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Gameplay/MarkerComponent.h>
#include <GameEngine/Messages/DamageMessage.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plMarkerComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Marker", GetMarkerType, SetMarkerType)->AddAttributes(new plDynamicStringEnumAttribute("SpatialDataCategoryEnum")),
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(0.1)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnMsgUpdateLocalBounds)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay/Logic"),
    new plSphereVisualizerAttribute("Radius", plColor::LightSkyBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plMarkerComponent::plMarkerComponent() = default;
plMarkerComponent::~plMarkerComponent() = default;

void plMarkerComponent::SetMarkerType(const char* szType)
{
  m_sMarkerType.Assign(szType);

  UpdateMarker();
}

const char* plMarkerComponent::GetMarkerType() const
{
  return m_sMarkerType;
}

void plMarkerComponent::SetRadius(float radius)
{
  m_fRadius = radius;

  UpdateMarker();
}

float plMarkerComponent::GetRadius() const
{
  return m_fRadius;
}

void plMarkerComponent::OnMsgUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3(0), m_fRadius), m_SpatialCategory);
}

void plMarkerComponent::UpdateMarker()
{
  if (!m_sMarkerType.IsEmpty())
  {
    m_SpatialCategory = plSpatialData::RegisterCategory(m_sMarkerType.GetString(), plSpatialData::Flags::None);
  }
  else
  {
    m_SpatialCategory = plInvalidSpatialDataCategory;
  }

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plMarkerComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_sMarkerType;
  s << m_fRadius;
}

void plMarkerComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_sMarkerType;
  s >> m_fRadius;
}

void plMarkerComponent::OnActivated()
{
  SUPER::OnActivated();

  UpdateMarker();
}

void plMarkerComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  GetOwner()->UpdateLocalBounds();
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_MarkerComponent);
