#include <JoltPlugin/JoltPluginPCH.h>

#if 0

#  include <Core/WorldSerializer/WorldReader.h>
#  include <Core/WorldSerializer/WorldWriter.h>
#  include <JoltPlugin/Constraints/Jolt6DOFConstraintComponent.h>
#  include <JoltPlugin/System/JoltCore.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_BITFLAGS(plJoltAxis, 1)
  PLASMA_BITFLAGS_CONSTANT(plJoltAxis::X),
  PLASMA_BITFLAGS_CONSTANT(plJoltAxis::Y),
  PLASMA_BITFLAGS_CONSTANT(plJoltAxis::Z),
PLASMA_END_STATIC_REFLECTED_BITFLAGS;

PLASMA_BEGIN_COMPONENT_TYPE(plJolt6DOFConstraintComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_BITFLAGS_ACCESSOR_PROPERTY("FreeLinearAxis", plJoltAxis, GetFreeLinearAxis, SetFreeLinearAxis),
    PLASMA_ENUM_ACCESSOR_PROPERTY("LinearLimitMode", plJoltConstraintLimitMode, GetLinearLimitMode, SetLinearLimitMode),
    PLASMA_ACCESSOR_PROPERTY("LinearRangeX", GetLinearRangeX, SetLinearRangeX),
    PLASMA_ACCESSOR_PROPERTY("LinearRangeY", GetLinearRangeY, SetLinearRangeY),
    PLASMA_ACCESSOR_PROPERTY("LinearRangeZ", GetLinearRangeZ, SetLinearRangeZ),
    PLASMA_ACCESSOR_PROPERTY("LinearStiffness", GetLinearStiffness, SetLinearStiffness)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("LinearDamping", GetLinearDamping, SetLinearDamping)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_BITFLAGS_ACCESSOR_PROPERTY("FreeAngularAxis", plJoltAxis, GetFreeAngularAxis, SetFreeAngularAxis),
    PLASMA_ENUM_ACCESSOR_PROPERTY("SwingLimitMode", plJoltConstraintLimitMode, GetSwingLimitMode, SetSwingLimitMode),
    PLASMA_ACCESSOR_PROPERTY("SwingLimit", GetSwingLimit, SetSwingLimit)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::Degree(175))),
    PLASMA_ACCESSOR_PROPERTY("SwingStiffness", GetSwingStiffness, SetSwingStiffness)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("SwingDamping", GetSwingDamping, SetSwingDamping)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ENUM_ACCESSOR_PROPERTY("TwistLimitMode", plJoltConstraintLimitMode, GetTwistLimitMode, SetTwistLimitMode),
    PLASMA_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new plClampValueAttribute(-plAngle::Degree(175), plAngle::Degree(175))),
    PLASMA_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new plClampValueAttribute(-plAngle::Degree(175), plAngle::Degree(175))),
    PLASMA_ACCESSOR_PROPERTY("TwistStiffness", GetTwistStiffness, SetTwistStiffness)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("TwistDamping", GetTwistDamping, SetTwistDamping)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.2, plColor::SlateGray)
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJolt6DOFConstraintComponent::plJolt6DOFConstraintComponent() = default;
plJolt6DOFConstraintComponent::~plJolt6DOFConstraintComponent() = default;

void plJolt6DOFConstraintComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_FreeLinearAxis;
  s << m_FreeAngularAxis;
  s << m_fLinearStiffness;
  s << m_fLinearDamping;
  s << m_fSwingStiffness;
  s << m_fSwingDamping;

  s << m_LinearLimitMode;
  s << m_vLinearRangeX;
  s << m_vLinearRangeY;
  s << m_vLinearRangeZ;

  s << m_SwingLimitMode;
  s << m_SwingLimit;

  s << m_TwistLimitMode;
  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;
  s << m_fTwistStiffness;
  s << m_fTwistDamping;
}

void plJolt6DOFConstraintComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_FreeLinearAxis;
  s >> m_FreeAngularAxis;
  s >> m_fLinearStiffness;
  s >> m_fLinearDamping;
  s >> m_fSwingStiffness;
  s >> m_fSwingDamping;

  s >> m_LinearLimitMode;
  s >> m_vLinearRangeX;
  s >> m_vLinearRangeY;
  s >> m_vLinearRangeZ;
  s >> m_SwingLimitMode;
  s >> m_SwingLimit;

  s >> m_TwistLimitMode;
  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;
  s >> m_fTwistStiffness;
  s >> m_fTwistDamping;
}

void plJolt6DOFConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  //PLASMA_ASSERT_DEV(localFrame0.isFinite() && localFrame0.isValid() && localFrame0.isSane(), "frame 0");
  //PLASMA_ASSERT_DEV(localFrame1.isFinite() && localFrame1.isValid() && localFrame1.isSane(), "frame 1");
  //
  //  m_pJoint = PxD6JointCreate(*(plJolt::GetSingleton()->GetJoltAPI()), actor0, localFrame0, actor1, localFrame1);
}

void plJolt6DOFConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  //JoltD6Joint* pJoint = static_cast<PxD6Joint*>(m_pJoint);

  //if (m_LinearLimitMode == plJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eX, m_FreeLinearAxis.IsSet(plJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, m_FreeLinearAxis.IsSet(plJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, m_FreeLinearAxis.IsSet(plJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeLinearAxis;

  //  if (m_LinearLimitMode == plJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (plMath::IsEqual(m_vLinearRangeX.x, m_vLinearRangeX.y, 0.05f))
  //      freeAxis.Remove(plJoltAxis::X);
  //    if (plMath::IsEqual(m_vLinearRangeY.x, m_vLinearRangeY.y, 0.05f))
  //      freeAxis.Remove(plJoltAxis::Y);
  //    if (plMath::IsEqual(m_vLinearRangeZ.x, m_vLinearRangeZ.y, 0.05f))
  //      freeAxis.Remove(plJoltAxis::Z);
  //  }

  //  pJoint->setMotion(PxD6Axis::eX, freeAxis.IsSet(plJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eY, freeAxis.IsSet(plJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eZ, freeAxis.IsSet(plJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  PxJointLinearLimitPair l(0, 0, PxSpring(0, 0));

  //  if (m_LinearLimitMode == plJoltConstraintLimitMode::SoftLimit)
  //  {
  //    l.stiffness = m_fLinearStiffness;
  //    l.damping = m_fLinearDamping;
  //  }
  //  else
  //  {
  //    l.restitution = m_fLinearStiffness;
  //    l.bounceThreshold = m_fLinearDamping;
  //  }

  //  if (freeAxis.IsSet(plJoltAxis::X))
  //  {
  //    l.lower = m_vLinearRangeX.x;
  //    l.upper = m_vLinearRangeX.y;

  //    if (l.lower > l.upper)
  //      plMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eX, l);
  //  }

  //  if (freeAxis.IsSet(plJoltAxis::Y))
  //  {
  //    l.lower = m_vLinearRangeY.x;
  //    l.upper = m_vLinearRangeY.y;

  //    if (l.lower > l.upper)
  //      plMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eY, l);
  //  }

  //  if (freeAxis.IsSet(plJoltAxis::Z))
  //  {
  //    l.lower = m_vLinearRangeZ.x;
  //    l.upper = m_vLinearRangeZ.y;

  //    if (l.lower > l.upper)
  //      plMath::Swap(l.lower, l.upper);

  //    pJoint->setLinearLimit(PxD6Axis::eZ, l);
  //  }
  //}


  //if (m_SwingLimitMode == plJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eSWING1, m_FreeAngularAxis.IsSet(plJoltAxis::Y) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, m_FreeAngularAxis.IsSet(plJoltAxis::Z) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == plJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (plMath::IsZero(m_SwingLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(plJoltAxis::Y);
  //      freeAxis.Remove(plJoltAxis::Z);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eSWING1, freeAxis.IsSet(plJoltAxis::Y) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);
  //  pJoint->setMotion(PxD6Axis::eSWING2, freeAxis.IsSet(plJoltAxis::Z) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsAnySet(plJoltAxis::Y | plJoltAxis::Z))
  //  {
  //    const float fSwingLimit = plMath::Max(plAngle::Degree(0.5f).GetRadian(), m_SwingLimit.GetRadian());

  //    PxJointLimitCone l(fSwingLimit, fSwingLimit);

  //    if (m_SwingLimitMode == plJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fSwingStiffness;
  //      l.damping = m_fSwingDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fSwingStiffness;
  //      l.bounceThreshold = m_fSwingDamping;
  //    }

  //    pJoint->setSwingLimit(l);
  //  }
  //}

  //if (m_TwistLimitMode == plJoltConstraintLimitMode::NoLimit)
  //{
  //  pJoint->setMotion(PxD6Axis::eTWIST, m_FreeAngularAxis.IsSet(plJoltAxis::X) ? PxD6Motion::eFREE : PxD6Motion::eLOCKED);
  //}
  //else
  //{
  //  auto freeAxis = m_FreeAngularAxis;

  //  if (m_SwingLimitMode == plJoltConstraintLimitMode::HardLimit)
  //  {
  //    if (plMath::IsEqual(m_LowerTwistLimit.GetDegree(), m_UpperTwistLimit.GetDegree(), 1.0f))
  //    {
  //      freeAxis.Remove(plJoltAxis::X);
  //    }
  //  }

  //  pJoint->setMotion(PxD6Axis::eTWIST, freeAxis.IsSet(plJoltAxis::X) ? PxD6Motion::eLIMITED : PxD6Motion::eLOCKED);

  //  if (freeAxis.IsSet(plJoltAxis::X))
  //  {
  //    PxJointAngularLimitPair l(m_LowerTwistLimit.GetRadian(), m_UpperTwistLimit.GetRadian());

  //    if (l.lower > l.upper)
  //    {
  //      plMath::Swap(l.lower, l.upper);
  //    }

  //    if (plMath::IsEqual(l.lower, l.upper, plAngle::Degree(0.5f).GetRadian()))
  //    {
  //      l.lower -= plAngle::Degree(0.5f).GetRadian();
  //      l.upper += plAngle::Degree(0.5f).GetRadian();
  //    }

  //    if (m_TwistLimitMode == plJoltConstraintLimitMode::SoftLimit)
  //    {
  //      l.stiffness = m_fTwistStiffness;
  //      l.damping = m_fTwistDamping;
  //    }
  //    else
  //    {
  //      l.restitution = m_fTwistStiffness;
  //      l.bounceThreshold = m_fTwistDamping;
  //    }

  //    pJoint->setTwistLimit(l);
  //  }
  //}
}

void plJolt6DOFConstraintComponent::SetFreeLinearAxis(plBitflags<plJoltAxis> flags)
{
  m_FreeLinearAxis = flags;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetFreeAngularAxis(plBitflags<plJoltAxis> flags)
{
  m_FreeAngularAxis = flags;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearLimitMode(plJoltConstraintLimitMode::Enum mode)
{
  m_LinearLimitMode = mode;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearRangeX(const plVec2& value)
{
  m_vLinearRangeX = value;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearRangeY(const plVec2& value)
{
  m_vLinearRangeY = value;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearRangeZ(const plVec2& value)
{
  m_vLinearRangeZ = value;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearStiffness(float f)
{
  m_fLinearStiffness = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLinearDamping(float f)
{
  m_fLinearDamping = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetSwingLimitMode(plJoltConstraintLimitMode::Enum mode)
{
  m_SwingLimitMode = mode;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetSwingLimit(plAngle f)
{
  m_SwingLimit = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetSwingStiffness(float f)
{
  m_fSwingStiffness = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetSwingDamping(float f)
{
  m_fSwingDamping = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetTwistLimitMode(plJoltConstraintLimitMode::Enum mode)
{
  m_TwistLimitMode = mode;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetLowerTwistLimit(plAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetUpperTwistLimit(plAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetTwistStiffness(float f)
{
  m_fTwistStiffness = f;
  QueueApplySettings();
}

void plJolt6DOFConstraintComponent::SetTwistDamping(float f)
{
  m_fTwistDamping = f;
  QueueApplySettings();
}

#endif


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_Jolt6DOFConstraintComponent);

