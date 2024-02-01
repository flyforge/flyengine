#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTransformComponent, 3, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    PL_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new plDefaultValueAttribute(true)), // Whether the animation should start right away.
    PL_ACCESSOR_PROPERTY("ReverseAtEnd", GetReverseAtEnd, SetReverseAtEnd)->AddAttributes(new plDefaultValueAttribute(true)), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    PL_ACCESSOR_PROPERTY("ReverseAtStart", GetReverseAtStart, SetReverseAtStart)->AddAttributes(new plDefaultValueAttribute(true)), // If true, it will not stop at the end, but turn around and continue.
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    PL_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    PL_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plTransformComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  inout_stream.GetStream() << m_Flags.GetValue();
  inout_stream.GetStream() << m_AnimationTime;
  inout_stream.GetStream() << m_fAnimationSpeed;
}


void plTransformComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  plTransformComponentFlags::StorageType flags;
  inout_stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  inout_stream.GetStream() >> m_AnimationTime;
  inout_stream.GetStream() >> m_fAnimationSpeed;
}

void plTransformComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  // reset to start state
  m_AnimationTime = plTime::MakeZero();
  m_Flags.Add(plTransformComponentFlags::Running);
  m_Flags.Remove(plTransformComponentFlags::AnimationReversed);
}

bool plTransformComponent::IsRunning(void) const
{
  return m_Flags.IsAnySet(plTransformComponentFlags::Running);
}

void plTransformComponent::SetRunning(bool b)
{
  m_Flags.AddOrRemove(plTransformComponentFlags::Running, b);
}

bool plTransformComponent::GetReverseAtStart(void) const
{
  return (m_Flags.IsAnySet(plTransformComponentFlags::AutoReturnStart));
}

void plTransformComponent::SetReverseAtStart(bool b)
{
  m_Flags.AddOrRemove(plTransformComponentFlags::AutoReturnStart, b);
}

bool plTransformComponent::GetReverseAtEnd(void) const
{
  return (m_Flags.IsAnySet(plTransformComponentFlags::AutoReturnEnd));
}

void plTransformComponent::SetReverseAtEnd(bool b)
{
  m_Flags.AddOrRemove(plTransformComponentFlags::AutoReturnEnd, b);
}

plTransformComponent::plTransformComponent() = default;
plTransformComponent::~plTransformComponent() = default;

void plTransformComponent::SetDirectionForwards(bool bForwards)
{
  m_Flags.AddOrRemove(plTransformComponentFlags::AnimationReversed, !bForwards);
}

void plTransformComponent::ToggleDirection()
{
  m_Flags.AddOrRemove(plTransformComponentFlags::AnimationReversed, !m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed));
}

bool plTransformComponent::IsDirectionForwards() const
{
  return !m_Flags.IsAnySet(plTransformComponentFlags::AnimationReversed);
}

/*! Distance should be given in meters, but can be anything else, too. E.g. "angles" or "radians". All other values need to use the same
units. For example, when distance is given in angles, acceleration has to be in "angles per square seconds". Deceleration can be positive or
negative, internally the absolute value is used. Distance, acceleration, max velocity and time need to be positive. Time is expected to be
"in seconds". The returned value is 0, if time is negative. It is clamped to fDistanceInMeters, if time is too big.
*/

float CalculateAcceleratedMovement(
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, plTime& ref_timeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)ref_timeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      ref_timeSinceStartInSec = plTime::MakeFromSeconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return plMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((ref_timeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
    return 0.0f;

  // calculate the duration and distance of accelerated movement
  double fAccTime = 0.0;
  if (fAcceleration > 0.0)
    fAccTime = fMaxVelocity / fAcceleration;
  double fAccDist = fMaxVelocity * fAccTime * 0.5;

  // calculate the duration and distance of decelerated movement
  double fDecTime = 0.0f;
  if (fDeceleration > 0.0f)
    fDecTime = fMaxVelocity / fDeceleration;
  double fDecDist = fMaxVelocity * fDecTime * 0.5f;

  // if acceleration and deceleration take longer, than the whole path is long
  if (fAccDist + fDecDist > fDistanceInMeters)
  {
    double fFactor = fDistanceInMeters / (fAccDist + fDecDist);

    // shorten the acceleration path
    if (fAcceleration > 0.0f)
    {
      fAccDist *= fFactor;
      fAccTime = plMath::Sqrt(2 * fAccDist / fAcceleration);
    }

    // shorten the deceleration path
    if (fDeceleration > 0.0f)
    {
      fDecDist *= fFactor;
      fDecTime = plMath::Sqrt(2 * fDecDist / fDeceleration);
    }
  }

  // if the time is still within the acceleration phase, return accelerated distance
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime)
    return static_cast<float>(0.5 * fAcceleration * plMath::Square(ref_timeSinceStartInSec.GetSeconds()));

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (ref_timeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return static_cast<float>(fAccDist + (ref_timeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity);

  // if the time is, however, outside the whole path, just return the upper end
  if (ref_timeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    ref_timeSinceStartInSec = plTime::MakeFromSeconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = ref_timeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

  // return the distance with the decelerated movement
  return static_cast<float>(fDistanceInMeters - 0.5 * fDeceleration * plMath::Square(fDecTime - fDecTime2));
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plTransformComponentPatch_1_2 : public plGraphPatch
{
public:
  plTransformComponentPatch_1_2()
    : plGraphPatch("plTransformComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("Run at Startup", "RunAtStartup");
    pNode->RenameProperty("Reverse at Start", "ReverseAtStart");
    pNode->RenameProperty("Reverse at End", "ReverseAtEnd");
  }
};

plTransformComponentPatch_1_2 g_plTransformComponentPatch_1_2;

//////////////////////////////////////////////////////////////////////////

class plTransformComponentPatch_2_3 : public plGraphPatch
{
public:
  plTransformComponentPatch_2_3()
    : plGraphPatch("plTransformComponent", 3)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

plTransformComponentPatch_2_3 g_plTransformComponentPatch_2_3;

PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_TransformComponent);
