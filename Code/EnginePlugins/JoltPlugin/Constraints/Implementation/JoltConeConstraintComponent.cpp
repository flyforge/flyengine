#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Constraints/JoltConeConstraintComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltConeConstraintComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("ConeAngle", GetConeAngle, SetConeAngle)->AddAttributes(new plClampValueAttribute(plAngle(), plAngle::Degree(175))),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plConeVisualizerAttribute(plBasisAxis::PositiveX, "ConeAngle", 0.3f, nullptr)
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltConeConstraintComponent::plJoltConeConstraintComponent() = default;
plJoltConeConstraintComponent::~plJoltConeConstraintComponent() = default;

void plJoltConeConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_ConeAngle;
}

void plJoltConeConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_ConeAngle;
}

void plJoltConeConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::ConeConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mHalfConeAngle = m_ConeAngle.GetRadian() * 0.5f;
  opt.mTwistAxis1 = inv1.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameA.m_qRotation * plVec3::UnitXAxis()));
  opt.mTwistAxis2 = inv2.Multiply3x3(plJoltConversionUtils::ToVec3(m_LocalFrameB.m_qRotation * plVec3::UnitXAxis()));

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

void plJoltConeConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint);
  pConstraint->SetHalfConeAngle(m_ConeAngle.GetRadian() * 0.5f);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool plJoltConeConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::ConeConstraint*>(m_pConstraint))
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
      if (pConstraint->GetTotalLambdaRotation() >= m_fBreakTorque)
      {
        return true;
      }
    }
  }

  return false;
}

void plJoltConeConstraintComponent::SetConeAngle(plAngle f)
{
  m_ConeAngle = f;
  QueueApplySettings();
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltConeConstraintComponent);
