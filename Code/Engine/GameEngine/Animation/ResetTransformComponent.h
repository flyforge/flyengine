#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using plResetTransformComponentManager = plComponentManager<class plResetTransformComponent, plBlockStorageType::Compact>;

/// \brief This component sets the local transform of its owner to known values when the simulation starts.
///
/// This component is meant for use cases where an object may be activated and deactivated over and over.
/// For example due to a state machine switching between different object states by (de-)activating a sub-tree of objects.
///
/// Every time an object becomes active, it may want to start moving again from a fixed location.
/// This component helps with that, by reseting the local transform of its owner to such a fixed location once.
///
/// After that, it does nothing else, until it gets deactivated and reactivated again.
class PL_GAMEENGINE_DLL plResetTransformComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plResetTransformComponent, plComponent, plResetTransformComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plResetTransformComponent

public:
  plResetTransformComponent();
  ~plResetTransformComponent();

  plVec3 m_vLocalPosition = plVec3::MakeZero();
  plQuat m_qLocalRotation = plQuat::MakeIdentity();
  plVec3 m_vLocalScaling = plVec3(1, 1, 1);
  float m_fLocalUniformScaling = 1.0f;

  bool m_bResetLocalPositionX = true;
  bool m_bResetLocalPositionY = true;
  bool m_bResetLocalPositionZ = true;
  bool m_bResetLocalRotation = true;
  bool m_bResetLocalScaling = true;
};
