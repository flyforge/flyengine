#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

using plSpatialAnchorComponentManager = plComponentManagerSimple<class plSpatialAnchorComponent, plComponentUpdateType::WhenSimulating>;

class PL_GAMEENGINE_DLL plSpatialAnchorComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSpatialAnchorComponent, plComponent, plSpatialAnchorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plSpatialAnchorComponent

public:
  plSpatialAnchorComponent();
  ~plSpatialAnchorComponent();

  /// \brief Attempts to create a new anchor at the given location.
  ///
  /// On failure, the existing anchor will continue to be used.
  /// On success, the new anchor will be used and the new location.
  plResult RecreateAnchorAt(const plTransform& position);

protected:
  void Update();

private:
  plXRSpatialAnchorID m_AnchorID;
};
