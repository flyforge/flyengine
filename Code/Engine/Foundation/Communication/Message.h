#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>

using plMessageId = plUInt16;
class plStreamWriter;
class plStreamReader;

/// \brief Base class for all message types. Each message type has it's own id which is used to dispatch messages efficiently.
///
/// To implement a custom message type derive from plMessage and add PL_DECLARE_MESSAGE_TYPE to the type declaration.
/// PL_IMPLEMENT_MESSAGE_TYPE needs to be added to a cpp.
/// \see plRTTI
///
/// For the automatic cloning to work and for efficiency the messages must only contain simple data members.
/// For instance, everything that allocates internally (strings, arrays) should be avoided.
/// Instead, such objects should be located somewhere else and the message should only contain pointers to the data.
///
class PL_FOUNDATION_DLL plMessage : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plMessage, plReflectedClass);

protected:
  explicit plMessage(size_t messageSize)
  {
    const auto sizeOffset = (reinterpret_cast<uintptr_t>(&m_Id) - reinterpret_cast<uintptr_t>(this)) + sizeof(m_Id);
    memset((void*)plMemoryUtils::AddByteOffset(this, sizeOffset), 0, messageSize - sizeOffset);
    m_uiSize = static_cast<plUInt16>(messageSize);
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    m_uiDebugMessageRouting = 0;
#endif
  }

public:
  PL_ALWAYS_INLINE plMessage()
    : plMessage(sizeof(plMessage))
  {
  }

  virtual ~plMessage() = default;

  /// \brief Derived message types can override this method to influence sorting order. Smaller keys are processed first.
  virtual plInt32 GetSortingKey() const { return 0; }

  /// \brief Returns the id for this message type.
  PL_ALWAYS_INLINE plMessageId GetId() const { return m_Id; }

  /// \brief Returns the size in byte of this message.
  PL_ALWAYS_INLINE plUInt16 GetSize() const { return m_uiSize; }

  /// \brief Calculates a hash of the message.
  PL_ALWAYS_INLINE plUInt64 GetHash() const { return plHashingUtils::xxHash64(this, m_uiSize); }

  /// \brief Implement this for efficient transmission across process boundaries (e.g. network transfer etc.)
  ///
  /// If the message is only ever sent within the same process between nodes of the same plWorld,
  /// this does not need to be implemented.
  ///
  /// Note that PackageForTransfer() will automatically include the plRTTI type version into the stream
  /// and ReplicatePackedMessage() will pass this into Deserialize(). Use this if the serialization changes.
  virtual void Serialize(plStreamWriter& inout_stream) const { PL_ASSERT_NOT_IMPLEMENTED; }

  /// \see Serialize()
  virtual void Deserialize(plStreamReader& inout_stream, plUInt8 uiTypeVersion) { PL_ASSERT_NOT_IMPLEMENTED; }

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  /// set to true while debugging a message routing problem
  /// if the message is not delivered to any recipient at all, information about why that is will be written to plLog
  PL_ALWAYS_INLINE void SetDebugMessageRouting(bool bDebug) { m_uiDebugMessageRouting = bDebug; }

  PL_ALWAYS_INLINE bool GetDebugMessageRouting() const { return m_uiDebugMessageRouting; }
#endif

protected:
  PL_ALWAYS_INLINE static plMessageId GetNextMsgId() { return s_NextMsgId++; }

  plMessageId m_Id;

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  plUInt16 m_uiSize : 15;
  plUInt16 m_uiDebugMessageRouting : 1;
#else
  plUInt16 m_uiSize;
#endif

  static plMessageId s_NextMsgId;


  //////////////////////////////////////////////////////////////////////////
  // Transferring and replicating messages
  //

public:
  /// \brief Writes msg to stream in such a way that ReplicatePackedMessage() can restore it even in another process
  ///
  /// For this to work the message type has to have the Serialize and Deserialize functions implemented.
  ///
  /// \note This is NOT used by plWorld. Within the same process messages can be dispatched more efficiently.
  static void PackageForTransfer(const plMessage& msg, plStreamWriter& inout_stream);

  /// \brief Restores a message that was written by PackageForTransfer()
  ///
  /// If the message type is unknown, nullptr is returned.
  /// \see PackageForTransfer()
  static plUniquePtr<plMessage> ReplicatePackedMessage(plStreamReader& inout_stream);

private:
};

/// \brief Add this macro to the declaration of your custom message type.
#define PL_DECLARE_MESSAGE_TYPE(messageType, baseType)      \
private:                                                    \
  PL_ADD_DYNAMIC_REFLECTION(messageType, baseType);         \
  static plMessageId MSG_ID;                                \
                                                            \
protected:                                                  \
  PL_ALWAYS_INLINE explicit messageType(size_t messageSize) \
    : baseType(messageSize)                                 \
  {                                                         \
    m_Id = messageType::MSG_ID;                             \
  }                                                         \
                                                            \
public:                                                     \
  static plMessageId GetTypeMsgId()                         \
  {                                                         \
    static plMessageId id = plMessage::GetNextMsgId();      \
    return id;                                              \
  }                                                         \
                                                            \
  PL_ALWAYS_INLINE messageType()                            \
    : messageType(sizeof(messageType))                      \
  {                                                         \
  }

/// \brief Implements the given message type. Add this macro to a cpp outside of the type declaration.
#define PL_IMPLEMENT_MESSAGE_TYPE(messageType) plMessageId messageType::MSG_ID = messageType::GetTypeMsgId();


/// \brief Base class for all message senders.
template <typename T>
struct plMessageSenderBase
{
  using MessageType = T;
};
