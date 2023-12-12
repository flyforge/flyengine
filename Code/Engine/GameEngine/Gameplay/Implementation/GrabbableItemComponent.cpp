#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGrabbableItemGrabPoint, plNoBase, 1, plRTTIDefaultAllocator<plGrabbableItemGrabPoint>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    PLASMA_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE


PLASMA_BEGIN_COMPONENT_TYPE(plGrabbableItemComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_ACCESSOR_PROPERTY("GrabPoints", GrabPoints_GetCount, GrabPoints_GetValue, GrabPoints_SetValue, GrabPoints_Insert, GrabPoints_Remove),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay"),
    new plColorAttribute(plColorScheme::Gameplay),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGrabbableItemComponent::plGrabbableItemComponent() = default;
plGrabbableItemComponent::~plGrabbableItemComponent() = default;

plUInt32 plGrabbableItemComponent::GrabPoints_GetCount() const
{
  return m_GrabPoints.GetCount();
}

plGrabbableItemGrabPoint plGrabbableItemComponent::GrabPoints_GetValue(plUInt32 uiIndex) const
{
  return m_GrabPoints[uiIndex];
}

void plGrabbableItemComponent::GrabPoints_SetValue(plUInt32 uiIndex, plGrabbableItemGrabPoint value)
{
  m_GrabPoints[uiIndex] = value;
}

void plGrabbableItemComponent::GrabPoints_Insert(plUInt32 uiIndex, plGrabbableItemGrabPoint value)
{
  m_GrabPoints.Insert(value, uiIndex);
}

void plGrabbableItemComponent::GrabPoints_Remove(plUInt32 uiIndex)
{
  m_GrabPoints.RemoveAtAndCopy(uiIndex);
}

void plGrabbableItemComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  const plUInt8 uiNumGrabPoints = static_cast<plUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void plGrabbableItemComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  plUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
