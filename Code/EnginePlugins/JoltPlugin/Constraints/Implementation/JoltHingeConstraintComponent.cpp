#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Reflection/Implementation/PropertyAttributes.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockMulti.h>
#include <Jolt/Physics/Constraints/HingeConstraint.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <JoltPlugin/Constraints/JoltHingeConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltHingeConstraintComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("LimitMode", plJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    PLASMA_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitAngle, SetLowerLimitAngle)->AddAttributes(new plClampValueAttribute(plAngle::Degree(0), plAngle::Degree(180))),
    PLASMA_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitAngle, SetUpperLimitAngle)->AddAttributes(new plClampValueAttribute(plAngle::Degree(0), plAngle::Degree(180))),
    PLASMA_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ENUM_ACCESSOR_PROPERTY("DriveMode", plJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    PLASMA_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    PLASMA_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plMinValueTextAttribute("Maximum")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 0.2f, plColor::BurlyWood)
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltHingeConstraintComponent::plJoltHingeConstraintComponent() = default;
plJoltHingeConstraintComponent::~plJoltHingeConstraintComponent() = default;

void plJoltHingeConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_LimitMode;
  s << m_LowerLimit;
  s << m_UpperLimit;

  s << m_DriveMode;
  s << m_DriveTargetValue;
  s << m_fDriveStrength;

  s << m_fFriction;
}

void plJoltHingeConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_LimitMode;
  s >> m_LowerLimit;
  s >> m_UpperLimit;

  s >> m_DriveMode;
  s >> m_DriveTargetValue;
  s >> m_fDriveStrength;

  s >> m_fFriction;
}

void plJoltHingeConstraintComponent::SetLimitMode(plJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetLowerLimitAngle(plAngle f)
{
  m_LowerLimit = plMath::Clamp(f, plAngle(), plAngle::Degree(180));
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetUpperLimitAngle(plAngle f)
{
  m_UpperLimit = plMath::Clamp(f, plAngle(), plAngle::Degree(180));
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetFriction(float f)
{
  m_fFriction = plMath::Max(f, 0.0f);
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetDriveMode(plJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetDriveTargetValue(plAngle f)
{
  m_DriveTargetValue = f;
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = plMath::Max(f, 0.0f);
  QueueApplySettings();
}

void plJoltHingeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::HingeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHingeAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3(1, 0, 0)));
  opt.mHingeAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3(1, 0, 0)));
  opt.mNormalAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3(0, 1, 0)));
  opt.mNormalAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void plJoltHingeConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  JPH::HingeConstraint* pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionTorque(m_fFriction);

  if (m_LimitMode != plJoltConstraintLimitMode::NoLimit)
  {
    float low = m_LowerLimit.GetRadian();
    float high = m_UpperLimit.GetRadian();

    const float fLowest = plAngle::Degree(1.0f).GetRadian();

    // there should be at least some slack
    if (low <= fLowest && high <= fLowest)
    {
      low = fLowest;
      high = fLowest;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-JPH::JPH_PI, +JPH::JPH_PI);
  }

  // drive
  {
    if (m_DriveMode == plJoltConstraintDriveMode::NoDrive)
    {
      pConstraint->SetMotorState(JPH::EMotorState::Off);
    }
    else
    {
      if (m_DriveMode == plJoltConstraintDriveMode::DriveVelocity)
      {
        pConstraint->SetMotorState(JPH::EMotorState::Velocity);
        pConstraint->SetTargetAngularVelocity(m_DriveTargetValue.GetRadian());
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetAngle(m_DriveTargetValue.GetRadian());
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

      pConstraint->GetMotorSettings().mSpringSettings.mMode = JPH::ESpringMode::FrequencyAndDamping;
      pConstraint->GetMotorSettings().mSpringSettings.mFrequency = 20.0f;
      pConstraint->GetMotorSettings().SetForceLimit(strength);
      pConstraint->GetMotorSettings().SetTorqueLimit(strength);
    }
  }

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool plJoltHingeConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::HingeConstraint*>(m_pConstraint))
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
      if (pConstraint->GetTotalLambdaRotation()[0] >= m_fBreakTorque ||
          pConstraint->GetTotalLambdaRotation()[1] >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltHingeConstraintComponent);
