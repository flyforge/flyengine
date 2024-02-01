#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Threading/ThreadSignal.h>
#include <Foundation/Types/UniquePtr.h>

class plIpcChannel;
class plMessageLoop;

/// \brief Event data for plIpcChannel::m_Events
struct PL_FOUNDATION_DLL plIpcChannelEvent
{
  enum Type
  {
    Disconnected, ///< Server or client are in a dorment state.
    Connecting,   ///< The server is listening for clients or the client is trying to find the server.
    Connected,    ///< Client and server are connected to each other.
    NewMessages,  ///< Sent when a new messages have been received or when disconnected to wake up any thread waiting for messages.
  };

  plIpcChannelEvent() = default;

  plIpcChannelEvent(Type type, plIpcChannel* pChannel)
    : m_Type(type)
    , m_pChannel(pChannel)
  {
  }

  Type m_Type = NewMessages;
  plIpcChannel* m_pChannel = nullptr;
};



/// \brief Base class for a communication channel between processes.
///
///  The channel allows for byte blobs to be send back and forth between two processes.
///  A client should only try to connect to a server once the server has changed to ConnectionState::Connecting as this indicates the server is ready to be conneccted to.
///
///  Use plIpcChannel:::CreatePipeChannel to create an IPC pipe instance.
///  To send more complex messages accross, you can create a plIpcProcessMessageProtocol on top of the channel.
class PL_FOUNDATION_DLL plIpcChannel
{
public:
  struct Mode
  {
    using StorageType = plUInt8;
    enum Enum
    {
      Server,
      Client,
      Default = Server
    };
  };

  struct ConnectionState
  {
    using StorageType = plUInt8;
    enum Enum
    {
      Disconnected,
      Connecting, ///< In case of the server, this state indicates that the server is ready to be connected to.
      Connected,
      Default = Disconnected
    };
  };

  virtual ~plIpcChannel();

  /// \brief Creates an IPC communication channel using pipes.
  /// \param szAddress Name of the pipe, must be unique on a system and less than 200 characters.
  /// \param mode Whether to run in client or server mode.
  static plInternal::NewInstance<plIpcChannel> CreatePipeChannel(plStringView sAddress, Mode::Enum mode);

  static plInternal::NewInstance<plIpcChannel> CreateNetworkChannel(plStringView sAddress, Mode::Enum mode);


  /// \brief Connects async. On success, m_Events will be broadcasted.
  void Connect();
  /// \brief Disconnect async. On completion, m_Events will be broadcasted.
  void Disconnect();
  /// \brief Returns whether we have a connection.
  bool IsConnected() const { return m_iConnectionState == ConnectionState::Connected; }
  /// \brief Returns the current state of the connection.
  plEnum<ConnectionState> GetConnectionState() const { return plEnum<ConnectionState>(m_iConnectionState); }

  /// \brief Sends a message. pMsg can be destroyed after the call.
  bool Send(plArrayPtr<const plUInt8> data);

  using ReceiveCallback = plDelegate<void(plArrayPtr<const plUInt8> message)>;
  void SetReceiveCallback(ReceiveCallback callback);

  /// \brief Block and wait for new messages and call ProcessMessages.
  plResult WaitForMessages(plTime timeout);

public:
  plEvent<const plIpcChannelEvent&, plMutex> m_Events; ///< Will be sent from any thread.

protected:
  plIpcChannel(plStringView sAddress, Mode::Enum mode);

  /// \brief Override this and return true, if the surrounding infrastructure should call the 'Tick()' function.
  virtual bool RequiresRegularTick() { return false; }
  /// \brief Can implement regular updates, e.g. for polling network state.
  virtual void Tick() {}

  /// \brief Called on worker thread after Connect was called.
  virtual void InternalConnect() = 0;
  /// \brief Called on worker thread after Disconnect was called.
  virtual void InternalDisconnect() = 0;
  /// \brief Called on worker thread to sent pending messages.
  virtual void InternalSend() = 0;
  /// \brief Called by Send to determine whether the message loop need to be woken up.
  virtual bool NeedWakeup() const = 0;

  void SetConnectionState(plEnum<ConnectionState> state);
  /// \brief Implementation needs to call this when new data has been received.
  ///  data can be invalidated after the function.
  void ReceiveData(plArrayPtr<const plUInt8> data);
  void FlushPendingOperations();

private:
protected:
  enum Constants : plUInt32
  {
    HEADER_SIZE = 8,                     ///< Magic value and size plUint32
    MAGIC_VALUE = 'USED',                ///< Magic value
    MAX_MESSAGE_SIZE = 1024 * 1024 * 16, ///< Arbitrary message size limit
  };

  friend class plMessageLoop;
  plThreadID m_ThreadId = 0;

  plAtomicInteger<ConnectionState::Enum> m_iConnectionState = ConnectionState::Disconnected;

  // Setup in ctor
  plString m_sAddress;
  const plEnum<Mode> m_Mode;
  plMessageLoop* m_pOwner = nullptr;

  // Mutex locked
  plMutex m_OutputQueueMutex;
  plDeque<plContiguousMemoryStreamStorage> m_OutputQueue;

  // Only accessed from worker thread
  plDynamicArray<plUInt8> m_MessageAccumulator; ///< Message is assembled in here

  // Mutex locked
  plMutex m_ReceiveCallbackMutex;
  ReceiveCallback m_ReceiveCallback;
  plThreadSignal m_IncomingMessages;
};
