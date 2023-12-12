#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Communication/MessageQueue.h>

namespace
{
  struct plMsgTest : public plMessage
  {
    PLASMA_DECLARE_MESSAGE_TYPE(plMsgTest, plMessage);
  };

  PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgTest);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgTest, 1, plRTTIDefaultAllocator<plMsgTest>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;

  struct TestMessage : public plMsgTest
  {
    PLASMA_DECLARE_MESSAGE_TYPE(TestMessage, plMsgTest);

    int x;
    int y;
  };

  struct MetaData
  {
    int receiver;
  };

  using TestMessageQueue = plMessageQueue<MetaData>;

  PLASMA_IMPLEMENT_MESSAGE_TYPE(TestMessage);
  PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(TestMessage, 1, plRTTIDefaultAllocator<TestMessage>)
  PLASMA_END_DYNAMIC_REFLECTED_TYPE;
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Communication, MessageQueue)
{
  {
    TestMessage msg;
    PLASMA_TEST_INT(msg.GetSize(), sizeof(TestMessage));
  }

  TestMessageQueue q;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enqueue")
  {
    for (plUInt32 i = 0; i < 100; ++i)
    {
      TestMessage* pMsg = PLASMA_DEFAULT_NEW(TestMessage);
      pMsg->x = rand();
      pMsg->y = rand();

      MetaData md;
      md.receiver = rand() % 10;

      q.Enqueue(pMsg, md);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Sorting")
  {
    struct MessageComparer
    {
      bool Less(const TestMessageQueue::Entry& a, const TestMessageQueue::Entry& b) const
      {
        if (a.m_MetaData.receiver != b.m_MetaData.receiver)
          return a.m_MetaData.receiver < b.m_MetaData.receiver;

        return a.m_pMessage->GetHash() < b.m_pMessage->GetHash();
      }
    };

    q.Sort(MessageComparer());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator[]")
  {
    PLASMA_LOCK(q);

    plMessage* pLastMsg = q[0].m_pMessage;
    MetaData lastMd = q[0].m_MetaData;

    for (plUInt32 i = 1; i < q.GetCount(); ++i)
    {
      plMessage* pMsg = q[i].m_pMessage;
      MetaData md = q[i].m_MetaData;

      if (md.receiver == lastMd.receiver)
      {
        PLASMA_TEST_BOOL(pMsg->GetHash() >= pLastMsg->GetHash());
      }
      else
      {
        PLASMA_TEST_BOOL(md.receiver >= lastMd.receiver);
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Dequeue")
  {
    plMessage* pMsg = nullptr;
    MetaData md;

    while (q.TryDequeue(pMsg, md))
    {
      PLASMA_DEFAULT_DELETE(pMsg);
    }
  }
}
