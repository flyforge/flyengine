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

struct plOnJoltContact
{
  using StorageType = plUInt32;

  enum Enum
  {
    None = 0,
    // SendReportMsg = PLASMA_BIT(0),
    ImpactReactions = PLASMA_BIT(1),
    SlideReactions = PLASMA_BIT(2),
    RollXReactions = PLASMA_BIT(3),
    RollYReactions = PLASMA_BIT(4),
    RollZReactions = PLASMA_BIT(5),

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

struct PLASMA_JOLTPLUGIN_DLL plJoltMsgDisconnectConstraints : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plJoltMsgDisconnectConstraints, plMessage);

  /// The actor that is being deleted. All constraints that are linked to it must be removed for Jolt not to crash.
  plJoltActorComponent* m_pActor = nullptr;

  /// The ID of the Jolt body that is being removed. If an actor were to have multiple bodies, this message may be sent multiple times.
  plUInt32 m_uiJoltBodyID = 0;
};