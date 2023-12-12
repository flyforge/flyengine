#include <GameEngine/GameEnginePCH.h>

#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <GameEngine/Animation/TransformComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTransformComponent, 3, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Speed", m_fAnimationSpeed), // How many units per second the animation should do.
    PLASMA_ACCESSOR_PROPERTY("Running", IsRunning, SetRunning)->AddAttributes(new plDefaultValueAttribute(true)), // Whether the animation should start right away.
    PLASMA_ACCESSOR_PROPERTY("ReverseAtEnd", GetReverseAtEnd, SetReverseAtEnd)->AddAttributes(new plDefaultValueAttribute(true)), // If true, after coming back to the start point, the animation won't stop but turn around and continue.
    PLASMA_ACCESSOR_PROPERTY("ReverseAtStart", GetReverseAtStart, SetReverseAtStart)->AddAttributes(new plDefaultValueAttribute(true)), // If true, it will not stop at the end, but turn around and continue.
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Animation"),
    new plColorAttribute(plColorScheme::Animation),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetDirectionForwards, In, "Forwards"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(IsDirectionForwards),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(ToggleDirection),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plTransformComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  stream.GetStream() << m_Flags.GetValue();
  stream.GetStream() << m_AnimationTime;
  stream.GetStream() << m_fAnimationSpeed;
}


void plTransformComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  plTransformComponentFlags::StorageType flags;
  stream.GetStream() >> flags;
  m_Flags.SetValue(flags);

  stream.GetStream() >> m_AnimationTime;
  stream.GetStream() >> m_fAnimationSpeed;
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
  float fDistanceInMeters, float fAcceleration, float fMaxVelocity, float fDeceleration, plTime& fTimeSinceStartInSec)
{
  // linear motion, if no acceleration or deceleration is present
  if ((fAcceleration <= 0.0f) && (fDeceleration <= 0.0f))
  {
    const float fDist = fMaxVelocity * (float)fTimeSinceStartInSec.GetSeconds();

    if (fDist > fDistanceInMeters)
    {
      fTimeSinceStartInSec = plTime::Seconds(fDistanceInMeters / fMaxVelocity);
      return fDistanceInMeters;
    }

    return plMath::Max(0.0f, fDist);
  }

  // do some sanity-checks
  if ((fTimeSinceStartInSec.GetSeconds() <= 0.0) || (fMaxVelocity <= 0.0f) || (fDistanceInMeters <= 0.0f))
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
  if (fTimeSinceStartInSec.GetSeconds() <= fAccTime)
    return static_cast<float>(0.5 * fAcceleration * plMath::Square(fTimeSinceStartInSec.GetSeconds()));

  // calculate duration and length of the path, that has maximum velocity
  const double fMaxVelDistance = fDistanceInMeters - (fAccDist + fDecDist);
  const double fMaxVelTime = fMaxVelDistance / fMaxVelocity;

  // if the time is within this phase, return the accelerated path plus the constant velocity path
  if (fTimeSinceStartInSec.GetSeconds() <= fAccTime + fMaxVelTime)
    return static_cast<float>(fAccDist + (fTimeSinceStartInSec.GetSeconds() - fAccTime) * fMaxVelocity);

  // if the time is, however, outside the whole path, just return the upper end
  if (fTimeSinceStartInSec.GetSeconds() >= fAccTime + fMaxVelTime + fDecTime)
  {
    fTimeSinceStartInSec = plTime::Seconds(fAccTime + fMaxVelTime + fDecTime); // clamp the time
    return fDistanceInMeters;
  }

  // calculate the time into the decelerated movement
  const double fDecTime2 = fTimeSinceStartInSec.GetSeconds() - (fAccTime + fMaxVelTime);

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

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
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

  virtual void Patch(plGraphPatchContext& context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->RenameProperty("RunAtStartup", "Running");
  }
};

plTransformComponentPatch_2_3 g_plTransformComponentPatch_2_3;

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_Animation_Implementation_TransformComponent);
