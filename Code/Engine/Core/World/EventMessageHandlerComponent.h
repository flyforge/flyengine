#pragma once

#include <Core/Messages/EventMessage.h>
#include <Core/World/World.h>

struct plEventMessage;

/// \brief Base class for components that want to handle 'event messages'
///
/// Event messages are messages that are 'broadcast' to indicate something happened on a component,
/// e.g. a trigger that got activated or an animation that finished playing. These messages are 'bubbled up'
/// the object hierarchy to the closest parent object that holds an plEventMessageHandlerComponent.
class PLASMA_CORE_DLL plEventMessageHandlerComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plEventMessageHandlerComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;

protected:
  virtual void Deinitialize() override;


  //////////////////////////////////////////////////////////////////////////
  // plEventMessageHandlerComponent

public:
  /// \brief Keep the constructor private or protected in derived classes, so it cannot be called manually.
  plEventMessageHandlerComponent();
  ~plEventMessageHandlerComponent();

  /// \brief Sets the debug output object flag. The effect is type specific, most components will not do anything different.
  void SetDebugOutput(bool enable);

  /// \brief Gets the debug output object flag.
  bool GetDebugOutput() const;

  /// \brief Registers or de-registers this component as a global event handler.
  void SetGlobalEventHandlerMode(bool enable); // [ property ]

  /// \brief Returns whether this component is registered as a global event handler.
  bool GetGlobalEventHandlerMode() const { return m_bIsGlobalEventHandler; } // [ property ]

  /// \brief Sets whether unhandled event messages should be passed to parent objects or not.
  void SetPassThroughUnhandledEvents(bool bPassThrough);                               // [ property ]
  bool GetPassThroughUnhandledEvents() const { return m_bPassThroughUnhandledEvents; } // [ property ]

  /// \brief Returns all global event handler for the given world.
  static plArrayPtr<plComponentHandle> GetAllGlobalEventHandler(const plWorld* pWorld);

  static void ClearGlobalEventHandlersForWorld(const plWorld* pWorld);

private:
  bool m_bDebugOutput = false;
  bool m_bIsGlobalEventHandler = false;
  bool m_bPassThroughUnhandledEvents = false;
};
