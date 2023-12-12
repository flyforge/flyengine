#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Math/Intersection.h>
#include <GameEngine/Effects/Wind/WindVolumeComponent.h>

plSpatialData::Category plWindVolumeComponent::SpatialDataCategory = plSpatialData::RegisterCategory("WindVolumes", plSpatialData::Flags::None);

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plWindVolumeComponent, 2)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Strength", plWindStrength, m_Strength),
    PLASMA_MEMBER_PROPERTY("ReverseDirection", m_bReverseDirection),
    PLASMA_MEMBER_PROPERTY("BurstDuration", m_BurstDuration),
    PLASMA_ENUM_MEMBER_PROPERTY("OnFinishedAction", plOnComponentFinishedAction, m_OnFinishedAction),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgComponentInternalTrigger, OnTriggered),
    PLASMA_MESSAGE_HANDLER(plMsgDeleteGameObject, OnMsgDeleteGameObject),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Effects/Wind"),
    new plColorAttribute(plColorScheme::Effects),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plWindVolumeComponent::plWindVolumeComponent() = default;
plWindVolumeComponent::~plWindVolumeComponent() = default;

void plWindVolumeComponent::OnActivated()
{
  SUPER::OnActivated();

  GetOwner()->UpdateLocalBounds();
}

void plWindVolumeComponent::OnDeactivated()
{
  GetOwner()->UpdateLocalBounds();

  SUPER::OnDeactivated();
}

void plWindVolumeComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_BurstDuration.IsPositive())
  {
    plMsgComponentInternalTrigger msg;
    msg.m_sMessage.Assign("Suicide");

    PostMessage(msg, m_BurstDuration);
  }
}

void plWindVolumeComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
  s << m_bReverseDirection;
}

void plWindVolumeComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;

  if (uiVersion >= 2)
  {
    s >> m_bReverseDirection;
  }
}

plSimdVec4f plWindVolumeComponent::ComputeForceAtGlobalPosition(const plSimdVec4f& globalPos) const
{
  const plSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const plSimdTransform tInv = t.GetInverse();
  const plSimdVec4f localPos = tInv.TransformPosition(globalPos);

  const plSimdVec4f force = ComputeForceAtLocalPosition(localPos);

  return t.TransformDirection(force);
}

void plWindVolumeComponent::OnTriggered(plMsgComponentInternalTrigger& msg)
{
  if (msg.m_sMessage != plTempHashedString("Suicide"))
    return;

  plOnComponentFinishedAction::HandleFinishedAction(this, m_OnFinishedAction);

  SetActiveFlag(false);
}

void plWindVolumeComponent::OnMsgDeleteGameObject(plMsgDeleteGameObject& msg)
{
  if (m_BurstDuration.IsPositive())
  {
    plOnComponentFinishedAction::HandleDeleteObjectMsg(msg, m_OnFinishedAction);
  }
}

float plWindVolumeComponent::GetWindInMetersPerSecond() const
{
  return m_bReverseDirection ? -plWindStrength::GetInMetersPerSecond(m_Strength) : plWindStrength::GetInMetersPerSecond(m_Strength);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plWindVolumeSphereComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereVisualizerAttribute("Radius", plColor::CornflowerBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeSphereComponent::plWindVolumeSphereComponent() = default;
plWindVolumeSphereComponent::~plWindVolumeSphereComponent() = default;

void plWindVolumeSphereComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
}

void plWindVolumeSphereComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

plSimdVec4f plWindVolumeSphereComponent::ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const
{
  // TODO: could do this computation in global space

  plSimdFloat lenScaled = localPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  plSimdFloat forceFactor = plSimdFloat(1.0f) - (lenScaled * lenScaled);

  const plSimdFloat force = GetWindInMetersPerSecond() * forceFactor.Max(0.0f);

  plSimdVec4f dir = localPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void plWindVolumeSphereComponent::SetRadius(float val)
{
  m_fRadius = plMath::Max(val, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(plBoundingSphere(plVec3::ZeroVector(), m_fRadius), plWindVolumeComponent::SpatialDataCategory);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plWindVolumeCylinderMode, 1)
  PLASMA_ENUM_CONSTANTS(plWindVolumeCylinderMode::Directional, plWindVolumeCylinderMode::Vortex)
PLASMA_END_STATIC_REFLECTED_ENUM;

PLASMA_BEGIN_COMPONENT_TYPE(plWindVolumeCylinderComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.1f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
    PLASMA_ENUM_MEMBER_PROPERTY("Mode", plWindVolumeCylinderMode, m_Mode),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCylinderVisualizerAttribute(plBasisAxis::PositiveX, "Length", "Radius", plColor::CornflowerBlue),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 1.0f, plColor::DeepSkyBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeCylinderComponent::plWindVolumeCylinderComponent() = default;
plWindVolumeCylinderComponent::~plWindVolumeCylinderComponent() = default;

void plWindVolumeCylinderComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fRadius;
  s << m_fLength;
  s << m_Mode;
}

void plWindVolumeCylinderComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;

  s >> m_fLength;
  s >> m_Mode;
}

plSimdVec4f plWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const
{
  const plSimdFloat fCylDist = localPos.x();

  if (fCylDist <= -m_fLength * 0.5f || fCylDist >= m_fLength * 0.5f)
    return plSimdVec4f::ZeroVector();

  plSimdVec4f orthoDir = localPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= plMath::Square(m_fRadius))
    return plSimdVec4f::ZeroVector();

  if (m_Mode == plWindVolumeCylinderMode::Vortex)
  {
    plSimdVec4f forceDir = plSimdVec4f(1, 0, 0, 0).CrossRH(orthoDir);
    forceDir.NormalizeIfNotZero<3>();
    return forceDir * GetWindInMetersPerSecond();
  }

  return plSimdVec4f(GetWindInMetersPerSecond(), 0, 0);
}

void plWindVolumeCylinderComponent::SetRadius(float val)
{
  m_fRadius = plMath::Max(val, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeCylinderComponent::SetLength(float val)
{
  m_fLength = plMath::Max(val, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeCylinderComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  const plVec3 corner(m_fLength * 0.5f, m_fRadius, m_fRadius);

  msg.AddBounds(plBoundingBox(-corner, corner), plWindVolumeComponent::SpatialDataCategory);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plWindVolumeConeComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::Degree(45)), new plClampValueAttribute(plAngle::Degree(1), plAngle::Degree(179))),
    PLASMA_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plConeVisualizerAttribute(plBasisAxis::PositiveX, "Angle", 1.0f, "Length", plColor::CornflowerBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeConeComponent::plWindVolumeConeComponent() = default;
plWindVolumeConeComponent::~plWindVolumeConeComponent() = default;

void plWindVolumeConeComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void plWindVolumeConeComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

plSimdVec4f plWindVolumeConeComponent::ComputeForceAtLocalPosition(const plSimdVec4f& localPos) const
{
  const plSimdFloat fConeDist = localPos.x();

  if (fConeDist <= plSimdFloat::Zero() || fConeDist >= m_fLength)
    return plSimdVec4f::ZeroVector();

  // TODO: precompute base radius
  const float fBaseRadius = plMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const plSimdFloat fConeRadius = (fConeDist / plSimdFloat(m_fLength)) * plSimdFloat(fBaseRadius);

  plSimdVec4f orthoDir = localPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return plSimdVec4f::ZeroVector();

  return localPos.GetNormalized<3>() * GetWindInMetersPerSecond();
}

void plWindVolumeConeComponent::SetLength(float val)
{
  m_fLength = plMath::Max(val, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeConeComponent::SetAngle(plAngle val)
{
  m_Angle = plMath::Max(val, plAngle::Degree(1.0f));

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeConeComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  plVec3 c0, c1;
  c0.x = 0;
  c0.y = -plMath::Tan(m_Angle * 0.5f) * m_fLength;
  c0.z = c0.y;

  c1.x = m_fLength;
  c1.y = plMath::Tan(m_Angle * 0.5f) * m_fLength;
  c1.z = c1.y;

  msg.AddBounds(plBoundingBox(c0, c1), plWindVolumeComponent::SpatialDataCategory);
}
