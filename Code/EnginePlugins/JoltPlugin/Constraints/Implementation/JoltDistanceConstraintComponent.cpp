#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/DistanceConstraint.h>
#include <JoltPlugin/Constraints/JoltDistanceConstraintComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltDistanceConstraintComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("MinDistance", GetMinDistance, SetMinDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_ACCESSOR_PROPERTY("Frequency", GetFrequency, SetFrequency)->AddAttributes(new plClampValueAttribute(0.0f, 120.0f), new plDefaultValueAttribute(2.0f)),
    PLASMA_ACCESSOR_PROPERTY("Damping", GetDamping, SetDamping)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f), new plDefaultValueAttribute(0.5f)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plSphereVisualizerAttribute("MinDistance", plColor::IndianRed),
    new plSphereVisualizerAttribute("MaxDistance", plColor::LightSkyBlue),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltDistanceConstraintComponent::plJoltDistanceConstraintComponent() = default;
plJoltDistanceConstraintComponent::~plJoltDistanceConstraintComponent() = default;

void plJoltDistanceConstraintComponent::SetMinDistance(float value)
{
  m_fMinDistance = value;
  QueueApplySettings();
}

void plJoltDistanceConstraintComponent::SetMaxDistance(float value)
{
  m_fMaxDistance = value;
  QueueApplySettings();
}

void plJoltDistanceConstraintComponent::SetFrequency(float value)
{
  m_fFrequency = value;
  QueueApplySettings();
}

void plJoltDistanceConstraintComponent::SetDamping(float value)
{
  m_fDamping = value;
  QueueApplySettings();
}

void plJoltDistanceConstraintComponent::ApplySettings()
{
  plJoltConstraintComponent::ApplySettings();

  JPH::DistanceConstraint* pConstraint = static_cast<JPH::DistanceConstraint*>(m_pConstraint);

  pConstraint->SetFrequency(m_fFrequency);
  pConstraint->SetDamping(m_fDamping);

  const float fMin = plMath::Max(0.0f, m_fMinDistance);
  const float fMax = plMath::Max(fMin, m_fMaxDistance);
  pConstraint->SetDistance(fMin, fMax);

  if (pConstraint->GetBody2()->IsInBroadPhase())
  {
    // wake up the bodies that are attached to this constraint
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->GetJoltSystem()->GetBodyInterface().ActivateBody(pConstraint->GetBody2()->GetID());
  }
}

bool plJoltDistanceConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::DistanceConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition() >= m_fBreakForce)
      {
        return true;
      }
    }
  }

  return false;
}

void plJoltDistanceConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fMinDistance;
  s << m_fMaxDistance;
  s << m_fFrequency;
  s << m_fDamping;
}

void plJoltDistanceConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_fMinDistance;
  s >> m_fMaxDistance;
  s >> m_fFrequency;
  s >> m_fDamping;
}

void plJoltDistanceConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::DistanceConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;
  opt.mMinDistance = 0;
  opt.mMaxDistance = 1;
  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);
  opt.mDamping = m_fDamping;
  opt.mFrequency = m_fFrequency;

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltDistanceConstraintComponent);
