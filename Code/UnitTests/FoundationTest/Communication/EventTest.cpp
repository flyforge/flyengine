#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/Event.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Communication);

namespace
{
  struct Test
  {
    void DoStuff(plInt32* pEventData) { *pEventData += m_iData; }

    plInt32 m_iData;
  };

  struct TestRecursion
  {
    TestRecursion() { m_uiRecursionCount = 0; }
    void DoStuff(plUInt32 uiRecursions)
    {
      if (m_uiRecursionCount < uiRecursions)
      {
        m_uiRecursionCount++;
        m_Event.Broadcast(uiRecursions, 10);
      }
    }

    using Event = plEvent<plUInt32>;
    Event m_Event;
    plUInt32 m_uiRecursionCount;
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Communication, Event)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
    using TestEvent = plEvent<plInt32*>;
    TestEvent e;

    Test test1;
    test1.m_iData = 3;

    Test test2;
    test2.m_iData = 5;

    plInt32 iResult = 0;

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    PLASMA_TEST_INT(iResult, 3);

    e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    PLASMA_TEST_INT(iResult, 8);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    iResult = 0;
    e.Broadcast(&iResult);

    PLASMA_TEST_INT(iResult, 5);

    e.RemoveEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    iResult = 0;
    e.Broadcast(&iResult);

    PLASMA_TEST_INT(iResult, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Unsubscribing via ID")
  {
    using TestEvent = plEvent<plInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    auto subId1 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1));
    PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    auto subId2 = e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2));
    PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));

    e.RemoveEventHandler(subId1);
    PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

    e.RemoveEventHandler(subId2);
    PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Unsubscribing via Unsubscriber")
  {
    using TestEvent = plEvent<plInt32*>;
    TestEvent e;

    Test test1;
    Test test2;

    {
      TestEvent::Unsubscriber unsub1;

      {
        TestEvent::Unsubscriber unsub2;

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test1), unsub1);
        PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));

        e.AddEventHandler(TestEvent::Handler(&Test::DoStuff, &test2), unsub2);
        PLASMA_TEST_BOOL(e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
      }

      PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test2)));
    }

    PLASMA_TEST_BOOL(!e.HasEventHandler(TestEvent::Handler(&Test::DoStuff, &test1)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Recursion")
  {
    for (plUInt32 i = 0; i < 10; i++)
    {
      TestRecursion test;
      test.m_Event.AddEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
      test.m_Event.Broadcast(i, 10);
      PLASMA_TEST_INT(test.m_uiRecursionCount, i);
      test.m_Event.RemoveEventHandler(TestRecursion::Event::Handler(&TestRecursion::DoStuff, &test));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove while iterate")
  {
    using TestEvent = plEvent<int, plMutex, plDefaultAllocatorWrapper, plEventType::CopyOnBroadcast>;
    TestEvent e;

    plUInt32 callMap = 0;

    plEventSubscriptionID subscriptions[4] = {};

    subscriptions[0] = e.AddEventHandler(TestEvent::Handler([&](int i) { callMap |= PLASMA_BIT(0); }));

    subscriptions[1] = e.AddEventHandler(TestEvent::Handler([&](int i) {
      callMap |= PLASMA_BIT(1);
      e.RemoveEventHandler(subscriptions[1]);
    }));

    subscriptions[2] = e.AddEventHandler(TestEvent::Handler([&](int i) {
      callMap |= PLASMA_BIT(2);
      e.RemoveEventHandler(subscriptions[2]);
      e.RemoveEventHandler(subscriptions[3]);
    }));

    subscriptions[3] = e.AddEventHandler(TestEvent::Handler([&](int i) { callMap |= PLASMA_BIT(3); }));

    e.Broadcast(0);

    PLASMA_TEST_BOOL(callMap == (PLASMA_BIT(0) | PLASMA_BIT(1) | PLASMA_BIT(2) | PLASMA_BIT(3)));

    callMap = 0;
    e.Broadcast(0);
    PLASMA_TEST_BOOL(callMap == PLASMA_BIT(0));

    e.RemoveEventHandler(subscriptions[0]);
  }
}
