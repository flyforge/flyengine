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
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plWindVolumeComponent, 2)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Strength", plWindStrength, m_Strength),
    PL_MEMBER_PROPERTY("ReverseDirection", m_bReverseDirection),
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
    new plCategoryAttribute("Effects/Wind"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
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

void plWindVolumeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_BurstDuration;
  s << m_OnFinishedAction;
  s << m_Strength;
  s << m_bReverseDirection;
}

void plWindVolumeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_BurstDuration;
  s >> m_OnFinishedAction;
  s >> m_Strength;

  if (uiVersion >= 2)
  {
    s >> m_bReverseDirection;
  }
}

plSimdVec4f plWindVolumeComponent::ComputeForceAtGlobalPosition(const plSimdVec4f& vGlobalPos) const
{
  const plSimdTransform t = GetOwner()->GetGlobalTransformSimd();
  const plSimdTransform tInv = t.GetInverse();
  const plSimdVec4f localPos = tInv.TransformPosition(vGlobalPos);

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
PL_BEGIN_COMPONENT_TYPE(plWindVolumeSphereComponent, 1, plComponentMode::Static)
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
    new plSphereVisualizerAttribute("Radius", plColor::CornflowerBlue),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeSphereComponent::plWindVolumeSphereComponent() = default;
plWindVolumeSphereComponent::~plWindVolumeSphereComponent() = default;

void plWindVolumeSphereComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
}

void plWindVolumeSphereComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;
}

plSimdVec4f plWindVolumeSphereComponent::ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const
{
  // TODO: could do this computation in global space

  plSimdFloat lenScaled = vLocalPos.GetLength<3>() * m_fOneDivRadius;

  // inverse quadratic falloff to have sharper edges
  plSimdFloat forceFactor = plSimdFloat(1.0f) - (lenScaled * lenScaled);

  const plSimdFloat force = GetWindInMetersPerSecond() * forceFactor.Max(0.0f);

  plSimdVec4f dir = vLocalPos;
  dir.NormalizeIfNotZero<3>();

  return dir * force;
}

void plWindVolumeSphereComponent::SetRadius(float fVal)
{
  m_fRadius = plMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeSphereComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), m_fRadius), plWindVolumeComponent::SpatialDataCategory);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_STATIC_REFLECTED_ENUM(plWindVolumeCylinderMode, 1)
  PL_ENUM_CONSTANTS(plWindVolumeCylinderMode::Directional, plWindVolumeCylinderMode::Vortex)
PL_END_STATIC_REFLECTED_ENUM;

PL_BEGIN_COMPONENT_TYPE(plWindVolumeCylinderComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new plDefaultValueAttribute(5.0f), new plClampValueAttribute(0.1f, plVariant())),
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
    PL_ENUM_MEMBER_PROPERTY("Mode", plWindVolumeCylinderMode, m_Mode),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCylinderVisualizerAttribute(plBasisAxis::PositiveX, "Length", "Radius", plColor::CornflowerBlue),
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 1.0f, plColor::DeepSkyBlue),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeCylinderComponent::plWindVolumeCylinderComponent() = default;
plWindVolumeCylinderComponent::~plWindVolumeCylinderComponent() = default;

void plWindVolumeCylinderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fLength;
  s << m_Mode;
}

void plWindVolumeCylinderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fRadius;
  m_fOneDivRadius = 1.0f / m_fRadius;

  s >> m_fLength;
  s >> m_Mode;
}

plSimdVec4f plWindVolumeCylinderComponent::ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const
{
  const plSimdFloat fCylDist = vLocalPos.x();

  if (fCylDist <= -m_fLength * 0.5f || fCylDist >= m_fLength * 0.5f)
    return plSimdVec4f::MakeZero();

  plSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= plMath::Square(m_fRadius))
    return plSimdVec4f::MakeZero();

  if (m_Mode == plWindVolumeCylinderMode::Vortex)
  {
    plSimdVec4f forceDir = plSimdVec4f(1, 0, 0, 0).CrossRH(orthoDir);
    forceDir.NormalizeIfNotZero<3>();
    return forceDir * GetWindInMetersPerSecond();
  }

  return plSimdVec4f(GetWindInMetersPerSecond(), 0, 0);
}

void plWindVolumeCylinderComponent::SetRadius(float fVal)
{
  m_fRadius = plMath::Max(fVal, 0.1f);
  m_fOneDivRadius = 1.0f / m_fRadius;

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeCylinderComponent::SetLength(float fVal)
{
  m_fLength = plMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeCylinderComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  const plVec3 corner(m_fLength * 0.5f, m_fRadius, m_fRadius);

  msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-corner, corner)), plWindVolumeComponent::SpatialDataCategory);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plWindVolumeConeComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Angle", GetAngle, SetAngle)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(45)), new plClampValueAttribute(plAngle::MakeFromDegree(1), plAngle::MakeFromDegree(179))),
    PL_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.1f, plVariant())),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds)
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plConeVisualizerAttribute(plBasisAxis::PositiveX, "Angle", 1.0f, "Length", plColor::CornflowerBlue),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE;
// clang-format on

plWindVolumeConeComponent::plWindVolumeConeComponent() = default;
plWindVolumeConeComponent::~plWindVolumeConeComponent() = default;

void plWindVolumeConeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_fLength;
  s << m_Angle;
}

void plWindVolumeConeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_fLength;
  s >> m_Angle;
}

plSimdVec4f plWindVolumeConeComponent::ComputeForceAtLocalPosition(const plSimdVec4f& vLocalPos) const
{
  const plSimdFloat fConeDist = vLocalPos.x();

  if (fConeDist <= plSimdFloat::MakeZero() || fConeDist >= m_fLength)
    return plSimdVec4f::MakeZero();

  // TODO: precompute base radius
  const float fBaseRadius = plMath::Tan(m_Angle * 0.5f) * m_fLength;

  // TODO: precompute 1/length
  const plSimdFloat fConeRadius = (fConeDist / plSimdFloat(m_fLength)) * plSimdFloat(fBaseRadius);

  plSimdVec4f orthoDir = vLocalPos;
  orthoDir.SetX(0.0f);

  if (orthoDir.GetLengthSquared<3>() >= fConeRadius * fConeRadius)
    return plSimdVec4f::MakeZero();

  return vLocalPos.GetNormalized<3>() * GetWindInMetersPerSecond();
}

void plWindVolumeConeComponent::SetLength(float fVal)
{
  m_fLength = plMath::Max(fVal, 0.1f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plWindVolumeConeComponent::SetAngle(plAngle val)
{
  m_Angle = plMath::Max(val, plAngle::MakeFromDegree(1.0f));

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

  msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(c0, c1)), plWindVolumeComponent::SpatialDataCategory);
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Effects_Wind_Implementation_WindVolumeComponent);
