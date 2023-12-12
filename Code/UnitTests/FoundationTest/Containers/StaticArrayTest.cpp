#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/StaticArray.h>

namespace StaticArrayTestDetail
{
  class Dummy
  {
  public:
    int a;
    std::string s;

    Dummy()
      : a(0)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , s("Test")
    {
    }
    Dummy(const Dummy& other)

      = default;
    ~Dummy() = default;

    Dummy& operator=(const Dummy& other) = default;

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };
} // namespace StaticArrayTestDetail

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
static_assert(sizeof(plStaticArray<plInt32, 1>) == 24);
#else
static_assert(sizeof(plStaticArray<plInt32, 1>) == 16);
#endif

static_assert(plGetTypeClass<plStaticArray<plInt32, 1>>::value == plTypeIsMemRelocatable::value);
static_assert(plGetTypeClass<plStaticArray<StaticArrayTestDetail::Dummy, 1>>::value == plTypeIsClass::value);

PLASMA_CREATE_SIMPLE_TEST(Containers, StaticArray)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plStaticArray<plInt32, 32> a1;
    plStaticArray<plConstructionCounter, 32> a2;

    PLASMA_TEST_BOOL(a1.GetCount() == 0);
    PLASMA_TEST_BOOL(a2.GetCount() == 0);
    PLASMA_TEST_BOOL(a1.IsEmpty());
    PLASMA_TEST_BOOL(a2.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plStaticArray<plInt32, 32> a1;

    for (plInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    plStaticArray<plInt32, 64> a2 = a1;
    plStaticArray<plInt32, 32> a3(a1);

    PLASMA_TEST_BOOL(a1 == a2);
    PLASMA_TEST_BOOL(a1 == a3);
    PLASMA_TEST_BOOL(a2 == a3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Convert to ArrayPtr")
  {
    plStaticArray<plInt32, 128> a1;

    for (plInt32 i = 0; i < 100; ++i)
    {
      plInt32 r = rand() % 100000;
      a1.PushBack(r);
    }

    plArrayPtr<plInt32> ap = a1;

    PLASMA_TEST_BOOL(ap.GetCount() == a1.GetCount());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator =")
  {
    plStaticArray<plInt32, 128> a1, a2;

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    PLASMA_TEST_BOOL(a1 == a2);

    plArrayPtr<plInt32> arrayPtr(a1);

    a2 = arrayPtr;

    PLASMA_TEST_BOOL(a2 == arrayPtr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=")
  {
    plStaticArray<plInt32, 128> a1, a2;

    PLASMA_TEST_BOOL(a1 == a1);
    PLASMA_TEST_BOOL(a2 == a2);
    PLASMA_TEST_BOOL(a1 == a2);

    PLASMA_TEST_BOOL((a1 != a1) == false);
    PLASMA_TEST_BOOL((a2 != a2) == false);
    PLASMA_TEST_BOOL((a1 != a2) == false);

    for (plInt32 i = 0; i < 100; ++i)
    {
      plInt32 r = rand() % 100000;
      a1.PushBack(r);
      a2.PushBack(r);
    }

    PLASMA_TEST_BOOL(a1 == a1);
    PLASMA_TEST_BOOL(a2 == a2);
    PLASMA_TEST_BOOL(a1 == a2);

    PLASMA_TEST_BOOL((a1 != a2) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Index operator")
  {
    plStaticArray<plInt32, 128> a1;
    a1.SetCountUninitialized(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    const plStaticArray<plInt32, 128> ca1 = a1;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(ca1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    plStaticArray<plInt32, 128> a1;

    PLASMA_TEST_BOOL(a1.IsEmpty());

    for (plInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      PLASMA_TEST_INT(a1[i], 0);
      a1[i] = i;

      PLASMA_TEST_INT((int)a1.GetCount(), i + 1);
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

    a1.SetCountUninitialized(32);
    PLASMA_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    PLASMA_TEST_INT(a1[31], 45);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plStaticArray<plInt32, 128> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plStaticArray<plInt32, 128> a1;

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
    plStaticArray<plInt32, 128> a1;

    // always inserts at the front
    for (plInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], 99 - i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndCopy")
  {
    plStaticArray<plInt32, 128> a1;

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
    plStaticArray<plInt32, 128> a1;

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
    plStaticArray<plInt32, 128> a1;

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
    plStaticArray<plInt32, 128> a1;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    plStaticArray<plInt32, 128> a1;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construction / Destruction")
  {
    {
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

      plStaticArray<plConstructionCounter, 128> a1;
      plStaticArray<plConstructionCounter, 100> a2;

      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

      a1.PushBack(plConstructionCounter(1));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(plConstructionCounter(2), 0);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 0)); // two copies

      a1.Clear();
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 2));

      a1.PushBack(plConstructionCounter(3));
      a1.PushBack(plConstructionCounter(4));
      a1.PushBack(plConstructionCounter(5));
      a1.PushBack(plConstructionCounter(6));

      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(plConstructionCounter(3));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(plConstructionCounter(3));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingPrimitives")
  {
    plStaticArray<plUInt32, 128> list;

    list.Sort();

    for (plUInt32 i = 0; i < 45; i++)
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingObjects")
  {
    plStaticArray<StaticArrayTestDetail::Dummy, 128> list;

    for (plUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    StaticArrayTestDetail::Dummy last = 0;
    for (plUInt32 i = 0; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Various")
  {
    plStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    list.PushBack(1);
    list.PushBack(2);
    list.PushBack(3);
    list.Insert(4, 3);
    list.Insert(0, 1);
    list.Insert(0, 5);

    PLASMA_TEST_BOOL(list[0].a == 1);
    PLASMA_TEST_BOOL(list[1].a == 0);
    PLASMA_TEST_BOOL(list[2].a == 2);
    PLASMA_TEST_BOOL(list[3].a == 3);
    PLASMA_TEST_BOOL(list[4].a == 4);
    PLASMA_TEST_BOOL(list[5].a == 0);
    PLASMA_TEST_BOOL(list.GetCount() == 6);

    list.RemoveAtAndCopy(3);
    list.RemoveAtAndSwap(2);

    PLASMA_TEST_BOOL(list[0].a == 1);
    PLASMA_TEST_BOOL(list[1].a == 0);
    PLASMA_TEST_BOOL(list[2].a == 0);
    PLASMA_TEST_BOOL(list[3].a == 4);
    PLASMA_TEST_BOOL(list.GetCount() == 4);
    PLASMA_TEST_BOOL(list.IndexOf(0) == 1);
    PLASMA_TEST_BOOL(list.LastIndexOf(0) == 2);

    list.PushBack(5);
    PLASMA_TEST_BOOL(list[4].a == 5);
    StaticArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    PLASMA_TEST_BOOL(d.a == 5);
    PLASMA_TEST_BOOL(list.GetCount() == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment")
  {
    plStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    plStaticArray<StaticArrayTestDetail::Dummy, 32> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    PLASMA_TEST_BOOL(list.GetCount() == list2.GetCount());

    list2.Clear();
    PLASMA_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Count")
  {
    plStaticArray<StaticArrayTestDetail::Dummy, 32> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(StaticArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
  {
    plStaticArray<plInt32, 1024> a1;

    for (plInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (plInt32 i = 1; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    plUInt32 prev = 0;
    for (plUInt32 val : a1)
    {
      PLASMA_TEST_BOOL(prev <= val);
      prev = val;
    }

    // const array
    const plStaticArray<plInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[400]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
  {
    plStaticArray<plInt32, 1024> a1;

    for (plInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(rbegin(a1), rend(a1));

    for (plInt32 i = 1; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(a1[i - 1] >= a1[i]);
    }

    // foreach
    plUInt32 prev = 1000;
    for (plUInt32 val : a1)
    {
      PLASMA_TEST_BOOL(prev >= val);
      prev = val;
    }

    // const array
    const plStaticArray<plInt32, 1024>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }
}
