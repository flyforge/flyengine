#include <GameComponentsPlugin/GameComponentsPCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameComponentsPlugin/Effects/Shake/CameraShakeVolumeComponent.h>

plSpatialData::Category plCameraShakeVolumeComponent::SpatialDataCategory = plSpatialData::RegisterCategory("CameraShakeVolumes", plSpatialData::Flags::None);

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plCameraShakeVolumeComponent, 1)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Strength", m_fStrength),
    PL_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    PL_ENUM_MEMBER_PROPERTY("OnFinishedAction", plOnComponentFinishedAction, m_OnFinishedAction),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
    PL_MESSAGE_HANDLER(plMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/CameraShake"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plCameraShakeVolumeComponent::plCameraShakeVolumeComponent() = default;
plCameraShakeVolumeComponent::~plCameraShakeVolumeComponent() = default;

void plCameraShakeVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void plCameraShakeVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void plCameraShakeVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    plMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void plCameraShakeVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_fStrength;
}

void plCameraShakeVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_fStrength;
}

float plCameraShakeVolumeComponent::ComputeForceAtGlobalPosition(const plSimdVec4f& vGlobalPos) const
{
  const plSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const plSimdTransform tInv = t.GetInverse();
  const plSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

  return ComputeForceAtLocalPosition(localPos);
}

void plCameraShakeVolumeComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != plTempHashedString("Suicide"))
    return;

  plOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void plCameraShakeVolumeComponent::OnMsgDeleteGameObject(plMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    plOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plCameraShakeVolumeSphereComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plSphereVisualizerAttribute("Radius", plColor::SaddleBrown),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plCameraShakeVolumeSphereComponent::plCameraShakeVolumeSphereComponent() = default;
plCameraShakeVolumeSphereComponent::~plCameraShakeVolumeSphereComponent() = default;

void plCameraShakeVolumeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void plCameraShakeVolumeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

float plCameraShakeVolumeSphereComponent::ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const
{
  plSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  plSimdFloat forceFactor = plSimdFloat(1.0f) - lenScaled;

  const plSimdFloat force = forceFactor.Max(0.0f);

  return m_fStrength * force;
}

void plCameraShakeVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = plMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plCameraShakeVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fRadius), plCameraShakeVolumeComponent::SpatialDataCategory);
}
