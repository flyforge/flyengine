#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/SliderConstraint.h>
#include <JoltPlugin/Constraints/JoltSliderConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltSliderConstraintComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_ACCESSOR_PROPERTY("LimitMode", plJoltConstraintLimitMode, GetLimitMode, SetLimitMode),
    PL_ACCESSOR_PROPERTY("LowerLimit", GetLowerLimitDistance, SetLowerLimitDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("UpperLimit", GetUpperLimitDistance, SetUpperLimitDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ACCESSOR_PROPERTY("Friction", GetFriction, SetFriction)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_ENUM_ACCESSOR_PROPERTY("DriveMode", plJoltConstraintDriveMode, GetDriveMode, SetDriveMode),
    PL_ACCESSOR_PROPERTY("DriveTargetValue", GetDriveTargetValue, SetDriveTargetValue),
    PL_ACCESSOR_PROPERTY("DriveStrength", GetDriveStrength, SetDriveStrength)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plMinValueTextAttribute("Maximum")),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute(plBasisAxis::PositiveX, 1.0f, plColor::Orange, nullptr, "UpperLimit"),
    new plDirectionVisualizerAttribute(plBasisAxis::NegativeX, 1.0f, plColor::Teal, nullptr, "LowerLimit"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltSliderConstraintComponent::plJoltSliderConstraintComponent() = default;
plJoltSliderConstraintComponent::~plJoltSliderConstraintComponent() = default;

void plJoltSliderConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fLowerLimitDistance;
  s << m_fUpperLimitDistance;
  s << m_fFriction;
  s << m_LimitMode;

  s << m_DriveMode;
  s << m_fDriveTargetValue;
  s << m_fDriveStrength;
}

void plJoltSliderConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fLowerLimitDistance;
  s >> m_fUpperLimitDistance;
  s >> m_fFriction;
  s >> m_LimitMode;

  s >> m_DriveMode;
  s >> m_fDriveTargetValue;
  s >> m_fDriveStrength;
}

void plJoltSliderConstraintComponent::SetLimitMode(plJoltConstraintLimitMode::Enum mode)
{
  m_LimitMode = mode;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetLowerLimitDistance(float f)
{
  m_fLowerLimitDistance = f;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetUpperLimitDistance(float f)
{
  m_fUpperLimitDistance = f;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetFriction(float f)
{
  m_fFriction = f;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetDriveMode(plJoltConstraintDriveMode::Enum mode)
{
  m_DriveMode = mode;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetDriveTargetValue(float f)
{
  m_fDriveTargetValue = f;
  QueueApplySettings();
}

void plJoltSliderConstraintComponent::SetDriveStrength(float f)
{
  m_fDriveStrength = plMath::Max(f, 0.0f);
  QueueApplySettings();
}


void plJoltSliderConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  JPH::SliderConstraint* pConstraint = static_cast<JPH::SliderConstraint*>(m_pConstraint);

  pConstraint->SetMaxFrictionForce(m_fFriction);

  if (m_LimitMode != plJoltConstraintLimitMode::NoLimit)
  {
    float low = m_fLowerLimitDistance;
    float high = m_fUpperLimitDistance;

    if (low == high) // both zero
    {
      high = low + 0.01f;
    }

    pConstraint->SetLimits(-low, high);
  }
  else
  {
    pConstraint->SetLimits(-FLT_MAX, FLT_MAX);
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
        pConstraint->SetTargetVelocity(m_fDriveTargetValue);
      }
      else
      {
        pConstraint->SetMotorState(JPH::EMotorState::Position);
        pConstraint->SetTargetPosition(m_fDriveTargetValue);
      }

      const float strength = (m_fDriveStrength == 0) ? FLT_MAX : m_fDriveStrength;

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

bool plJoltSliderConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::SliderConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition()[0] >= m_fBreakForce ||
          pConstraint->GetTotalLambdaPosition()[1] >= m_fBreakForce)
      {
        return true;
      }
    }

    if (m_fBreakTorque > 0)
    {
      if (pConstraint->GetTotalLambdaRotation().ReduceMax() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void plJoltSliderConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::SliderConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mSliderAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3(1, 0, 0)));
  opt.mSliderAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3(1, 0, 0)));
  opt.mNormalAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3(0, 1, 0)));
  opt.mNormalAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3(0, 1, 0)));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltSliderConstraintComponent);
