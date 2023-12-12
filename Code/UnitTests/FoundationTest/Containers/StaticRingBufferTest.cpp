#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticRingBuffer.h>

using cc = plConstructionCounter;

PLASMA_CREATE_SIMPLE_TEST(Containers, StaticRingBuffer)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plStaticRingBuffer<plInt32, 32> r1;
      plStaticRingBuffer<plInt32, 16> r2;
      plStaticRingBuffer<cc, 2> r3;
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor / Operator=")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plStaticRingBuffer<cc, 16> r1;

      for (plUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      plStaticRingBuffer<cc, 16> r2(r1);

      for (plUInt32 i = 0; i < 16; ++i)
        PLASMA_TEST_BOOL(r2[i] == cc(i));

      plStaticRingBuffer<cc, 16> r3;
      r3 = r1;

      for (plUInt32 i = 0; i < 16; ++i)
        PLASMA_TEST_BOOL(r3[i] == cc(i));
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Operator==")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plStaticRingBuffer<cc, 16> r1;

      for (plUInt32 i = 0; i < 16; ++i)
        r1.PushBack(cc(i));

      plStaticRingBuffer<cc, 16> r2(r1);
      plStaticRingBuffer<cc, 16> r3(r1);
      r3.PeekFront() = cc(3);

      PLASMA_TEST_BOOL(r1 == r1);
      PLASMA_TEST_BOOL(r2 == r2);
      PLASMA_TEST_BOOL(r3 == r3);

      PLASMA_TEST_BOOL(r1 == r2);
      PLASMA_TEST_BOOL(r1 != r3);
      PLASMA_TEST_BOOL(r2 != r3);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack / operator[] / CanAppend")
  {
    plStaticRingBuffer<plInt32, 16> r;

    for (plUInt32 i = 0; i < 16; ++i)
    {
      PLASMA_TEST_BOOL(r.CanAppend());
      r.PushBack(i);
    }

    PLASMA_TEST_BOOL(!r.CanAppend());

    for (plUInt32 i = 0; i < 16; ++i)
      PLASMA_TEST_INT(r[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCount / IsEmpty")
  {
    plStaticRingBuffer<plInt32, 16> r;

    PLASMA_TEST_BOOL(r.IsEmpty());

    for (plUInt32 i = 0; i < 16; ++i)
    {
      PLASMA_TEST_INT(r.GetCount(), i);
      r.PushBack(i);
      PLASMA_TEST_INT(r.GetCount(), i + 1);

      PLASMA_TEST_BOOL(!r.IsEmpty());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear / IsEmpty")
  {
    plStaticRingBuffer<plInt32, 16> r;

    PLASMA_TEST_BOOL(r.IsEmpty());

    for (plUInt32 i = 0; i < 16; ++i)
      r.PushBack(i);

    PLASMA_TEST_BOOL(!r.IsEmpty());

    r.Clear();

    PLASMA_TEST_BOOL(r.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Cycle Items / PeekFront")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plStaticRingBuffer<plConstructionCounter, 16> r;

      for (plUInt32 i = 0; i < 16; ++i)
      {
        r.PushBack(plConstructionCounter(i));
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (plUInt32 i = 16; i < 1000; ++i)
      {
        PLASMA_TEST_BOOL(r.PeekFront() == plConstructionCounter(i - 16));
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // one temporary

        PLASMA_TEST_BOOL(!r.CanAppend());

        r.PopFront();
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 1));

        PLASMA_TEST_BOOL(r.CanAppend());

        r.PushBack(plConstructionCounter(i));
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary
      }

      for (plUInt32 i = 1000; i < 1016; ++i)
      {
        PLASMA_TEST_BOOL(r.PeekFront() == plConstructionCounter(i - 16));
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // one temporary

        r.PopFront();
        PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 1)); // one temporary
      }

      PLASMA_TEST_BOOL(r.IsEmpty());
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }
}
