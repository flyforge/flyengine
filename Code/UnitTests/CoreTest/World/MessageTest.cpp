#include <CoreTest/CoreTestPCH.h>

#include <Core/World/World.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Time/Clock.h>

namespace
{
  struct plMsgTest : public plMessage
  {
    PLASMA_DECLARE_MESSAGE_TYPE(plMsgTest, plMessage);
  };

  // clang-format off
  PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTest);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTest, 1, plRTTIDefaultAllocator<plMsgTest>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  struct TestMessage1 : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(TestMessage1, plMsgTest);

    int m_iValue;
  };

  struct TestMessage2 : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(TestMessage2, plMsgTest);

    virtual plInt32 GetSortingKey() const override { return 2; }

    int m_iValue;
  };

  // clang-format off
  PLASMA_IMPLEMENT_MESSAGE_TYPE(TestMessage1);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage1, 1, plRTTIDefaultAllocator<TestMessage1>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  PLASMA_IMPLEMENT_MESSAGE_TYPE(TestMessage2);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage2, 1, plRTTIDefaultAllocator<TestMessage2>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestComponentMsg;
  using TestComponentMsgManager = plComponentManager<TestComponentMsg, plBlockStorageType::FreeList>;

  class TestComponentMsg : public plComponent
  {
    PLASMA_DECLARE_COMPONENT_TYPE(TestComponentMsg, plComponent, TestComponentMsgManager);

  public:
    TestComponentMsg()

      = default;
    ~TestComponentMsg() = default;

    virtual void SerializeComponent(plWorldWriter& inout_stream) const override {}
    virtual void DeserializeComponent(plWorldReader& inout_stream) override {}

    void OnTestMessage(TestMessage1& ref_msg) { m_iSomeData += ref_msg.m_iValue; }

    void OnTestMessage2(TestMessage2& ref_msg) { m_iSomeData2 += 2 * ref_msg.m_iValue; }

    plInt32 m_iSomeData = 1;
    plInt32 m_iSomeData2 = 2;
  };

  // clang-format off
  PLASMA_BEGIN_COMPONENT_TYPE(TestComponentMsg, 1, plComponentMode::Static)
  {
    PLASMA_BEGIN_MESSAGEHANDLERS
    {
      PLASMA_MESSAGE_HANDLER(TestMessage1, OnTestMessage),
      PLASMA_MESSAGE_HANDLER(TestMessage2, OnTestMessage2),
    }
    PLASMA_END_MESSAGEHANDLERS;
  }
  PLASMA_END_COMPONENT_TYPE;
  // clang-format on

  void ResetComponents(plGameObject& ref_object)
  {
    TestComponentMsg* pComponent = nullptr;
    if (ref_object.TryGetComponentOfBaseType(pComponent))
    {
      pComponent->m_iSomeData = 1;
      pComponent->m_iSomeData2 = 2;
    }

    for (auto it = ref_object.GetChildren(); it.IsValid(); ++it)
    {
      ResetComponents(*it);
    }
  }
} // namespace

PLASMA_CREATE_SIMPLE_TEST(World, Messaging)
{
  plWorldDesc worldDesc("Test");
  plWorld world(worldDesc);
  PLASMA_LOCK(world.GetWriteMarker());

  TestComponentMsgManager* pManager = world.GetOrCreateComponentManager<TestComponentMsgManager>();

  plGameObjectDesc desc;
  desc.m_sName.Assign("Root");
  plGameObject* pRoot = nullptr;
  world.CreateObject(desc, pRoot);
  TestComponentMsg* pComponent = nullptr;
  pManager->CreateComponent(pRoot, pComponent);

  plGameObject* pParents[2];
  desc.m_hParent = pRoot->GetHandle();
  desc.m_sName.Assign("Parent1");
  world.CreateObject(desc, pParents[0]);
  pManager->CreateComponent(pParents[0], pComponent);

  desc.m_sName.Assign("Parent2");
  world.CreateObject(desc, pParents[1]);
  pManager->CreateComponent(pParents[1], pComponent);

  for (plUInt32 i = 0; i < 2; ++i)
  {
    desc.m_hParent = pParents[i]->GetHandle();
    for (plUInt32 j = 0; j < 4; ++j)
    {
      plStringBuilder sb;
      sb.AppendFormat("Parent{0}_Child{1}", i + 1, j + 1);
      desc.m_sName.Assign(sb.GetData());

      plGameObject* pObject = nullptr;
      world.CreateObject(desc, pObject);
      pManager->CreateComponent(pObject, pComponent);
    }
  }

  // one update step so components are initialized
  world.Update();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Direct Routing")
  {
    ResetComponents(*pRoot);

    TestMessage1 msg;
    msg.m_iValue = 4;
    pParents[0]->SendMessage(msg);

    TestMessage2 msg2;
    msg2.m_iValue = 4;
    pParents[0]->SendMessage(msg2);

    TestComponentMsg* pComponent2 = nullptr;
    pParents[0]->TryGetComponentOfBaseType(pComponent2);
    PLASMA_TEST_INT(pComponent2->m_iSomeData, 5);
    PLASMA_TEST_INT(pComponent2->m_iSomeData2, 10);

    // siblings, parent and children should not be affected
    pParents[1]->TryGetComponentOfBaseType(pComponent2);
    PLASMA_TEST_INT(pComponent2->m_iSomeData, 1);
    PLASMA_TEST_INT(pComponent2->m_iSomeData2, 2);

    pRoot->TryGetComponentOfBaseType(pComponent2);
    PLASMA_TEST_INT(pComponent2->m_iSomeData, 1);
    PLASMA_TEST_INT(pComponent2->m_iSomeData2, 2);

    for (auto it = pParents[0]->GetChildren(); it.IsValid(); ++it)
    {
      it->TryGetComponentOfBaseType(pComponent2);
      PLASMA_TEST_INT(pComponent2->m_iSomeData, 1);
      PLASMA_TEST_INT(pComponent2->m_iSomeData2, 2);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Queuing")
  {
    ResetComponents(*pRoot);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, plTime::Zero(), plObjectMsgQueueType::NextFrame);

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, plTime::Zero(), plObjectMsgQueueType::NextFrame);
    }

    world.Update();

    TestComponentMsg* pComponent2 = nullptr;
    pRoot->TryGetComponentOfBaseType(pComponent2);
    PLASMA_TEST_INT(pComponent2->m_iSomeData, 46);
    PLASMA_TEST_INT(pComponent2->m_iSomeData2, 92);

    plFrameAllocator::Reset();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Queuing with delay")
  {
    ResetComponents(*pRoot);

    for (plUInt32 i = 0; i < 10; ++i)
    {
      TestMessage1 msg;
      msg.m_iValue = i;
      pRoot->PostMessage(msg, plTime::Seconds(i + 1));

      TestMessage2 msg2;
      msg2.m_iValue = i;
      pRoot->PostMessage(msg2, plTime::Seconds(i + 1));
    }

    world.GetClock().SetFixedTimeStep(plTime::Seconds(1.001f));

    int iDesiredValue = 1;
    int iDesiredValue2 = 2;

    for (plUInt32 i = 0; i < 10; ++i)
    {
      iDesiredValue += i;
      iDesiredValue2 += i * 2;

      world.Update();

      TestComponentMsg* pComponent2 = nullptr;
      pRoot->TryGetComponentOfBaseType(pComponent2);
      PLASMA_TEST_INT(pComponent2->m_iSomeData, iDesiredValue);
      PLASMA_TEST_INT(pComponent2->m_iSomeData2, iDesiredValue2);
    }

    plFrameAllocator::Reset();
  }
}
