#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace SmallArrayTestDetail
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

  class NonMovableClass
  {
  public:
    NonMovableClass(int iVal)
    {
      m_val = iVal;
      m_pVal = &m_val;
    }

    NonMovableClass(const NonMovableClass& other)
    {
      m_val = other.m_val;
      m_pVal = &m_val;
    }

    void operator=(const NonMovableClass& other) { m_val = other.m_val; }

    int m_val = 0;
    int* m_pVal = nullptr;
  };

  template <typename T>
  static plSmallArray<T, 16> CreateArray(plUInt32 uiSize, plUInt32 uiOffset, plUInt32 uiUserData)
  {
    plSmallArray<T, 16> a;
    a.SetCount(static_cast<plUInt16>(uiSize));

    for (plUInt32 i = 0; i < uiSize; ++i)
    {
      a[i] = T(uiOffset + i);
    }

    a.template GetUserData<plUInt32>() = uiUserData;

    return a;
  }

  struct ExternalCounter
  {
    PLASMA_DECLARE_MEM_RELOCATABLE_TYPE();

    ExternalCounter() = default;

    ExternalCounter(int& ref_iCounter)
      : m_counter{&ref_iCounter}
    {
    }

    ~ExternalCounter()
    {
      if (m_counter)
        (*m_counter)++;
    }

    int* m_counter{};
  };
} // namespace SmallArrayTestDetail

static void TakesDynamicArray(plDynamicArray<int>& ref_ar, int iNum, int iStart);

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
static_assert(sizeof(plSmallArray<plInt32, 1>) == 16);
#else
static_assert(sizeof(plSmallArray<plInt32, 1>) == 12);
#endif

static_assert(plGetTypeClass<plSmallArray<plInt32, 1>>::value == plTypeIsMemRelocatable::value);
static_assert(plGetTypeClass<plSmallArray<SmallArrayTestDetail::NonMovableClass, 1>>::value == plTypeIsClass::value);

PLASMA_CREATE_SIMPLE_TEST(Containers, SmallArray)
{
  plConstructionCounter::Reset();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSmallArray<plInt32, 16> a1;
    plSmallArray<plConstructionCounter, 16> a2;

    PLASMA_TEST_BOOL(a1.GetCount() == 0);
    PLASMA_TEST_BOOL(a2.GetCount() == 0);
    PLASMA_TEST_BOOL(a1.IsEmpty());
    PLASMA_TEST_BOOL(a2.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plSmallArray<plInt32, 16> a1;

    PLASMA_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 32; ++i)
    {
      a1.PushBack(rand() % 100000);

      if (i < 16)
      {
        PLASMA_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);
      }
      else
      {
        PLASMA_TEST_BOOL(a1.GetHeapMemoryUsage() >= i * sizeof(plInt32));
      }
    }

    a1.GetUserData<plUInt32>() = 11;

    plSmallArray<plInt32, 16> a2 = a1;
    plSmallArray<plInt32, 16> a3(a1);

    PLASMA_TEST_BOOL(a1 == a2);
    PLASMA_TEST_BOOL(a1 == a3);
    PLASMA_TEST_BOOL(a2 == a3);

    PLASMA_TEST_INT(a2.GetUserData<plUInt32>(), 11);
    PLASMA_TEST_INT(a3.GetUserData<plUInt32>(), 11);

    plInt32 test[] = {1, 2, 3, 4};
    plArrayPtr<plInt32> aptr(test);

    plSmallArray<plInt32, 16> a4(aptr);

    PLASMA_TEST_BOOL(a4 == aptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Constructor / Operator")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      plSmallArray<plConstructionCounter, 16> a1(SmallArrayTestDetail::CreateArray<plConstructionCounter>(100, 20, 11));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      PLASMA_TEST_INT(a1.GetUserData<plUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<plConstructionCounter>(200, 50, 22);

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);

      PLASMA_TEST_INT(a1.GetUserData<plUInt32>(), 22);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    plConstructionCounter::Reset();

    {
      // move constructor internal storage
      plSmallArray<plConstructionCounter, 16> a2(SmallArrayTestDetail::CreateArray<plConstructionCounter>(10, 30, 11));

      PLASMA_TEST_INT(a2.GetCount(), 10);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 30 + i);

      PLASMA_TEST_INT(a2.GetUserData<plUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<plConstructionCounter>(8, 70, 22);

      PLASMA_TEST_INT(a2.GetCount(), 8);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 70 + i);

      PLASMA_TEST_INT(a2.GetUserData<plUInt32>(), 22);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    plConstructionCounter::Reset();

    plConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      plSmallArray<plConstructionCounterRelocatable, 16> a1(SmallArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(100, 20, 11));

      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(100, 0));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      PLASMA_TEST_INT(a1.GetUserData<plUInt32>(), 11);

      // move operator external storage
      a1 = SmallArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(200, 50, 22);
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(200, 100));

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);

      PLASMA_TEST_INT(a1.GetUserData<plUInt32>(), 22);
    }

    PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasAllDestructed());
    plConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      plSmallArray<plConstructionCounterRelocatable, 16> a2(SmallArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(10, 30, 11));
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(10, 0));

      PLASMA_TEST_INT(a2.GetCount(), 10);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 30 + i);

      PLASMA_TEST_INT(a2.GetUserData<plUInt32>(), 11);

      // move operator internal storage
      a2 = SmallArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(8, 70, 22);
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(8, 10));

      PLASMA_TEST_INT(a2.GetCount(), 8);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 70 + i);

      PLASMA_TEST_INT(a2.GetUserData<plUInt32>(), 22);
    }

    PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasAllDestructed());
    plConstructionCounterRelocatable::Reset();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Convert to ArrayPtr")
  {
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1, a2;

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
    plSmallArray<plInt32, 16> a1, a2;

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
    plSmallArray<plInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    const plSmallArray<plInt32, 16> ca1 = a1;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(ca1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    plSmallArray<plInt32, 16> a1;

    PLASMA_TEST_BOOL(a1.IsEmpty());

    for (plInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(static_cast<plUInt16>(i + 1));
      PLASMA_TEST_INT(a1[i], 0);
      a1[i] = i;

      PLASMA_TEST_INT(a1.GetCount(), i + 1);
      PLASMA_TEST_BOOL(!a1.IsEmpty());
    }

    for (plInt32 i = 0; i < 128; ++i)
      PLASMA_TEST_INT(a1[i], i);

    for (plInt32 i = 128; i >= 0; --i)
    {
      a1.SetCount(static_cast<plUInt16>(i));

      PLASMA_TEST_INT(a1.GetCount(), i);

      for (plInt32 i2 = 0; i2 < i; ++i2)
        PLASMA_TEST_INT(a1[i2], i2);
    }

    PLASMA_TEST_BOOL(a1.IsEmpty());

    a1.SetCountUninitialized(32);
    PLASMA_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    PLASMA_TEST_INT(a1[31], 45);

    // Test SetCount with fill value
    {
      plSmallArray<plInt32, 2> a2;
      a2.PushBack(5);
      a2.PushBack(3);
      a2.SetCount(10, 42);

      if (PLASMA_TEST_INT(a2.GetCount(), 10))
      {
        PLASMA_TEST_INT(a2[0], 5);
        PLASMA_TEST_INT(a2[1], 3);
        PLASMA_TEST_INT(a2[4], 42);
        PLASMA_TEST_INT(a2[9], 42);
      }

      a2.Clear();
      a2.PushBack(1);
      a2.PushBack(2);
      a2.PushBack(3);

      a2.SetCount(2, 10);
      if (PLASMA_TEST_INT(a2.GetCount(), 2))
      {
        PLASMA_TEST_INT(a2[0], 1);
        PLASMA_TEST_INT(a2[1], 2);
      }
    }
  }

  // Test SetCount with fill value
  {
    plSmallArray<plInt32, 2> a2;
    a2.PushBack(5);
    a2.PushBack(3);
    a2.SetCount(10, 42);

    if (PLASMA_TEST_INT(a2.GetCount(), 10))
    {
      PLASMA_TEST_INT(a2[0], 5);
      PLASMA_TEST_INT(a2[1], 3);
      PLASMA_TEST_INT(a2[4], 42);
      PLASMA_TEST_INT(a2[9], 42);
    }

    a2.Clear();
    a2.PushBack(1);
    a2.PushBack(2);
    a2.PushBack(3);

    a2.SetCount(2, 10);
    if (PLASMA_TEST_INT(a2.GetCount(), 2))
    {
      PLASMA_TEST_INT(a2[0], 1);
      PLASMA_TEST_INT(a2[1], 2);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    plSmallArray<plInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1;

    // always inserts at the front
    for (plInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], 99 - i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndCopy")
  {
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1;

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
    plSmallArray<plInt32, 16> a1;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExpandAndGetRef")
  {
    plSmallArray<plInt32, 16> a1;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Construction / Destruction")
  {
    {
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

      plSmallArray<plConstructionCounter, 16> a1;
      plSmallArray<plConstructionCounter, 16> a2;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compact")
  {
    plSmallArray<plInt32, 16> a;

    for (plInt32 i = 0; i < 1008; ++i)
    {
      a.PushBack(i);
      PLASMA_TEST_INT(a.GetCount(), i + 1);
    }

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (plInt32 i = 0; i < 1008; ++i)
      PLASMA_TEST_INT(a[i], i);

    // this tests whether the static array is reused properly
    a.SetCount(15);
    a.Compact();
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 15; ++i)
      PLASMA_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingPrimitives")
  {
    plSmallArray<plUInt32, 16> list;

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
    plSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (plUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    SmallArrayTestDetail::Dummy last = 0;
    for (plUInt32 i = 0; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Various")
  {
    plSmallArray<SmallArrayTestDetail::Dummy, 16> list;
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
    SmallArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    PLASMA_TEST_BOOL(d.a == 5);
    PLASMA_TEST_BOOL(list.GetCount() == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment")
  {
    plSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.GetUserData<plUInt32>() = 11;

    plSmallArray<SmallArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list2.GetUserData<plUInt32>() = 22;

    list = list2;
    PLASMA_TEST_INT(list.GetCount(), list2.GetCount());
    PLASMA_TEST_INT(list.GetUserData<plUInt32>(), list2.GetUserData<plUInt32>());

    list2.Clear();
    PLASMA_TEST_BOOL(list2.GetCount() == 0);

    list2 = list;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);

    for (int i = 0; i < 16; i++)
    {
      list2.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Count")
  {
    plSmallArray<SmallArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(SmallArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reserve")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    plSmallArray<plConstructionCounter, 16> a;

    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    a.Reserve(100);

    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    a.SetCount(10);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(10, 0));

    a.Reserve(100);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 0));

    a.SetCount(100);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(90, 0));

    a.Reserve(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 0));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compact")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    plSmallArray<plConstructionCounter, 16> a;

    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    a.SetCount(100);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 0));

    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(200, 100));

    a.SetCount(10);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(190, 0));

    a.SetCount(10);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 200));

    a.SetCount(100);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 0));

    a.Clear();
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 200));

    // this will deallocate ALL memory
    a.Compact();

    a.SetCount(10);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(10, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(200, 10));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
  {
    plSmallArray<plInt32, 16> a1;

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
    const plSmallArray<plInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[400]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
  {
    plSmallArray<plInt32, 16> a1;

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
    const plSmallArray<plInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      plSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      PLASMA_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      PLASMA_TEST_BOOL(counter == 1);

      b = std::move(a);
      PLASMA_TEST_BOOL(counter == 1);
    }
    PLASMA_TEST_BOOL(counter == 2);

    counter = 0;
    {
      plSmallArray<SmallArrayTestDetail::ExternalCounter, 2> a, b;
      PLASMA_TEST_BOOL(counter == 0);

      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      a.PushBack(SmallArrayTestDetail::ExternalCounter(counter));
      PLASMA_TEST_BOOL(counter == 4);

      b = std::move(a);
      PLASMA_TEST_BOOL(counter == 4);
    }
    PLASMA_TEST_BOOL(counter == 8);
  }
}
