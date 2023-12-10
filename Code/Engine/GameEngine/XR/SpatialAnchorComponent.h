#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRSpatialAnchorsInterface.h>

//////////////////////////////////////////////////////////////////////////

typedef plComponentManagerSimple<class plSpatialAnchorComponent, plComponentUpdateType::WhenSimulating> plSpatialAnchorComponentManager;

class PLASMA_GAMEENGINE_DLL plSpatialAnchorComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSpatialAnchorComponent, plComponent, plSpatialAnchorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

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
