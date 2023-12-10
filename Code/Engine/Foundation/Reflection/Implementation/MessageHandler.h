#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Reflection/Implementation/RTTI.h>

class plMessage;

/// \brief The base class for all message handlers that a type provides.
class PLASMA_FOUNDATION_DLL plAbstractMessageHandler
{
public:
  virtual ~plAbstractMessageHandler() = default;

  PLASMA_ALWAYS_INLINE void operator()(void* pInstance, plMessage& ref_msg) { (*m_DispatchFunc)(this, pInstance, ref_msg); }

  PLASMA_FORCE_INLINE void operator()(const void* pInstance, plMessage& ref_msg)
  {
    PLASMA_ASSERT_DEV(m_bIsConst, "Calling a non const message handler with a const instance.");
    (*m_ConstDispatchFunc)(this, pInstance, ref_msg);
  }

  PLASMA_ALWAYS_INLINE plMessageId GetMessageId() const { return m_Id; }

  PLASMA_ALWAYS_INLINE bool IsConst() const { return m_bIsConst; }

protected:
  using DispatchFunc = void (*)(plAbstractMessageHandler* pSelf, void* pInstance, plMessage&);
  using ConstDispatchFunc = void (*)(plAbstractMessageHandler* pSelf, const void* pInstance, plMessage&);

  union
  {
    DispatchFunc m_DispatchFunc = nullptr;
    ConstDispatchFunc m_ConstDispatchFunc;
  };
  plMessageId m_Id = plSmallInvalidIndex;
  bool m_bIsConst = false;
};

struct plMessageSenderInfo
{
  const char* m_szName;
  const plRTTI* m_pMessageType;
};

namespace plInternal
{
  template <typename Class, typename MessageType>
  struct MessageHandlerTraits
  {
    static plCompileTimeTrueType IsConst(void (Class::*)(MessageType&) const);
    static plCompileTimeFalseType IsConst(...);
  };

  template <bool bIsConst>
  struct MessageHandler
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&)>
    class Impl : public plAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_DispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = false;
      }

      static void Dispatch(plAbstractMessageHandler* pSelf, void* pInstance, plMessage& ref_msg)
      {
        Class* pTargetInstance = static_cast<Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };

  template <>
  struct MessageHandler<true>
  {
    template <typename Class, typename MessageType, void (Class::*Method)(MessageType&) const>
    class Impl : public plAbstractMessageHandler
    {
    public:
      Impl()
      {
        m_ConstDispatchFunc = &Dispatch;
        m_Id = MessageType::GetTypeMsgId();
        m_bIsConst = true;
      }

      /// \brief Casts the given message to the type of this message handler, then passes that to the class instance.
      static void Dispatch(plAbstractMessageHandler* pSelf, const void* pInstance, plMessage& ref_msg)
      {
        const Class* pTargetInstance = static_cast<const Class*>(pInstance);
        (pTargetInstance->*Method)(static_cast<MessageType&>(ref_msg));
      }
    };
  };
} // namespace plInternal

#define PLASMA_IS_CONST_MESSAGE_HANDLER(Class, MessageType, Method) \
  (sizeof(plInternal::MessageHandlerTraits<Class, MessageType>::IsConst(Method)) == sizeof(plCompileTimeTrueType))
