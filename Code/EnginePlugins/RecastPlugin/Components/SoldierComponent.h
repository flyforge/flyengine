#pragma once

#include <RecastPlugin/Components/NpcComponent.h>
#include <RecastPlugin/RecastPluginDLL.h>

class plRecastWorldModule;
class plPhysicsWorldModuleInterface;
struct plAgentSteeringEvent;

using plSoldierComponentManager = plComponentManagerSimple<class plSoldierComponent, plComponentUpdateType::WhenSimulating>;

class PL_RECASTPLUGIN_DLL plSoldierComponent : public plNpcComponent
{
  PL_DECLARE_COMPONENT_TYPE(plSoldierComponent, plNpcComponent, plSoldierComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

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
