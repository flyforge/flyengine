#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Message.h>
#include <Foundation/Reflection/Reflection.h>

#ifdef GetMessage
#  undef GetMessage
#endif

namespace
{

  struct plMsgTest : public plMessage
  {
    PLASMA_DECLARE_MESSAGE_TYPE(plMsgTest, plMessage);
  };

  PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTest);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTest, 1, plRTTIDefaultAllocator<plMsgTest>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  struct AddMessage : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(AddMessage, plMsgTest);

    plInt32 m_iValue;
  };
  PLASMA_IMPLEMENT_MESSAGE_TYPE(AddMessage);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(AddMessage, 1, plRTTIDefaultAllocator<AddMessage>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  struct SubMessage : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(SubMessage, plMsgTest);

    plInt32 m_iValue;
  };
  PLASMA_IMPLEMENT_MESSAGE_TYPE(SubMessage);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(SubMessage, 1, plRTTIDefaultAllocator<SubMessage>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  struct MulMessage : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(MulMessage, plMsgTest);

    plInt32 m_iValue;
  };
  PLASMA_IMPLEMENT_MESSAGE_TYPE(MulMessage);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(MulMessage, 1, plRTTIDefaultAllocator<MulMessage>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  struct GetMessage : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(GetMessage, plMsgTest);

    plInt32 m_iValue;
  };
  PLASMA_IMPLEMENT_MESSAGE_TYPE(GetMessage);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(GetMessage, 1, plRTTIDefaultAllocator<GetMessage>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

class BaseHandler : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(BaseHandler, plReflectedClass);

public:
  BaseHandler()

    = default;

  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue; }

  void OnMulMessage(MulMessage& ref_msg) { m_iValue *= ref_msg.m_iValue; }

  void OnGetMessage(GetMessage& ref_msg) const { ref_msg.m_iValue = m_iValue; }

  plInt32 m_iValue = 0;
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(BaseHandler, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_MESSAGEHANDLERS{
      PLASMA_MESSAGE_HANDLER(AddMessage, OnAddMessage),
      PLASMA_MESSAGE_HANDLER(MulMessage, OnMulMessage),
      PLASMA_MESSAGE_HANDLER(GetMessage, OnGetMessage),
  } PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

class DerivedHandler : public BaseHandler
{
  PLASMA_ADD_DYNAMIC_REFLECTION(DerivedHandler, BaseHandler);

public:
  void OnAddMessage(AddMessage& ref_msg) { m_iValue += ref_msg.m_iValue * 2; }

  void OnSubMessage(SubMessage& ref_msg) { m_iValue -= ref_msg.m_iValue; }
};

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(DerivedHandler, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(AddMessage, OnAddMessage),
    PLASMA_MESSAGE_HANDLER(SubMessage, OnSubMessage),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PLASMA_CREATE_SIMPLE_TEST(Reflection, MessageHandler)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple Dispatch")
  {
    BaseHandler test;
    const plRTTI* pRTTI = test.GetStaticRTTI();

    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    PLASMA_TEST_BOOL(!pRTTI->CanHandleMessage<SubMessage>());
    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());
    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<GetMessage>());

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    PLASMA_TEST_BOOL(handled);

    PLASMA_TEST_INT(test.m_iValue, 4);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg); // should do nothing
    PLASMA_TEST_BOOL(!handled);

    PLASMA_TEST_INT(test.m_iValue, 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple Dispatch const")
  {
    const BaseHandler test;
    const plRTTI* pRTTI = test.GetStaticRTTI();

    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    PLASMA_TEST_BOOL(!handled); // should do nothing since object is const and the add message handler is non-const

    PLASMA_TEST_INT(test.m_iValue, 0);

    GetMessage getMsg;
    getMsg.m_iValue = 12;
    handled = pRTTI->DispatchMessage(&test, getMsg);
    PLASMA_TEST_BOOL(handled);
    PLASMA_TEST_INT(getMsg.m_iValue, 0);

    PLASMA_TEST_INT(test.m_iValue, 0); // object must not be modified
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dispatch with inheritance")
  {
    DerivedHandler test;
    const plRTTI* pRTTI = test.GetStaticRTTI();

    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<AddMessage>());
    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<SubMessage>());
    PLASMA_TEST_BOOL(pRTTI->CanHandleMessage<MulMessage>());

    // message handler overridden by derived class
    AddMessage addMsg;
    addMsg.m_iValue = 4;
    bool handled = pRTTI->DispatchMessage(&test, addMsg);
    PLASMA_TEST_BOOL(handled);

    PLASMA_TEST_INT(test.m_iValue, 8);

    SubMessage subMsg;
    subMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, subMsg);
    PLASMA_TEST_BOOL(handled);

    PLASMA_TEST_INT(test.m_iValue, 4);

    // message handled by base class
    MulMessage mulMsg;
    mulMsg.m_iValue = 4;
    handled = pRTTI->DispatchMessage(&test, mulMsg);
    PLASMA_TEST_BOOL(handled);

    PLASMA_TEST_INT(test.m_iValue, 16);
  }
}
