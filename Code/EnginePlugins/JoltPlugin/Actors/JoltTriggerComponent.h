#pragma once

#include <Core/Messages/TriggerMessage.h>
#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltTriggerComponentManager : public plComponentManager<class plJoltTriggerComponent, plBlockStorageType::FreeList>
{
public:
  plJoltTriggerComponentManager(plWorld* pWorld);
  ~plJoltTriggerComponentManager();

private:
  friend class plJoltWorldModule;
  friend class plJoltTriggerComponent;

  void UpdateMovingTriggers();

  plSet<plJoltTriggerComponent*> m_MovingTriggers;
};

//////////////////////////////////////////////////////////////////////////

class PLASMA_JOLTPLUGIN_DLL plJoltTriggerComponent : public plJoltActorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltTriggerComponent, plJoltActorComponent, plJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

public:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltTriggerComponent
public:
  plJoltTriggerComponent();
  ~plJoltTriggerComponent();

  void SetTriggerMessage(const char* szSz) { m_sTriggerMessage.Assign(szSz); }  // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class plJoltWorldModule;
  friend class plJoltContactListener;

  void PostTriggerMessage(const plGameObjectHandle& hOtherObject, plTriggerState::Enum triggerState) const;

  plHashedString m_sTriggerMessage;
  plEventMessageSender<plMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
