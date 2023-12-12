#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltSwingTwistConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltSwingTwistConstraintComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("SwingLimitY", GetSwingLimitY, SetSwingLimitY)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::Degree(175))),
    PLASMA_ACCESSOR_PROPERTY("SwingLimitZ", GetSwingLimitZ, SetSwingLimitZ)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::Degree(175))),

    PLASMA_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),

    PLASMA_ACCESSOR_PROPERTY("LowerTwistLimit", GetLowerTwistLimit, SetLowerTwistLimit)->AddAttributes(new plClampValueAttribute(plAngle::Degree(5), plAngle::Degree(175)), new plDefaultValueAttribute(plAngle::Degree(90))),
    PLASMA_ACCESSOR_PROPERTY("UpperTwistLimit", GetUpperTwistLimit, SetUpperTwistLimit)->AddAttributes(new plClampValueAttribute(plAngle::Degree(5), plAngle::Degree(175)), new plDefaultValueAttribute(plAngle::Degree(90))),

    //PLASMA_ENUM_ACCESSOR_PROPERTY("TwistDriveMode", plJoltConstraintDriveMode, GetTwistDriveMode, SetTwistDriveMode),
    //PLASMA_ACCESSOR_PROPERTY("TwistDriveTargetValue", GetTwistDriveTargetValue, SetTwistDriveTargetValue),
    //PLASMA_ACCESSOR_PROPERTY("TwistDriveStrength", GetTwistDriveStrength, SetTwistDriveStrength)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plMinValueTextAttribute("Maximum"))
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plConeVisualizerAttribute(plBasisAxis::PositiveX, "SwingLimitY", 0.3f, nullptr)
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltSwingTwistConstraintComponent::plJoltSwingTwistConstraintComponent() = default;
plJoltSwingTwistConstraintComponent::~plJoltSwingTwistConstraintComponent() = default;

void plJoltSwingTwistConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_SwingLimitY;
  s << m_SwingLimitZ;

  s << m_LowerTwistLimit;
  s << m_UpperTwistLimit;

  s << m_fFriction;

  // s << m_TwistDriveMode;
  // s << m_TwistDriveTargetValue;
  // s << m_fTwistDriveStrength;
}

void plJoltSwingTwistConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_SwingLimitY;
  s >> m_SwingLimitZ;

  s >> m_LowerTwistLimit;
  s >> m_UpperTwistLimit;

  s >> m_fFriction;

  // s >> m_TwistDriveMode;
  // s >> m_TwistDriveTargetValue;
  // s >> m_fTwistDriveStrength;
}

void plJoltSwingTwistConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SwingTwistConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPosition1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPosition2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mPlaneHalfConeAngle = m_SwingLimitY.GetRadian() * 0.5f;
  opt.mNormalHalfConeAngle = m_SwingLimitZ.GetRadian() * 0.5f;
  opt.mMaxFrictionTorque = m_fFriction;
  opt.mTwistAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3::UnitXAxis()));
  opt.mTwistAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3::UnitXAxis()));
  opt.mTwistMinAngle = -m_LowerTwistLimit.GetRadian();
  opt.mTwistMaxAngle = m_UpperTwistLimit.GetRadian();
  opt.mPlaneAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3::UnitYAxis()));
  opt.mPlaneAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3::UnitYAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void plJoltSwingTwistConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);
  pConstraint->SetPlaneHalfConeAngle(m_SwingLimitY.GetRadian() * 0.5f);
  pConstraint->SetNormalHalfConeAngle(m_SwingLimitZ.GetRadian() * 0.5f);
  pConstraint->SetTwistMinAngle(-m_LowerTwistLimit.GetRadian());
  pConstraint->SetTwistMaxAngle(m_UpperTwistLimit.GetRadian());

  // drive
  //{
  //  if (m_TwistDriveMode == plJoltConstraintDriveMode::NoDrive)
  //  {
  //    pConstraint->SetTwistMotorState(JPH::EMotorState::Off);
  //  }
  //  else
  //  {
  //    if (m_TwistDriveMode == plJoltConstraintDriveMode::DriveVelocity)
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Velocity);
  //      pConstraint->SetTargetAngularVelocityCS(JPH::Vec3::sReplicate(m_TwistDriveTargetValue.GetRadian()));
  //    }
  //    else
  //    {
  //      pConstraint->SetTwistMotorState(JPH::EMotorState::Position);
  //      //pConstraint->SetTargetOrientationCS(m_TwistDriveTargetValue.GetRadian());
  //    }

  //    const float strength = (m_fTwistDriveStrength == 0) ? FLT_MAX : m_fTwistDriveStrength;

  //    pConstraint->GetTwistMotorSettings().mFrequency = 2.0f;
  //    pConstraint->GetTwistMotorSettings().SetForceLimit(strength);
  //    pConstraint->GetTwistMotorSettings().SetTorqueLimit(strength);
  //  }
  //}

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool plJoltSwingTwistConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::SwingTwistConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition().ReduceMax() >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaSwingY() >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaSwingZ() >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaTwist() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void plJoltSwingTwistConstraintComponent::SetSwingLimitZ(plAngle f)
{
  m_SwingLimitZ = f;
  QueueApplySettings();
}

void plJoltSwingTwistConstraintComponent::SetSwingLimitY(plAngle f)
{
  m_SwingLimitY = f;
  QueueApplySettings();
}

void plJoltSwingTwistConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void plJoltSwingTwistConstraintComponent::SetLowerTwistLimit(plAngle f)
{
  m_LowerTwistLimit = f;
  QueueApplySettings();
}

void plJoltSwingTwistConstraintComponent::SetUpperTwistLimit(plAngle f)
{
  m_UpperTwistLimit = f;
  QueueApplySettings();
}

// void plJoltSwingTwistConstraintComponent::SetTwistDriveMode(plJoltConstraintDriveMode::Enum mode)
//{
//   m_TwistDriveMode = mode;
//   QueueApplySettings();
// }
//
// void plJoltSwingTwistConstraintComponent::SetTwistDriveTargetValue(plAngle f)
//{
//   m_TwistDriveTargetValue = f;
//   QueueApplySettings();
// }
//
// void plJoltSwingTwistConstraintComponent::SetTwistDriveStrength(float f)
//{
//   m_fTwistDriveStrength = f;
//   QueueApplySettings();
// }


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSwingTwistConstraintComponent);
