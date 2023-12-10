#pragma once

#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/Declarations.h>

struct plXRHand
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Left = 0,
    Right,
    Default = Left
  };
};

struct plXRHandPart
{
  using StorageType = plUInt8;
  enum Enum : plUInt8
  {
    Palm = 0,
    Wrist,
    Thumb,
    Index,
    Middle,
    Ring,
    Little,
    COUNT,
    Default = Palm
  };
};

struct plXRHandBone
{
  plTransform m_Transform;
  float m_fRadius;
};

/// \brief XR Hand tracking interface.
///
/// Aquire interface via plSingletonRegistry::GetSingletonInstance<plXRHandTrackingInterface>().
class plXRHandTrackingInterface
{
public:
  enum class HandPartTrackingState
  {
    NotSupported, ///< The given hand part is not supported by this hand tracker implementation.
    Untracked,    ///< The given hand part is currently not tracked or tracking is lost. Retry next frame.
    Tracked,      ///< The given hand part is tracked and the bones array was filled successfully.
  };

  /// \brief Returns a array of bones in the given part of the hand.
  ///
  /// Bones are always ordered from furthest from the body moving inwards. E.g. for a finger index 0 should be the tip
  /// followed by distal, etc. ending in the wrist bone. Depending on the implementation the number
  /// of bones returned can be less than the number of actual bones in the hand.
  virtual HandPartTrackingState TryGetBoneTransforms(
    plEnum<plXRHand> hand, plEnum<plXRHandPart> handPart, plEnum<plXRTransformSpace> space, plDynamicArray<plXRHandBone>& out_bones) = 0;
};
