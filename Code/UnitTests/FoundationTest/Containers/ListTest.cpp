#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/List.h>

PLASMA_CREATE_SIMPLE_TEST(Containers, List)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor") { plList<plInt32> l; }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack() / PeekBack")
  {
    plList<plInt32> l;
    l.PushBack();

    PLASMA_TEST_INT(l.GetCount(), 1);
    PLASMA_TEST_INT(l.PeekBack(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack(i) / GetCount")
  {
    plList<plInt32> l;
    PLASMA_TEST_BOOL(l.GetHeapMemoryUsage() == 0);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);

      PLASMA_TEST_INT(l.GetCount(), i + 1);
      PLASMA_TEST_INT(l.PeekBack(), i);
    }

    PLASMA_TEST_BOOL(l.GetHeapMemoryUsage() >= sizeof(plInt32) * 1000);

    plUInt32 i = 0;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, i);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PopBack()")
  {
    plList<plInt32> l;

    plInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    while (!l.IsEmpty())
    {
      --i;
      PLASMA_TEST_INT(l.PeekBack(), i);
      l.PopBack();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushFront() / PeekFront")
  {
    plList<plInt32> l;
    l.PushFront();

    PLASMA_TEST_INT(l.GetCount(), 1);
    PLASMA_TEST_INT(l.PeekFront(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushFront(i) / PeekFront")
  {
    plList<plInt32> l;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      l.PushFront(i);

      PLASMA_TEST_INT(l.GetCount(), i + 1);
      PLASMA_TEST_INT(l.PeekFront(), i);
    }

    plUInt32 i2 = 1000;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      --i2;
      PLASMA_TEST_INT(*it, i2);
    }

    PLASMA_TEST_INT(i2, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PopFront()")
  {
    plList<plInt32> l;

    plInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushFront(i);

    while (!l.IsEmpty())
    {
      --i;
      PLASMA_TEST_INT(l.PeekFront(), i);
      l.PopFront();
    }
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear / IsEmpty")
  {
    plList<plInt32> l;

    PLASMA_TEST_BOOL(l.IsEmpty());

    for (plUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    PLASMA_TEST_BOOL(!l.IsEmpty());

    l.Clear();
    PLASMA_TEST_BOOL(l.IsEmpty());

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      l.PushBack(i);
      PLASMA_TEST_BOOL(!l.IsEmpty());

      l.Clear();
      PLASMA_TEST_BOOL(l.IsEmpty());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=")
  {
    plList<plInt32> l, l2;

    for (plUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    l2 = l;

    plUInt32 i = 0;
    for (plList<plInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, i);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plList<plInt32> l;

    for (plUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    plList<plInt32> l2(l);

    plUInt32 i = 0;
    for (plList<plInt32>::Iterator it = l2.GetIterator(); it != l2.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, i);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount")
  {
    plList<plInt32> l;
    l.SetCount(1000);
    PLASMA_TEST_INT(l.GetCount(), 1000);

    plInt32 i = 1;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, 0);
      *it = i;
      ++i;
    }

    l.SetCount(2000);
    i = 1;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      if (i > 1000)
        PLASMA_TEST_INT(*it, 0);
      else
        PLASMA_TEST_INT(*it, i);

      ++i;
    }

    l.SetCount(500);
    i = 1;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, i);
      ++i;
    }

    PLASMA_TEST_INT(i, 501);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert(item)")
  {
    plList<plInt32> l;

    for (plUInt32 i = 1; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    plInt32 i = 1;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      PLASMA_TEST_INT(*it, i + 10000);
      ++it;

      PLASMA_TEST_BOOL(it.IsValid());
      PLASMA_TEST_INT(*it, i);

      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove(item)")
  {
    plList<plInt32> l;

    plUInt32 i = 1;
    for (; i < 1000; ++i)
      l.PushBack(i);

    // create an interleaved array of values of i and i+10000
    for (plList<plInt32>::Iterator it = l.GetIterator(); it != l.GetEndIterator(); ++it)
    {
      // insert before this element
      l.Insert(it, *it + 10000);
    }

    // now remove every second element and only keep the larger values
    for (plList<plInt32>::Iterator it = l.GetLastIterator(); it.IsValid(); --it)
    {
      it = l.Remove(it);
      --it;
      --i;
      PLASMA_TEST_INT(*it, i + 10000);
    }

    i = 1;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(*it, i + 10000);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Iterator::IsValid")
  {
    plList<plInt32> l;

    for (plUInt32 i = 0; i < 1000; ++i)
      l.PushBack(i);

    plUInt32 i = 0;
    for (plList<plInt32>::Iterator it = l.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(*it, i);
      ++i;
    }

    PLASMA_TEST_BOOL(!l.GetEndIterator().IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Element Constructions / Destructions")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    plList<plConstructionCounter> l;

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    l.PushBack();
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1));

    l.PushBack(plConstructionCounter(1));
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1));

    l.SetCount(4);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(4, 2));

    l.Clear();
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 4));

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=")
  {
    plList<plInt32> l, l2;

    PLASMA_TEST_BOOL(l == l2);

    plInt32 i = 0;
    for (; i < 1000; ++i)
      l.PushBack(i);

    PLASMA_TEST_BOOL(l != l2);

    l2 = l;

    PLASMA_TEST_BOOL(l == l2);
  }
}
