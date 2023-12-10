#pragma once

#include <Core/Interfaces/WindWorldModule.h>
#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

typedef plComponentManagerSimple<class plSimpleWindComponent, plComponentUpdateType::WhenSimulating> plSimpleWindComponentManager;

class PLASMA_GAMEENGINE_DLL plSimpleWindComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSimpleWindComponent, plComponent, plSimpleWindComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void Initialize() override;
  virtual void OnActivated() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plSimpleWindComponent

public:
  plSimpleWindComponent();
  ~plSimpleWindComponent();

  plEnum<plWindStrength> m_MinWindStrength; // [ property ]
  plEnum<plWindStrength> m_MaxWindStrength; // [ property ]

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
