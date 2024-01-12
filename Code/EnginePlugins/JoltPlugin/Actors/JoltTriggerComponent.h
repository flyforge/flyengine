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

/// \brief Turns an object into a trigger that is capable of detecting when other physics objects enter its volume.
///
/// Triggers are physics actors and thus are set up the same way, e.g. they use physics shapes for their geometry,
/// but they act very differently. Triggers do not affect other objects, instead all objects just pass through them.
/// However, the trigger detects overlap with other objects and sends a message when a new object enters its volume
/// or when one leaves it.
///
/// \note The physics trigger only sends enter and leave messages. It does not send any message when an object stays inside
/// the trigger.
///
/// The message plMsgTriggerTriggered is sent for every change. It references the object that entered or left the volume
/// and it also contains a trigger-specific message string to identify what this should be used for.
class PLASMA_JOLTPLUGIN_DLL plJoltTriggerComponent : public plJoltActorComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plJoltTriggerComponent, plJoltActorComponent, plJoltTriggerComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnSimulationStarted() override;
  virtual void OnDeactivated() override;


  //////////////////////////////////////////////////////////////////////////
  // plJoltTriggerComponent
public:
  plJoltTriggerComponent();
  ~plJoltTriggerComponent();

  /// \brief Sets the text that the plMsgTriggerTriggered should contain when the trigger fires.
  void SetTriggerMessage(const char* szSz) { m_sTriggerMessage.Assign(szSz); }  // [ property ]
  const char* GetTriggerMessage() const { return m_sTriggerMessage.GetData(); } // [ property ]

protected:
  friend class plJoltWorldModule;
  friend class plJoltContactListener;

  void PostTriggerMessage(const plGameObjectHandle& hOtherObject, plTriggerState::Enum triggerState) const;

  plHashedString m_sTriggerMessage;
  plEventMessageSender<plMsgTriggerTriggered> m_TriggerEventSender; // [ event ]
};
