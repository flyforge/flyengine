#pragma once

#include <Core/World/SettingsComponentManager.h>
#include <GameEngine/GameEngineDLL.h>
#include <GameEngine/XR/XRInterface.h>

//////////////////////////////////////////////////////////////////////////

using plStageSpaceComponentManager = plSettingsComponentManager<class plStageSpaceComponent>;

/// \brief Singleton to set the type of stage space and its global transform in the world.
///
/// The global transform of the owner and the set stage space are read out by the XR
/// implementation every frame.
class PL_GAMEENGINE_DLL plStageSpaceComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plStageSpaceComponent, plComponent, plStageSpaceComponentManager);

public:
  plStageSpaceComponent();
  ~plStageSpaceComponent();

  //
  // plDeviceTrackingComponent Interface
  //

  /// \brief Sets the stage space used by the XR experience.
  void SetStageSpace(plEnum<plXRStageSpace> space);
  plEnum<plXRStageSpace> GetStageSpace() const;

protected:
  //
  // plComponent Interface
  //
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

private:
  plEnum<plXRStageSpace> m_Space;
};
