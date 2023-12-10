#pragma once

#include <Core/Input/Declarations.h>
#include <Core/Messages/EventMessage.h>
#include <Core/Messages/TriggerMessage.h>
#include <Core/World/World.h>
#include <GameEngine/GameEngineDLL.h>

typedef plComponentManagerSimple<class plInputComponent, plComponentUpdateType::WhenSimulating> plInputComponentManager;

/// \brief Which types of input events are broadcast
struct PLASMA_GAMEENGINE_DLL plInputMessageGranularity
{
  typedef plInt8 StorageType;

  /// \brief Which types of input events are broadcast
  enum Enum
  {
    PressOnly,           ///< Key pressed events are sent, but nothing else
    PressAndRelease,     ///< Key pressed and key released events are sent
    PressReleaseAndDown, ///< Key pressed and released events are sent, and while a key is down, another message is sent every frame as well

    Default = PressOnly
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plInputMessageGranularity);

/// \brief plInputComponent raises this event when it detects input
struct PLASMA_GAMEENGINE_DLL plMsgInputActionTriggered : public plEventMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgInputActionTriggered, plEventMessage);

  /// The input action string.
  plHashedString m_sInputAction;

  /// The 'trigger state', depending on the key state and the configuration on the plInputComponent
  plEnum<plTriggerState> m_TriggerState;

  /// For analog keys, how much they are pressed. Typically between 0 and 1.
  float m_fKeyPressValue;

private:
  const char* GetInputAction() const { return m_sInputAction; }
  void SetInputAction(const char* szInputAction) { m_sInputAction.Assign(szInputAction); }
};

/// \brief This component polls all input events from the given input set every frame and broadcasts the information to components on the same game
/// object.
///
/// To deactivate input handling, just deactivate the entire component.
/// To use the input data, add a message handler on another component and handle messages of type plTriggerMessage.
/// For every input event, one such message is sent every frame.
/// The granularity property defines for which input events (key pressed, released or down) messages are sent.
class PLASMA_GAMEENGINE_DLL plInputComponent : public plComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plInputComponent, plComponent, plInputComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& stream) const override;
  virtual void DeserializeComponent(plWorldReader& stream) override;


  //////////////////////////////////////////////////////////////////////////
  // plInputComponent

public:
  plInputComponent();
  ~plInputComponent();

  float GetCurrentInputState(const char* szInputAction, bool bOnlyKeyPressed = false) const; // [ scriptable ]

  plString m_sInputSet;                            // [ property ]
  plEnum<plInputMessageGranularity> m_Granularity; // [ property ]
  bool m_bForwardToBlackboard = false;             // [ property ]

protected:
  void Update();

  plEventMessageSender<plMsgInputActionTriggered> m_InputEventSender; // [ event ]
};
