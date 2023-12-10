#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Constraints/PointConstraint.h>
#include <JoltPlugin/Constraints/JoltPointConstraintComponent.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltPointConstraintComponent, 1, plComponentMode::Static)
{
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltPointConstraintComponent::plJoltPointConstraintComponent() = default;
plJoltPointConstraintComponent::~plJoltPointConstraintComponent() = default;

void plJoltPointConstraintComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  // auto& s = stream.GetStream();
}

void plJoltPointConstraintComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // auto& s = stream.GetStream();
}

void plJoltPointConstraintComponent::ApplySettings()
{
  SUPER::ApplySettings();
}

void plJoltPointConstraintComponent::CreateContstraintType(JPH::Body* pBody0, JPH::Body* pBody1)
{
  const auto inv1 = pBody0->GetInverseCenterOfMassTransform() * pBody0->GetWorldTransform();
  const auto inv2 = pBody1->GetInverseCenterOfMassTransform() * pBody1->GetWorldTransform();

  JPH::PointConstraintSettings opt;
  opt.mDrawConstraintSize = 0.1f;

  opt.mSpace = JPH::EConstraintSpace::LocalToBodyCOM;
  opt.mPoint1 = inv1 * plJoltConversionUtils::ToVec3(m_LocalFrameA.m_vPosition);
  opt.mPoint2 = inv2 * plJoltConversionUtils::ToVec3(m_LocalFrameB.m_vPosition);

  m_pConstraint = opt.Create(*pBody0, *pBody1);
}

bool plJoltPointConstraintComponent::ExceededBreakingPoint()
{
  if (auto pConstraint = static_cast<JPH::PointConstraint*>(m_pConstraint))
  {
    if (m_fBreakForce > 0)
    {
      if (pConstraint->GetTotalLambdaPosition().ReduceMax() >= m_fBreakForce)
      {
        return true;
      }
    }
  }

  return false;
}

PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Constraints_Implementation_JoltPointConstraintComponent);
