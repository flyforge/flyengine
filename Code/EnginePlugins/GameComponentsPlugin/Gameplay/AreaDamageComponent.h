#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>

class plPhysicsWorldModuleInterface;
struct plPhysicsOverlapResult;

class PL_GAMECOMPONENTS_DLL plAreaDamageComponentManager : public plComponentManager<class plAreaDamageComponent, plBlockStorageType::FreeList>
{
  using SUPER = plComponentManager<plAreaDamageComponent, plBlockStorageType::FreeList>;

public:
  plAreaDamageComponentManager(plWorld* pWorld);

  virtual void Initialize() override;

private:
  friend class plAreaDamageComponent;
  plPhysicsWorldModuleInterface* m_pPhysicsInterface = nullptr;
};

/// \brief Used to apply damage to objects in the vicinity and push physical objects away.
///
/// The component queries for dynamic physics shapes within a given radius.
/// For all objects found it sends the messages plMsgPhysicsAddImpulse and plMsgDamage.
/// The former is used to apply a physical impulse, to push the objects away from the center of the explosion.
/// The second message is used to apply damage to the objects. This only has an effect, if those objects
/// handle that message type./// 
///
/// This component is mainly meant as an example how to make gameplay functionality, such as explosions.
/// If its functionality is insufficient for your use-case, write your own and take its code as inspiration.
class PL_GAMECOMPONENTS_DLL plAreaDamageComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAreaDamageComponent, plComponent, plAreaDamageComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

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
