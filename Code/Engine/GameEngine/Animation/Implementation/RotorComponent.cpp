#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/RotorComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, plTime& fTimeSinceStartInSec);

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRotorComponent, 3, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis),
    PLASMA_MEMBER_PROPERTY("AxisDeviation", m_AxisDeviation)->AddAttributes(new plClampValueAttribute(plAngle::Degree(-180), plAngle::Degree(180))),
    PLASMA_MEMBER_PROPERTY("DegreesToRotate", m_iDegreeToRotate),
    PLASMA_MEMBER_PROPERTY("Acceleration", m_fAcceleration),
    PLASMA_MEMBER_PROPERTY("Deceleration", m_fDeceleration),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plRotorComponent::plRotorComponent() = default;
plRotorComponent::~plRotorComponent() = default;

void plRotorComponent::Update()
{
  if (m_Flags.IsAnySet(plTransformComponentFlags::Running) && m_fAnimationSpeed > 0.0f)
  {
    if (m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    if (m_iDegreeToRotate > 0)
    {
      const float fNewDistance =
        CalculateAcceleratedMovement((float)m_iDegreeToRotate, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

      plQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, plAngle::Degree(fNewDistance));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * -m_qLastRotation * qRotation);

      m_qLastRotation = qRotation;

      if (!m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed))
      {
        if (fNewDistance >= m_iDegreeToRotate)
        {
          if (!m_Flags.IsSet(plTransformComponentFlags::AutoReturnEnd))
          {
            m_Flags.Remove(plTransformComponentFlags::Running);
          }

          m_Flags.Add(plTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachEnd"))
          // RaiseEvent();
        }
      }
      else
      {
        if (fNewDistance <= 0.0f)
        {
          if (!m_Flags.IsSet(plTransformComponentFlags::AutoReturnStart))
          {
            m_Flags.Remove(plTransformComponentFlags::Running);
          }

          m_Flags.Remove(plTransformComponentFlags::AnimationReversed);

          /// \todo Scripting integration
          // if (PrepareEvent("ANIMATOR_OnReachStart"))
          // RaiseEvent();
        }
      }
    }
    else
    {
      /// \todo This will probably give precision issues pretty quickly

      plQuat qRotation;
      qRotation.SetFromAxisAndAngle(m_vRotationAxis, plAngle::Degree(m_fAnimationSpeed * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds()));

      GetOwner()->SetLocalRotation(GetOwner()->GetLocalRotation() * qRotation);
    }
  }
}

void plRotorComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_iDegreeToRotate;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_qLastRotation;
  s << m_AxisDeviation;
}


void plRotorComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_iDegreeToRotate;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_qLastRotation;

  if (uiVersion >= 3)
  {
    s >> m_AxisDeviation;
  }
}

void plRotorComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  switch (m_Axis)
  {
    case plBasisAxis::PositiveX:
      m_vRotationAxis.Set(1, 0, 0);
      break;
    case plBasisAxis::PositiveY:
      m_vRotationAxis.Set(0, 1, 0);
      break;
    case plBasisAxis::PositiveZ:
      m_vRotationAxis.Set(0, 0, 1);
      break;
    case plBasisAxis::NegativeX:
      m_vRotationAxis.Set(-1, 0, 0);
      break;
    case plBasisAxis::NegativeY:
      m_vRotationAxis.Set(0, -1, 0);
      break;
    case plBasisAxis::NegativeZ:
      m_vRotationAxis.Set(0, 0, -1);
      break;
  }

  if (m_AxisDeviation.GetRadian() != 0.0f)
  {
    if (m_AxisDeviation > plAngle::Degree(179))
    {
      m_vRotationAxis = plVec3::CreateRandomDirection(GetWorld()->GetRandomNumberGenerator());
    }
    else
    {
      m_vRotationAxis = plVec3::CreateRandomDeviation(GetWorld()->GetRandomNumberGenerator(), m_AxisDeviation, m_vRotationAxis);

      if (m_AxisDeviation.GetRadian() > 0 && GetWorld()->GetRandomNumberGenerator().Bool())
        m_vRotationAxis = -m_vRotationAxis;
    }
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plRotorComponentPatch_1_2 : public plGraphPatch
{
public:
  plRotorComponentPatch_1_2()
    : plGraphPatch("plRotorComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // Base class
    context.PatchBaseClass("plTransformComponent", 2, true);

    // this class
    pNode->RenameProperty("Degrees to Rotate", "DegreesToRotate");
  }
};

plRotorComponentPatch_1_2 g_plRotorComponentPatch_1_2;


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_RotorComponent);
