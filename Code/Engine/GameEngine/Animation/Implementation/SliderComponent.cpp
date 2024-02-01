#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Animation/SliderComponent.h>

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, plTime& ref_timeSinceStartInSec);

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSliderComponent, 3, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ENUM_MEMBER_PROPERTY("Axis", plBasisAxis, m_Axis)->AddAttributes(new plDefaultValueAttribute((int)plBasisAxis::PositiveZ)),
    PL_MEMBER_PROPERTY("Distance", m_fDistanceToTravel)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("Acceleration", m_fAcceleration)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("Deceleration", m_fDeceleration)->AddAttributes(new plClampValueAttribute(0.0f, plVariant())),
    PL_MEMBER_PROPERTY("RandomStart", m_RandomStart)->AddAttributes(new plClampValueAttribute(plTime::MakeZero(), plVariant())),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plDirectionVisualizerAttribute("Axis", 1.0, plColor::MediumPurple, nullptr, "Distance")
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
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

    const float fNewDistance = CalculateAcceleratedMovement(m_fDistanceToTravel, m_fAcceleration, m_fAnimationSpeed, m_fDeceleration, m_AnimationTime);

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
  SUPER::OnSimulationStarted();

  // reset to start state
  m_fLastDistance = 0.0f;

  if (m_RandomStart.IsPositive())
  {
    m_AnimationTime = plTime::MakeFromSeconds(GetWorld()->GetRandomNumberGenerator().DoubleInRange(0.0, m_RandomStart.GetSeconds()));
  }
}

void plSliderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_fDistanceToTravel;
  s << m_fAcceleration;
  s << m_fDeceleration;
  s << m_Axis.GetValue();
  s << m_fLastDistance;
  s << m_RandomStart;
}


void plSliderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

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

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    // Base class
    ref_context.PatchBaseClass("plTransformComponent", 2, true);
  }
};

plSliderComponentPatch_1_2 g_plSliderComponentPatch_1_2;


PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_SliderComponent);
