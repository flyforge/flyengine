#pragma once

#include <Core/World/WorldModule.h>

/// \brief Defines the strength / speed of wind. Inspired by the Beaufort Scale.
///
/// See https://en.wikipedia.org/wiki/Beaufort_scale
struct PL_CORE_DLL plWindStrength
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

  /// \brief Maps the wind strength name to a meters per second speed value as defined by the Beaufort Scale.
  ///
  /// The value only defines how fast wind moves, how much it affects an object, like bending it, depends
  /// on additional factors like stiffness and is thus object specific.
  static float GetInMetersPerSecond(plWindStrength::Enum strength);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plWindStrength);

class PL_CORE_DLL plWindWorldModuleInterface : public plWorldModule
{
  PL_ADD_DYNAMIC_REFLECTION(plWindWorldModuleInterface, plWorldModule);

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
  /// \param vObjectDir The main direction of the object. For example the (average) direction of a tree branch, or the direction of a rope or cable. The flutter value will be orthogonal to the object direction and the wind direction. So when wind blows sideways onto a branch, the branch would flutter upwards and downwards. For a rope hanging downwards, wind blowing against it would make it flutter sideways.
  /// \param fFlutterSpeed How fast the object shall flutter (frequency).
  /// \param uiFlutterRandomOffset A random number that adds an offset to the flutter, such that multiple objects next to each other will flutter out of phase.
  plVec3 ComputeWindFlutter(const plVec3& vWind, const plVec3& vObjectDir, float fFlutterSpeed, plUInt32 uiFlutterRandomOffset) const;
};
