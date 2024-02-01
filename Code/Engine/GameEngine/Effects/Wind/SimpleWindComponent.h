#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

using plSimpleWindComponentManager = plComponentManagerSimple<class plSimpleWindComponent, plComponentUpdateType::WhenSimulating>;

/// \brief Calculates one global wind force using a very basic formula.
///
/// This component computes a wind vector that varies between a minimum and maximum strength
/// and around a certain direction.
///
/// Sets up the plSimpleWindWorldModule as the implementation of the plWindWorldModuleInterface.
///
/// When sampling the wind through this interface, the returned value is the same at every location.
///
/// Use a single instance of this component in a scene, when you need wind values, e.g. to make cloth and ropes sway,
/// but don't need a complex wind simulation.
class PL_GAMEENGINE_DLL plSimpleWindComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSimpleWindComponent, plComponent, plSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;

  //////////////////////////////////////////////////////////////////////////
  // plSimpleWindComponent

public:
  plSimpleWindComponent();
  ~plSimpleWindComponent();

  /// The minimum speed that the wind should always blow with.
  plEnum<plWindStrength> m_MinWindStrength; // [ property ]

  /// The maximum speed that the wind should blow with.
  plEnum<plWindStrength> m_MaxWindStrength; // [ property ]

  /// The wind blows in the positive X direction of the game object.
  /// The direction may deviate this much from that direction. Set to 180 degree to remove the limit.
  plAngle m_Deviation; // [ property ]

protected:
  void Update();
  void ComputeNextState();

  float m_fLastStrength = 0;
  float m_fNextStrength = 0;
  plVec3 m_vLastDirection;
  plVec3 m_vNextDirection;
  plTime m_LastChange;
  plTime m_NextChange;
};
