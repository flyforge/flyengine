#include <Core/World/Component.h>
#include <GameComponentsPlugin/GameComponentsDLL.h>
#include <GameEngine/AI/SensorComponent.h>

struct plMsgObjectGrabbed;
struct plMsgSensorDetectedObjectsChanged;

/// \brief This event is posted by plPowerConnectorComponent whenever the power input on a connector changes.
///
/// When a connector gets input through it's connection to another connector, this message is sent.
/// This can then be used by scripts on parent nodes to switch other functionality on or off.
/// Both the previous and new value are sent, so that the difference can be calculated.
/// This is useful in case a script has multiple connectors and needs to keep track of the total amount of power available.
class PL_GAMECOMPONENTS_DLL plEventMsgSetPowerInput : public plEventMessage
{
  PL_DECLARE_MESSAGE_TYPE(plEventMsgSetPowerInput, plEventMessage);

  plUInt16 m_uiPrevValue = 0;
  plUInt16 m_uiNewValue = 0;
};

using plPowerConnectorComponentManager = plComponentManager<class plPowerConnectorComponent, plBlockStorageType::Compact>;

/// \brief This component is for propagating the flow of power in cables or fluid in pipes and determine whether it arrives at a receiver.
///
/// This component is meant for building puzzles where you have to connect the right objects to power something.
/// It uses physics constraints to physically connect two pieces and have them snap together.
/// It also reacts to being grabbed (plMsgObjectGrabbed) to disconnect.
///
/// On its own this component doesn't do anything.
/// However, it can be set to be 'connected' to another object with an plPowerConnectorComponent, in which case it would propagate its own
/// 'output' as the 'input' on that component.
/// If its output is non-zero and thus the input on the connected component is also non-zero, the other component will post plEventMsgSetPowerInput,
/// to which a script can react and for example switch a light on.
///
/// Connectors are bi-directional ("full duplex"), so they can have both an input and an output and the two values are independent of each other.
/// That means power can flow in both or just one direction and therefore it is not important with which end a cable gets connected to something.
///
/// To enable building things like cables, each plPowerConnectorComponent can also have a 'buddy', which is an object on which another
/// plPowerConnectorComponent exists.
/// If a connector gets input, that input value is propagated to the buddy as its output value. Thus when a cable gets input on one end,
/// the other end (if it is properly set as the buddy) will output that value. So if that end is also 'connected' to something, the output will
/// be further propagated as the 'input' on that object. This can go through many hops until the value reaches the final connector (if you build a
/// circular chain it will stop when it reaches the starting point).
///
/// The component automatically connects to another object when it receives a plMsgSensorDetectedObjectsChanged, so it should have a child
/// object with a sensor. The sensor should use a dedicated spatial category to search for markers where it can connect.
///
/// To have a sensor (or other effects) only active when the connector is grabbed, put them in a child object with the name "ActiveWhenGrabbed"
/// and disable the object by default. The parent plPowerConnectorComponent will toggle the active flag of that object when it gets grabbed or let go.
///
/// To build a cable, don't forget to set each end as the 'buddy' of the other end.
class PL_GAMECOMPONENTS_DLL plPowerConnectorComponent : public plComponent
{
  PL_DECLARE_COMPONENT_TYPE(plPowerConnectorComponent, plComponent, plPowerConnectorComponentManager);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

protected:
  virtual void OnDeactivated() override;
  virtual void OnSimulationStarted() override;

  //////////////////////////////////////////////////////////////////////////
  // plPowerConnectorComponent

public:
  /// \brief Sets how much output (of whatever kind) this connector produces.
  ///
  /// If this is zero, it is either a receiver, or a pass-through connector, e.g. a cable, or just currently inactive.
  /// If this is non-zero, it acts like a source, and when another connector gets connected to it, that output will be propagated
  /// through the connection/buddy chain.
  void SetOutput(plUInt16 value);                   // [ property ]
  plUInt16 GetOutput() const { return m_uiOutput; } // [ property ]

  void SetBuddy(plGameObjectHandle hObject);
  void SetConnectedTo(plGameObjectHandle hObject);

  /// \brief Whether the connector is currently connected to another connector.
  bool IsConnected() const; // [ scriptable ]

  /// \brief Whether the connector is physically attached to another connector.
  bool IsAttached() const; // [ scriptable ]

  void Detach(); // [ scriptable ]
  void Attach(plGameObjectHandle hObject);

protected:
  void SetBuddyReference(const char* szReference);       // [ property ]
  void SetConnectedToReference(const char* szReference); // [ property ]

  void ConnectToSocket(plGameObjectHandle hSocket);

  void SetInput(plUInt16 value);

  /// \brief Whenever a plMsgSensorDetectedObjectsChanged arrives, the connector attempts to connect to the reported object.
  void OnMsgSensorDetectedObjectsChanged(plMsgSensorDetectedObjectsChanged& msg); // [ message handler ]

  /// \brief Whenever the connector gets grabbed, it detaches from its current connection.
  ///
  /// It also toggles the active flag of the child object with the name "ActiveWhenGrabbed".
  /// So to only have it connect to other connectors when grabbed, put the sensor component into such a child object.
  void OnMsgObjectGrabbed(plMsgObjectGrabbed& msg); // [ message handler ]


  plTime m_DetachTime;
  plGameObjectHandle m_hAttachPoint;
  plGameObjectHandle m_hGrabbedBy;

  plUInt16 m_uiOutput = 0;
  plUInt16 m_uiInput = 0;

  plGameObjectHandle m_hBuddy;
  plGameObjectHandle m_hConnectedTo;

  void InputChanged(plUInt16 uiPrevInput, plUInt16 uiInput);
  void OutputChanged(plUInt16 uiOutput);

private:
  const char* DummyGetter() const { return nullptr; }
};
