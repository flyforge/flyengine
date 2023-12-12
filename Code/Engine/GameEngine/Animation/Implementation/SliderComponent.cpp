#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SliderComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, plTime& fTimeSinceStartInSec);

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSliderComponent, 3, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveZ)),
    PLASMA_MEMBER_PROPERTY("Distance", m_fDistanceToTravel)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_MEMBER_PROPERTY("RandomStart", m_RandomStart)->AddAttributes(new plClampValueAttribute(plTime::Zero(), plVariant())),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute("Axis", 1.0, plColor::MediumPurple, nullptr, "Distance")
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSliderComponent::plSliderComponent() = default;
plSliderComponent::~plSliderComponent() = default;

void plSliderComponent::Update()
{
  if (m_Flags.IsAnySet(plTransformComponentFlags::Running))
  {
    plVec3 vAxis;

    switch (m_Axis)
    {
      case plBasisAxis::PositiveX:
        vAxis.Set(1, 0, 0);
        break;
      case plBasisAxis::PositiveY:
        vAxis.Set(0, 1, 0);
        break;
      case plBasisAxis::PositiveZ:
        vAxis.Set(0, 0, 1);
        break;
      case plBasisAxis::NegativeX:
        vAxis.Set(-1, 0, 0);
        break;
      case plBasisAxis::NegativeY:
        vAxis.Set(0, -1, 0);
        break;
      case plBasisAxis::NegativeZ:
        vAxis.Set(0, 0, -1);
        break;
    }

    if (m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed))
      m_AnimationTime -= GetWorld()->GetClock().GetTimeDiff();
    else
      m_AnimationTime += GetWorld()->GetClock().GetTimeDiff();

    const float fNewDistance =
      CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

    const float fDistanceDiff = fNewDistance - m_fLastDistance;

    GetOwner()->SetLocalPosition(GetOwner()->GetLocalPosition() + GetOwner()->GetLocalRotation() * vAxis * fDistanceDiff);

    m_fLastDistance = fNewDistance;

    if (!m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed))
    {
      if (fNewDistance >= m_fDistanceToTravel)
      {
        if (!m_Flags.IsSet(plTransformComponentFlags::AutoReturnEnd))
        {
          m_Flags.Remove(plTransformComponentFlags::Running);
        }

        m_Flags.Add(plTransformComponentFlags::AnimationReversed);

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

        // if (PrepareEvent("ANIMATOR_OnReachStart"))
        // RaiseEvent();
      }
    }
  }
}



void plSliderComponent::OnSimulationStarted()
{
  if (m_RandomStart.IsPositive())
  {
    m_AnimationTime = plTime::Seconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomStart.GetSeconds()));
  }
}

void plSliderComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
  s << m_RandomStart;
}


void plSliderComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_fDistanceToTravel;
  s >> m_fAcceleration;
  s >> m_fDeceleration;
  s >> m_Axis;
  s >> m_fLastDistance;

  if (uiVersion >= 3)
  {
    s >> m_RandomStart;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plSliderComponentPatch_1_2 : public plGraphPatch
{
public:
  plSliderComponentPatch_1_2()
    : plGraphPatch("plSliderComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // Base class
    context.PatchBaseClass("plTransformComponent", 2, true);
  }
};

plSliderComponentPatch_1_2 g_plSliderComponentPatch_1_2;


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SliderComponent);
