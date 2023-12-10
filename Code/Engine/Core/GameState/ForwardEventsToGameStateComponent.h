#pragma once

#include <Core/World/Component.h>
#include <Core/World/ComponentManager.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>

using plForwardEventsToGameStateComponentManager = plComponentManager<class plForwardEventsToGameStateComponent, plBlockStorageType::Compact>;

/// \brief This event handler component forwards any message that it receives to the active plGameStateBase.
///
/// Game states can have message handlers just like any other reflected type.
/// However, since they are not part of the plWorld, messages are not delivered to them.
/// By attaching this component to a game object, all event messages that arrive at that node are
/// forwarded to the active game state. This way, a game state can receive information, such as
/// when a trigger gets activated.
///
/// Multiple of these components can exist in a scene, gathering and forwarding messages from many
/// different game objects, so that the game state can react to many different things.
class PLASMA_CORE_DLL plForwardEventsToGameStateComponent : public plEventMessageHandlerComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plForwardEventsToGameStateComponent, plEventMessageHandlerComponent, plForwardEventsToGameStateComponentManager);

public:
  //////////////////////////////////////////////////////////////////////////
  // plForwardEventsToGameStateComponent

public:
  plForwardEventsToGameStateComponent();
  ~plForwardEventsToGameStateComponent();

protected:
  virtual bool HandlesMessage(const plMessage& msg) const override;
  virtual bool OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) override;
  virtual bool OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) const override;

  virtual void Initialize() override;
};
