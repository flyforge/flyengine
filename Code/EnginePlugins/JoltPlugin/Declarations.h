#pragma once

#include <Foundation/Communication/Message.h>
#include <Foundation/Math/Vec3.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Enum.h>
#include <JoltPlugin/JoltPluginDLL.h>

class plJoltActorComponent;

struct plJoltSteppingMode
{
  using StorageType = plUInt32;

  enum Enum
  {
    Variable,
    Fixed,
    SemiFixed,

    Default = SemiFixed
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plJoltSteppingMode);

//////////////////////////////////////////////////////////////////////////

/// \brief Flags for what should happen when two physical bodies touch.
///
/// The reactions need to be set up through plSurface's.
/// For most objects only some reactions make sense.
/// For example a box may hit another object as well as slide, but it cannot roll.
/// A barrel can impact and slide on some sides, but roll around its up axis (Z).
/// A sphere can impact and roll around all its axis, but never slide.
/// A soft object may not have any impact reactions.
struct plOnJoltContact
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    // SendReportMsg = PLASMA_BIT(0),
    ImpactReactions = PLASMA_BIT(1), ///< Spawn prefabs for impacts (two objects hit each other with enough force).
    SlideReactions = PLASMA_BIT(2),  ///< Spawn prefabs for sliding (one object slides along the surface of another).
    RollXReactions = PLASMA_BIT(3),  ///< Spawn prefabs for rolling (one object rotates around its X axis while touching another).
    RollYReactions = PLASMA_BIT(4),  ///< Spawn prefabs for rolling (one object rotates around its Y axis while touching another).
    RollZReactions = PLASMA_BIT(5),  ///< Spawn prefabs for rolling (one object rotates around its Z axis while touching another).

    AllRollReactions = RollXReactions | RollYReactions | RollZReactions,
    SlideAndRollReactions = AllRollReactions | SlideReactions,
    AllReactions = ImpactReactions | AllRollReactions | SlideReactions,

    Default = None
  };

  struct Bits
  {
    StorageType SendReportMsg : 1;
    StorageType ImpactReactions : 1;
    StorageType SlideReactions : 1;
    StorageType RollXReactions : 1;
    StorageType RollYReactions : 1;
    StorageType RollZReactions : 1;
  };
};

PLASMA_DECLARE_FLAGS_OPERATORS(plOnJoltContact);
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_JOLTPLUGIN_DLL, plOnJoltContact);

//////////////////////////////////////////////////////////////////////////

struct plJoltSettings
{
  plVec3 m_vObjectGravity = plVec3(0, 0, -9.81f);
  plVec3 m_vCharacterGravity = plVec3(0, 0, -12.0f);

  plEnum<plJoltSteppingMode> m_SteppingMode = plJoltSteppingMode::SemiFixed;
  float m_fFixedFrameRate = 60.0f;
  plUInt32 m_uiMaxSubSteps = 4;

  plUInt32 m_uiMaxBodies = 1000 * 10;
};

//////////////////////////////////////////////////////////////////////////

/// \brief This message can be sent to a constraint component to break the constraint.
struct PLASMA_JOLTPLUGIN_DLL plJoltMsgDisconnectConstraints : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plJoltMsgDisconnectConstraints, plMessage);

  /// The actor that is being deleted. All constraints that are linked to it must be removed for Jolt not to crash.
  plJoltActorComponent* m_pActor = nullptr;

  /// The ID of the Jolt body that is being removed. If an actor were to have multiple bodies, this message may be sent multiple times.
  plUInt32 m_uiJoltBodyID = plInvalidIndex;
};
