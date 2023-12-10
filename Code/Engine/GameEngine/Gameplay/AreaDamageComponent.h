#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

class plPhysicsWorldModuleInterface;
struct plPhysicsOverlapResult;

class PLASMA_GAMEENGINE_DLL plAreaDamageComponentManager : public plComponentManager<class plAreaDamageComponent, plBlockStorageType::FreeList>
{
  typedef plComponentManager<plAreaDamageComponent, plBlockStorageType::FreeList> SUPER;

public:
  plAreaDamageComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class plAreaDamageComponent;
  plPhysicsWorldModuleInterface* m_pPhysicsInterface;
};

class PLASMA_GAMEENGINE_DLL plAreaDamageComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAreaDamageComponent, plComponent, plAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plAreaDamageComponent

public:
  plAreaDamageComponent();
  ~plAreaDamageComponent();

  void ApplyAreaDamage(); // [ scriptable ]

  bool m_bTriggerOnCreation = true; // [ property ]
  plUInt8 m_uiCollisionLayer = 0;   // [ property ]
  float m_fRadius = 5.0f;           // [ property ]
  float m_fDamage = 10.0f;          // [ property ]
  float m_fImpulse = 100.0f;        // [ property ]
};
