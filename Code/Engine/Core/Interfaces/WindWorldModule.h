#pragma once

#include <Core/World/WorldModule.h>

struct PLASMA_CORE_DLL plWindStrength
{
  using StorageType = plUInt8;

  enum Enum
  {
    Calm,
    LightBreple,
    GentleBreple,
    ModerateBreple,
    StrongBreple,
    Storm,
    WeakShockwave,
    MediumShockwave,
    StrongShockwave,
    ExtremeShockwave,

    Default = LightBreple
  };

  static float GetInMetersPerSecond(plWindStrength::Enum strength);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plWindStrength);

class PLASMA_CORE_DLL plWindWorldModuleInterface : public plWorldModule
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plWindWorldModuleInterface, plWorldModule);

protected:
  plWindWorldModuleInterface(plWorld* pWorld);

public:
  virtual plVec3 GetWindAt(const plVec3& vPosition) const = 0;

  /// \brief Computes a 'fluttering' wind motion orthogonal to an object direction.
  ///
  /// This is used to apply sideways or upwards wind forces on an object, such that it flutters in the wind,
  /// even when the wind is constant.
  ///
  /// \param vWind The sampled (and potentially boosted or clamped) wind value.
  /// \param vObjectDir The main direction of the object. For example the (average) direction of a tree branch, or the direction of a rope or cable. The flutter value will be orthogonal to the object direction and the wind direction. So when when blows sideways onto a branch, the branch would flutter upwards and downwards. For a rope hanging downwards, wind blowing against it would make it flutter sideways.
  /// \param fFlutterSpeed How fast the object shall flutter (frequency).
  /// \param uiFlutterRandomOffset A random number that adds an offset to the flutter, such that multiple objects next to each other will flutter out of phase.
  plVec3 ComputeWindFlutter(const plVec3& vWind, const plVec3& vObjectDir, float fFlutterSpeed, plUInt32 uiFlutterRandomOffset) const;
};
