#pragma once

#include <GameEngine/GameEngineDLL.h>

using plXRSpatialAnchorID = plGenericId<32, 16>;

/// \brief XR spatial anchors interface.
///
/// Aquire interface via plSingletonRegistry::GetSingletonInstance<plXRSpatialAnchorsInterface>().
class plXRSpatialAnchorsInterface
{
public:
  /// \brief Creates a spatial anchor at the given world space position.
  /// Returns an invalid handle if anchors can't be created right now. Retry next frame.
  virtual plXRSpatialAnchorID CreateAnchor(const plTransform& globalTransform) = 0;

  /// \brief Destroys a previously created anchor.
  virtual plResult DestroyAnchor(plXRSpatialAnchorID id) = 0;

  /// \brief Tries to resolve the anchor position. Can fail of the anchor is invalid or tracking is
  /// currently lost.
  virtual plResult TryGetAnchorTransform(plXRSpatialAnchorID id, plTransform& out_globalTransform) = 0;
};
