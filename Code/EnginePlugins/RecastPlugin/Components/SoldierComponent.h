#pragma once

#include <GameEngine/AI/NpcComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plRecastWorldModule;
class plPhysicsWorldModuleInterface;
struct plAgentSteeringEvent;

typedef plComponentManagerSimple<class plSoldierComponent, plComponentUpdateType::WhenSimulating> plSoldierComponentManager;

class PLASMA_RECASTPLUGIN_DLL plSoldierComponent : public plNpcComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plSoldierComponent, plNpcComponent, plSoldierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;


  //////////////////////////////////////////////////////////////////////////
  // plSoldierComponent

public:
  plSoldierComponent();
  ~plSoldierComponent();

protected:
  void Update();

  void SteeringEventHandler(const plAgentSteeringEvent& e);

  enum class State
  {
    Idle,
    WaitingForPath,
    Walking,
    ErrorState,
  };

  State m_State = State::Idle;
  plComponentHandle m_hSteeringComponent;
};
