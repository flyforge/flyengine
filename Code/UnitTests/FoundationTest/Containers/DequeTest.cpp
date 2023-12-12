#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Types/UniquePtr.h>

namespace DequeTestDetail
{
  using st = plConstructionCounter;

  static plDeque<st> CreateArray(plUInt32 uiSize, plUInt32 uiOffset)
  {
    plDeque<st> a;
    a.SetCount(uiSize);

    for (plUInt32 i = 0; i < uiSize; ++i)
      a[i] = uiOffset + i;

    return a;
  }
} // namespace DequeTestDetail

PLASMA_CREATE_SIMPLE_TEST(Containers, Deque)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Fill / Empty (Sawtooth)")
  {
    plDeque<plInt32> d;

    plUInt32 uiVal = 0;

    for (plInt32 i = 0; i < 10000; ++i)
      d.PushBack(uiVal++);

    // this is kind of the worst case scenario, as it will deallocate and reallocate chunks in every loop
    // the smaller the chunk size, the more allocations will happen
    for (plUInt32 s2 = 0; s2 < 10; ++s2)
    {
      for (plInt32 i = 0; i < 1000; ++i)
        d.PopBack();
      for (plInt32 i = 0; i < 1000; ++i)
        d.PushBack(uiVal++);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Fill / Empty")
  {
    plDeque<plInt32> d;

    PLASMA_TEST_BOOL(d.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 10000; ++i)
      d.PushBack(i);

    PLASMA_TEST_BOOL(d.GetHeapMemoryUsage() > 10000 * sizeof(plInt32));

    for (plInt32 i = 0; i < 10000; ++i)
      d.PopFront();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Queue Back")
  {
    plDeque<plInt32> d;

    d.PushBack(0);

    for (plInt32 i = 0; i < 10000; ++i)
    {
      d.PushBack(i);
      d.PopFront();
    }

    d.Compact();

    for (plInt32 i = 0; i < 10000; ++i)
    {
      d.PushBack(i);
      d.PopFront();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Queue Front")
  {
    plDeque<plInt32> d;

    d.PushBack(0);

    for (plInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();
    }

    d.Compact();

    for (plInt32 i = 0; i < 10000; ++i)
    {
      d.PushFront(i);
      d.PopBack();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "POD Types")
  {
    plDeque<plInt32> d1;
    d1.SetCount(5120);

    d1.Compact();

    PLASMA_TEST_BOOL(d1.GetCount() == 5120);


    d1.SetCount(1);
    d1.Compact();

    PLASMA_TEST_BOOL(d1.GetHeapMemoryUsage() > 0);

    d1.Clear();

    d1.Compact();

    PLASMA_TEST_BOOL(d1.GetHeapMemoryUsage() == 0);
  }

  plStartup::ShutdownCoreSystems();

  plStartup::StartupCoreSystems();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "POD Types")
  {
    plDeque<plInt32> d1;
    d1.SetCount(1000);

    PLASMA_TEST_BOOL(d1.GetCount() == 1000);
    d1.Clear();
    PLASMA_TEST_BOOL(d1.IsEmpty());

    for (plInt32 i = 1; i < 1000; ++i)
    {
      d1.PushBack(i);

      PLASMA_TEST_BOOL(d1.PeekBack() == i);
      PLASMA_TEST_BOOL(d1.GetCount() == i);
      PLASMA_TEST_BOOL(!d1.IsEmpty());
    }

    d1.Clear();
    PLASMA_TEST_BOOL(d1.IsEmpty());

    for (plInt32 i = 1; i < 1000; ++i)
    {
      d1.PushFront(i);

      PLASMA_TEST_BOOL(d1.PeekFront() == i);
      PLASMA_TEST_BOOL(d1.GetCount() == i);
      PLASMA_TEST_BOOL(!d1.IsEmpty());
    }

    d1.Clear();
    PLASMA_TEST_BOOL(d1.IsEmpty());

    for (plInt32 i = 1; i <= 1000; ++i)
    {
      d1.PushFront(i);
      d1.PushBack(i);

      PLASMA_TEST_BOOL(d1.PeekFront() == i);
      PLASMA_TEST_BOOL(d1.PeekBack() == i);
      PLASMA_TEST_BOOL(d1.GetCount() == i * 2);
      PLASMA_TEST_BOOL(!d1.IsEmpty());
    }

    plDeque<plInt32> d2;
    d2 = d1;

    for (plInt32 i = 1000; i >= 1; --i)
    {
      PLASMA_TEST_BOOL(d1.PeekFront() == i);
      PLASMA_TEST_BOOL(d1.PeekBack() == i);
      PLASMA_TEST_BOOL(d1.GetCount() == i * 2);
      PLASMA_TEST_BOOL(!d1.IsEmpty());

      d1.PopFront();
      d1.PopBack();


      PLASMA_TEST_BOOL(d2.PeekFront() == i);
      PLASMA_TEST_BOOL(d2.PeekBack() == i);
      PLASMA_TEST_BOOL(d2.GetCount() == i * 2);
      PLASMA_TEST_BOOL(!d2.IsEmpty());

      d2.PopFront();
      d2.PopBack();
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
    {
      plDeque<plInt32> a1;
      for (plInt32 i = 0; i < 1000; ++i)
        a1.PushBack(1000 - i - 1);

      // STL sort
      std::sort(begin(a1), end(a1));

      plUInt32 prev = 0;
      for (plUInt32 val : a1)
      {
        PLASMA_TEST_BOOL(prev <= val);
        prev = val;
      }

      // STL lower bound
      auto lb = std::lower_bound(begin(a1), end(a1), 400);
      PLASMA_TEST_BOOL(*lb == a1[400]);
    }

    PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
    {
      plDeque<plInt32> a1;
      for (plInt32 i = 0; i < 1000; ++i)
        a1.PushBack(1000 - i - 1);

      std::sort(rbegin(a1), rend(a1));

      // foreach
      plUInt32 prev = 1000;
      for (plUInt32 val : a1)
      {
        PLASMA_TEST_BOOL(prev >= val);
        prev = val;
      }

      // const array
      const plDeque<plInt32>& a2 = a1;

      // STL lower bound
      auto lb2 = std::lower_bound(rbegin(a2), rend(a2), 400);
      PLASMA_TEST_INT(*lb2, a2[1000 - 400 - 1]);
    }
  }

  plStartup::ShutdownCoreSystems();

  plStartup::StartupCoreSystems();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Non-POD Types")
  {
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    {
      plDeque<DequeTestDetail::st> v1;
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

      {
        v1.PushBack(DequeTestDetail::st(3));
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1));

        v1.PushBack();
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(1, 0));

        v1.PopBack();
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1));
      }
      {
        v1.PushFront(DequeTestDetail::st(3));
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1));

        v1.PushFront();
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(1, 0));

        v1.PopFront();
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1));
      }

      PLASMA_TEST_BOOL(v1.GetCount() == 2);

      v1.SetCount(12);
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(10, 0));

      {
        plDeque<DequeTestDetail::st> v2;
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

        v2 = v1;
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        v2.Clear();
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));

        plDeque<DequeTestDetail::st> v3(v1);
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        plDeque<DequeTestDetail::st> v4(v1);
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(12, 0));

        v4.SetCount(0);
        PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));
      }

      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 12));
    }

    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingPrimitives")
  {
    plDeque<plUInt32> list;

    list.Sort();

    for (plUInt32 i = 0; i < 245; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    plUInt32 last = 0;
    for (plUInt32 i = 0; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }



  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plDeque<plInt32> a1;
    plDeque<DequeTestDetail::st> a2;

    PLASMA_TEST_BOOL(a1.GetCount() == 0);
    PLASMA_TEST_BOOL(a2.GetCount() == 0);
    PLASMA_TEST_BOOL(a1.IsEmpty());
    PLASMA_TEST_BOOL(a2.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    plDeque<plInt32> a2 = a1;
    plDeque<plInt32> a3(a1);

    PLASMA_TEST_BOOL(a1.GetCount() == a2.GetCount());
    PLASMA_TEST_BOOL(a1.GetCount() == a3.GetCount());

    for (plUInt32 i = 0; i < a1.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(a1[i] == a2[i]);
      PLASMA_TEST_BOOL(a1[i] == a3[i]);
      PLASMA_TEST_BOOL(a2[i] == a3[i]);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Constructor / Operator")
  {
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    {
      // move constructor
      plDeque<DequeTestDetail::st> a1(DequeTestDetail::CreateArray(100, 20));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DequeTestDetail::CreateArray(200, 50);

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);
    }

    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator = / operator == / operator !=")
  {
    plDeque<plInt32> a1;
    plDeque<plInt32> a2;

    PLASMA_TEST_BOOL(a1 == a2);

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    PLASMA_TEST_BOOL(a1 != a2);

    a2 = a1;

    PLASMA_TEST_BOOL(a1.GetCount() == a2.GetCount());

    for (plUInt32 i = 0; i < a1.GetCount(); ++i)
      PLASMA_TEST_BOOL(a1[i] == a2[i]);

    PLASMA_TEST_BOOL(a1 == a2);
    PLASMA_TEST_BOOL(a2 == a1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Index operator")
  {
    plDeque<plInt32> a1;
    a1.SetCount(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    plDeque<plInt32> ca1;
    ca1 = a1;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(ca1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    plDeque<plInt32> a1;

    PLASMA_TEST_BOOL(a1.IsEmpty());

    for (plInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      PLASMA_TEST_INT(a1[i], 0); // default init
      a1[i] = i;
      PLASMA_TEST_INT(a1[i], i);

      PLASMA_TEST_INT(a1.GetCount(), i + 1);
      PLASMA_TEST_BOOL(!a1.IsEmpty());
    }

    for (plInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(a1[i], i);

    for (plInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(i);

      PLASMA_TEST_INT(a1.GetCount(), i);

      for (plInt32 i2 = 0; i2 < i; ++i2)
        PLASMA_TEST_INT(a1[i2], i2);
    }

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCountUninitialized")
  {
    plDeque<plInt32> a1;

    PLASMA_TEST_BOOL(a1.IsEmpty());

    for (plInt32 i = 0; i < 128; ++i)
    {
      a1.SetCountUninitialized(i + 1);
      // no default init
      a1[i] = i;
      PLASMA_TEST_INT(a1[i], i);

      PLASMA_TEST_INT(a1.GetCount(), i + 1);
      PLASMA_TEST_BOOL(!a1.IsEmpty());
    }

    for (plInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(a1[i], i);

    for (plInt32 i = 128; i >= 0; --i)
    {
      a1.SetCountUninitialized(i);

      PLASMA_TEST_INT(a1.GetCount(), i);

      for (plInt32 i2 = 0; i2 < i; ++i2)
        PLASMA_TEST_INT(a1[i2], i2);
    }

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EnsureCount")
  {
    plDeque<plInt32> a1;

    PLASMA_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(0);
    PLASMA_TEST_INT(a1.GetCount(), 0);

    a1.EnsureCount(1);
    PLASMA_TEST_INT(a1.GetCount(), 1);

    a1.EnsureCount(2);
    PLASMA_TEST_INT(a1.GetCount(), 2);

    a1.EnsureCount(1);
    PLASMA_TEST_INT(a1.GetCount(), 2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plDeque<plInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = -100; i < 100; ++i)
      PLASMA_TEST_BOOL(!a1.Contains(i));

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (plInt32 i = 0; i < 100; ++i)
    {
      PLASMA_TEST_BOOL(a1.Contains(i));
      PLASMA_TEST_INT(a1.IndexOf(i), i);
      PLASMA_TEST_INT(a1.LastIndexOf(i), i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plDeque<plInt32> a1;

    // always inserts at the front
    for (plInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], 99 - i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndCopy")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i % 2);

    while (a1.RemoveAndCopy(1))
    {
    }

    PLASMA_TEST_BOOL(a1.GetCount() == 50);

    for (plUInt32 i = 0; i < a1.GetCount(); ++i)
      PLASMA_TEST_INT(a1[i], 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndSwap")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAndSwap(9);
    a1.RemoveAndSwap(7);
    a1.RemoveAndSwap(5);
    a1.RemoveAndSwap(3);
    a1.RemoveAndSwap(1);

    PLASMA_TEST_INT(a1.GetCount(), 5);

    for (plInt32 i = 0; i < 5; ++i)
      PLASMA_TEST_BOOL(plMath::IsEven(a1[i]));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAtAndCopy")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAtAndCopy(9);
    a1.RemoveAtAndCopy(7);
    a1.RemoveAtAndCopy(5);
    a1.RemoveAtAndCopy(3);
    a1.RemoveAtAndCopy(1);

    PLASMA_TEST_INT(a1.GetCount(), 5);

    for (plInt32 i = 0; i < 5; ++i)
      PLASMA_TEST_INT(a1[i], i * 2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAtAndSwap")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
      a1.Insert(i, i); // inserts at the end

    a1.RemoveAtAndSwap(9);
    a1.RemoveAtAndSwap(7);
    a1.RemoveAtAndSwap(5);
    a1.RemoveAtAndSwap(3);
    a1.RemoveAtAndSwap(1);

    PLASMA_TEST_INT(a1.GetCount(), 5);

    for (plInt32 i = 0; i < 5; ++i)
      PLASMA_TEST_BOOL(plMath::IsEven(a1[i]));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandAndGetRef")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 20; ++i)
    {
      plInt32& intRef = a1.ExpandAndGetRef();
      intRef = i * 5;
    }


    PLASMA_TEST_BOOL(a1.GetCount() == 20);

    for (plInt32 i = 0; i < 20; ++i)
    {
      PLASMA_TEST_INT(a1[i], i * 5);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
    {
      a1.PushBack(i);
      PLASMA_TEST_INT(a1.PeekBack(), i);
    }

    for (plInt32 i = 9; i >= 0; --i)
    {
      PLASMA_TEST_INT(a1.PeekBack(), i);
      a1.PopBack();
    }

    a1.PushBack(23);
    a1.PushBack(2);
    a1.PushBack(3);

    a1.PopBack(2);
    PLASMA_TEST_INT(a1.PeekBack(), 23);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushFront / PopFront / PeekFront")
  {
    plDeque<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
    {
      a1.PushFront(i);
      PLASMA_TEST_INT(a1.PeekFront(), i);
    }

    for (plInt32 i = 9; i >= 0; --i)
    {
      PLASMA_TEST_INT(a1.PeekFront(), i);
      a1.PopFront();
    }

    a1.PushFront(23);
    a1.PushFront(2);
    a1.PushFront(3);

    a1.PopFront(2);
    PLASMA_TEST_INT(a1.PeekFront(), 23);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construction / Destruction")
  {
    {
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

      plDeque<DequeTestDetail::st> a1;
      plDeque<DequeTestDetail::st> a2;

      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

      a1.PushBack(DequeTestDetail::st(1));
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(DequeTestDetail::st(2), 0);
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 2));

      a1.PushBack(DequeTestDetail::st(3));
      a1.PushBack(DequeTestDetail::st(4));
      a1.PushBack(DequeTestDetail::st(5));
      a1.PushBack(DequeTestDetail::st(6));

      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DequeTestDetail::st(3));
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DequeTestDetail::st(3));
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reserve")
  {
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    plDeque<DequeTestDetail::st> a;

    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing had to be copied over

    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compact")
  {
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    plDeque<DequeTestDetail::st> a;

    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.SetCount(10);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 0));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(190, 0));

    // this does not deallocate memory
    a.Clear();
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    a.Clear();
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(100);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    PLASMA_TEST_BOOL(DequeTestDetail::st::HasDone(100, 0));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetContiguousRange")
  {
    // deques allocate data in 4 KB chunks, so an integer deque will have 1024 ints per chunk

    plDeque<plInt32> d;

    for (plUInt32 i = 0; i < 100 * 1024; ++i)
      d.PushBack(i);

    plDynamicArray<plInt32> a;
    a.SetCountUninitialized(d.GetCount());

    plUInt32 uiArrayPos = 0;

    for (plUInt32 i = 0; i < 100; ++i)
    {
      const plUInt32 uiOffset = i * 1024 + i;

      const plUInt32 uiRange = d.GetContiguousRange(uiOffset);

      PLASMA_TEST_INT(uiRange, 1024 - i);

      plMemoryUtils::Copy(&a[uiArrayPos], &d[uiOffset], uiRange);

      uiArrayPos += uiRange;
    }

    a.SetCountUninitialized(uiArrayPos);

    uiArrayPos = 0;

    for (plUInt32 i = 0; i < 100; ++i)
    {
      const plUInt32 uiOffset = i * 1024 + i;
      const plUInt32 uiRange = 1024 - i;

      for (plUInt32 r = 0; r < uiRange; ++r)
      {
        PLASMA_TEST_INT(a[uiArrayPos], uiOffset + r);
        ++uiArrayPos;
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plDeque<plInt32> a1, a2;

    plInt32 content1[] = {1, 2, 3, 4};
    plInt32 content2[] = {5, 6, 7, 8, 9};
    for (plInt32 i : content1)
    {
      a1.PushBack(i);
    }
    for (plInt32 i : content2)
    {
      a2.PushBack(i);
    }

    plInt32* a1Ptr = &a1[0];
    plInt32* a2Ptr = &a2[0];

    a1.Swap(a2);

    // The pointers should be simply swapped
    PLASMA_TEST_BOOL(a2Ptr == &a1[0]);
    PLASMA_TEST_BOOL(a1Ptr == &a2[0]);

    PLASMA_TEST_INT(PLASMA_ARRAY_SIZE(content1), a2.GetCount());
    PLASMA_TEST_INT(PLASMA_ARRAY_SIZE(content2), a1.GetCount());

    // The data should be swapped
    for (int i = 0; i < PLASMA_ARRAY_SIZE(content1); ++i)
    {
      PLASMA_TEST_INT(content1[i], a2[i]);
    }
    for (int i = 0; i < PLASMA_ARRAY_SIZE(content2); ++i)
    {
      PLASMA_TEST_INT(content2[i], a1[i]);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move PushBack / PushFront")
  {
    plDeque<plUniquePtr<plUInt32>> a1, a2;
    a1.PushBack(plUniquePtr<plUInt32>(PLASMA_DEFAULT_NEW(plUInt32, 1)));
    a1.PushBack(plUniquePtr<plUInt32>(PLASMA_DEFAULT_NEW(plUInt32, 2)));

    a2.PushFront(plUniquePtr<plUInt32>(PLASMA_DEFAULT_NEW(plUInt32, 3)));
    a2.PushFront(plUniquePtr<plUInt32>(PLASMA_DEFAULT_NEW(plUInt32, 4)));

    a1.Swap(a2);

    PLASMA_TEST_INT(*a1[0].Borrow(), 4);
    PLASMA_TEST_INT(*a1[1].Borrow(), 3);

    PLASMA_TEST_INT(*a2[0].Borrow(), 1);
    PLASMA_TEST_INT(*a2[1].Borrow(), 2);
  }
}
