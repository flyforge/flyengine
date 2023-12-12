#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HybridArrayTestDetail
{

  class Dummy
  {
  public:
    int a = 0;
    std::string s = "Test";

    Dummy() = default;

    Dummy(int a)
      : a(a)
    {
    }

    Dummy(const Dummy& other) = default;
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
  static plHybridArray<T, 16> CreateArray(plUInt32 uiSize, plUInt32 uiOffset)
  {
    plHybridArray<T, 16> a;
    a.SetCount(uiSize);

    for (plUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

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
} // namespace HybridArrayTestDetail

static void TakesDynamicArray(plDynamicArray<int>& ref_ar, int iNum, int iStart);

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
static_assert(sizeof(plHybridArray<plInt32, 1>) == 32);
#else
static_assert(sizeof(plHybridArray<plInt32, 1>) == 20);
#endif

static_assert(plGetTypeClass<plHybridArray<plInt32, 1>>::value == plTypeIsClass::value);
static_assert(plGetTypeClass<plHybridArray<HybridArrayTestDetail::NonMovableClass, 1>>::value == plTypeIsClass::value);

PLASMA_CREATE_SIMPLE_TEST(Containers, HybridArray)
{
  plConstructionCounter::Reset();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plHybridArray<plInt32, 16> a1;
    plHybridArray<plConstructionCounter, 16> a2;

    PLASMA_TEST_BOOL(a1.GetCount() == 0);
    PLASMA_TEST_BOOL(a2.GetCount() == 0);
    PLASMA_TEST_BOOL(a1.IsEmpty());
    PLASMA_TEST_BOOL(a2.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plHybridArray<plInt32, 16> a1;

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

    plHybridArray<plInt32, 16> a2 = a1;
    plHybridArray<plInt32, 16> a3(a1);

    PLASMA_TEST_BOOL(a1 == a2);
    PLASMA_TEST_BOOL(a1 == a3);
    PLASMA_TEST_BOOL(a2 == a3);

    plInt32 test[] = {1, 2, 3, 4};
    plArrayPtr<plInt32> aptr(test);

    plHybridArray<plInt32, 16> a4(aptr);

    PLASMA_TEST_BOOL(a4 == aptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Constructor / Operator")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      // move constructor external storage
      plHybridArray<plConstructionCounter, 16> a1(HybridArrayTestDetail::CreateArray<plConstructionCounter>(100, 20));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<plConstructionCounter>(200, 50);

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    plConstructionCounter::Reset();

    {
      // move constructor internal storage
      plHybridArray<plConstructionCounter, 16> a2(HybridArrayTestDetail::CreateArray<plConstructionCounter>(10, 30));

      PLASMA_TEST_INT(a2.GetCount(), 10);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<plConstructionCounter>(8, 70);

      PLASMA_TEST_INT(a2.GetCount(), 8);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 70 + i);
    }

    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    plConstructionCounter::Reset();

    plConstructionCounterRelocatable::Reset();
    {
      // move constructor external storage relocatable
      plHybridArray<plConstructionCounterRelocatable, 16> a1(HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(100, 20));

      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(100, 0));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator external storage
      a1 = HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(200, 50);
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(200, 100));

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);
    }

    PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasAllDestructed());
    plConstructionCounterRelocatable::Reset();

    {
      // move constructor internal storage relocatable
      plHybridArray<plConstructionCounterRelocatable, 16> a2(HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(10, 30));
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(10, 0));

      PLASMA_TEST_INT(a2.GetCount(), 10);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 30 + i);

      // move operator internal storage
      a2 = HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(8, 70);
      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(8, 10));

      PLASMA_TEST_INT(a2.GetCount(), 8);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_INT(a2[i].m_iData, 70 + i);
    }

    PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasAllDestructed());
    plConstructionCounterRelocatable::Reset();

    {
      // move constructor with different allocators
      plProxyAllocator proxyAllocator("test allocator", plFoundation::GetDefaultAllocator());
      {
        plHybridArray<plConstructionCounterRelocatable, 16> a1(&proxyAllocator);

        a1 = HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(8, 70);
        PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(8, 0));
        PLASMA_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        PLASMA_TEST_INT(a1.GetCount(), 8);
        for (plUInt32 i = 0; i < a1.GetCount(); ++i)
          PLASMA_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = HybridArrayTestDetail::CreateArray<plConstructionCounterRelocatable>(32, 100);
        PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(32, 8));
        PLASMA_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        PLASMA_TEST_INT(a1.GetCount(), 32);
        for (plUInt32 i = 0; i < a1.GetCount(); ++i)
          PLASMA_TEST_INT(a1[i].m_iData, 100 + i);
      }

      PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasAllDestructed());
      plConstructionCounterRelocatable::Reset();

      auto allocatorStats = proxyAllocator.GetStats();
      PLASMA_TEST_BOOL(allocatorStats.m_uiNumAllocations == allocatorStats.m_uiNumDeallocations); // check for memory leak?
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Convert to ArrayPtr")
  {
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1, a2;

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
    plHybridArray<plInt32, 16> a1, a2;

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
    plHybridArray<plInt32, 16> a1;
    a1.SetCountUninitialized(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    const plHybridArray<plInt32, 16> ca1 = a1;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(ca1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    plHybridArray<plInt32, 16> a1;

    PLASMA_TEST_BOOL(a1.IsEmpty());

    for (plInt32 i = 0; i < 128; ++i)
    {
      a1.SetCount(i + 1);
      PLASMA_TEST_INT(a1[i], 0);
      a1[i] = i;

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

    a1.SetCountUninitialized(32);
    PLASMA_TEST_INT(a1.GetCount(), 32);
    a1[31] = 45;
    PLASMA_TEST_INT(a1[31], 45);

    // Test SetCount with fill value
    {
      plHybridArray<plInt32, 2> a2;
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
    plHybridArray<plInt32, 2> a2;
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
    plHybridArray<plInt32, 16> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

    // always inserts at the front
    for (plInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], 99 - i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndCopy")
  {
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

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
    plHybridArray<plInt32, 16> a1;

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

      plHybridArray<plConstructionCounter, 16> a1;
      plHybridArray<plConstructionCounter, 16> a2;

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
    plHybridArray<plInt32, 16> a;

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

    // this tests whether the static array is reused properly (not the case anymore with new implementation that derives from plDynamicArray)
    a.SetCount(15);
    a.Compact();
    // PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() > 0);

    for (plInt32 i = 0; i < 15; ++i)
      PLASMA_TEST_INT(a[i], i);

    a.Clear();
    a.Compact();
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingPrimitives")
  {
    plHybridArray<plUInt32, 16> list;

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
    plHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    list.Reserve(128);

    for (plUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    HybridArrayTestDetail::Dummy last = 0;
    for (plUInt32 i = 0; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(last <= list[i]);
      last = list[i];
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Various")
  {
    plHybridArray<HybridArrayTestDetail::Dummy, 16> list;
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
    HybridArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    PLASMA_TEST_BOOL(d.a == 5);
    PLASMA_TEST_BOOL(list.GetCount() == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment")
  {
    plHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    plHybridArray<HybridArrayTestDetail::Dummy, 16> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Count")
  {
    plHybridArray<HybridArrayTestDetail::Dummy, 16> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(HybridArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(4);

    list.Compact();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reserve")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    plHybridArray<plConstructionCounter, 16> a;

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

    plHybridArray<plConstructionCounter, 16> a;

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

    a.SetCount(100);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    PLASMA_TEST_BOOL(plConstructionCounter::HasDone(200, 100));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
  {
    plHybridArray<plInt32, 16> a1;

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
    const plHybridArray<plInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[400]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
  {
    plHybridArray<plInt32, 16> a1;

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
    const plHybridArray<plInt32, 16>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {

    plInt32 content1[] = {1, 2, 3, 4};
    plInt32 content2[] = {5, 6, 7, 8, 9};
    plInt32 contentHeap1[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13};
    plInt32 contentHeap2[] = {11, 12, 13, 14, 15, 16, 17, 18, 19, 110, 111, 112, 113};

    {
      // local <-> local
      plHybridArray<plInt32, 8> a1;
      plHybridArray<plInt32, 16> a2;
      a1 = plMakeArrayPtr(content1);
      a2 = plMakeArrayPtr(content2);

      plInt32* a1Ptr = a1.GetData();
      plInt32* a2Ptr = a2.GetData();

      a1.Swap(a2);

      // Because the data points to the internal storage the pointers shouldn't change when swapping
      PLASMA_TEST_BOOL(a1Ptr == a1.GetData());
      PLASMA_TEST_BOOL(a2Ptr == a2.GetData());

      // The data however should be swapped
      PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(content2));
      PLASMA_TEST_BOOL(a2.GetArrayPtr() == plMakeArrayPtr(content1));

      PLASMA_TEST_INT(a1.GetCapacity(), 8);
      PLASMA_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // local <-> heap
      plHybridArray<plInt32, 8> a1;
      plDynamicArray<plInt32> a2;
      a1 = plMakeArrayPtr(content1);
      a2 = plMakeArrayPtr(contentHeap1);
      plInt32* a1Ptr = a1.GetData();
      plInt32* a2Ptr = a2.GetData();
      a1.Swap(a2);
      PLASMA_TEST_BOOL(a1Ptr != a1.GetData());
      PLASMA_TEST_BOOL(a2Ptr != a2.GetData());
      PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(contentHeap1));
      PLASMA_TEST_BOOL(a2.GetArrayPtr() == plMakeArrayPtr(content1));

      PLASMA_TEST_INT(a1.GetCapacity(), 16);
      PLASMA_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> local
      plHybridArray<plInt32, 8> a1;
      plHybridArray<plInt32, 7> a2;
      a1 = plMakeArrayPtr(content1);
      a2 = plMakeArrayPtr(contentHeap1);
      plInt32* a1Ptr = a1.GetData();
      plInt32* a2Ptr = a2.GetData();
      a2.Swap(a1); // Swap is opposite direction as before
      PLASMA_TEST_BOOL(a1Ptr != a1.GetData());
      PLASMA_TEST_BOOL(a2Ptr != a2.GetData());
      PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(contentHeap1));
      PLASMA_TEST_BOOL(a2.GetArrayPtr() == plMakeArrayPtr(content1));

      PLASMA_TEST_INT(a1.GetCapacity(), 16);
      PLASMA_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // heap <-> heap
      plDynamicArray<plInt32> a1;
      plHybridArray<plInt32, 8> a2;
      a1 = plMakeArrayPtr(contentHeap1);
      a2 = plMakeArrayPtr(contentHeap2);
      plInt32* a1Ptr = a1.GetData();
      plInt32* a2Ptr = a2.GetData();
      a2.Swap(a1);
      PLASMA_TEST_BOOL(a1Ptr != a1.GetData());
      PLASMA_TEST_BOOL(a2Ptr != a2.GetData());
      PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(contentHeap2));
      PLASMA_TEST_BOOL(a2.GetArrayPtr() == plMakeArrayPtr(contentHeap1));

      PLASMA_TEST_INT(a1.GetCapacity(), 16);
      PLASMA_TEST_INT(a2.GetCapacity(), 16);
    }

    {
      // empty <-> local
      plHybridArray<plInt32, 8> a1, a2;
      a2 = plMakeArrayPtr(content2);
      a1.Swap(a2);
      PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(content2));
      PLASMA_TEST_BOOL(a2.IsEmpty());

      PLASMA_TEST_INT(a1.GetCapacity(), 8);
      PLASMA_TEST_INT(a2.GetCapacity(), 8);
    }

    {
      // empty <-> empty
      plHybridArray<plInt32, 8> a1, a2;
      a1.Swap(a2);
      PLASMA_TEST_BOOL(a1.IsEmpty());
      PLASMA_TEST_BOOL(a2.IsEmpty());

      PLASMA_TEST_INT(a1.GetCapacity(), 8);
      PLASMA_TEST_INT(a2.GetCapacity(), 8);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move")
  {
    int counter = 0;
    {
      plHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      PLASMA_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      PLASMA_TEST_BOOL(counter == 1);

      b = std::move(a);
      PLASMA_TEST_BOOL(counter == 1);
    }
    PLASMA_TEST_BOOL(counter == 2);

    counter = 0;
    {
      plHybridArray<HybridArrayTestDetail::ExternalCounter, 2> a, b;
      PLASMA_TEST_BOOL(counter == 0);

      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      a.PushBack(HybridArrayTestDetail::ExternalCounter(counter));
      PLASMA_TEST_BOOL(counter == 4);

      b = std::move(a);
      PLASMA_TEST_BOOL(counter == 4);
    }
    PLASMA_TEST_BOOL(counter == 8);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Use plHybridArray with plDynamicArray")
  {
    plHybridArray<int, 16> a;

    TakesDynamicArray(a, 4, a.GetCount());
    PLASMA_TEST_INT(a.GetCount(), 4);
    PLASMA_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      PLASMA_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 12, a.GetCount());
    PLASMA_TEST_INT(a.GetCount(), 16);
    PLASMA_TEST_INT(a.GetCapacity(), 16);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      PLASMA_TEST_INT(a[i], i);
    }

    TakesDynamicArray(a, 8, a.GetCount());
    PLASMA_TEST_INT(a.GetCount(), 24);
    PLASMA_TEST_INT(a.GetCapacity(), 32);

    for (int i = 0; i < (int)a.GetCount(); ++i)
    {
      PLASMA_TEST_INT(a[i], i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Nested arrays")
  {
    plDynamicArray<plHybridArray<HybridArrayTestDetail::NonMovableClass, 4>> a;

    for (int i = 0; i < 100; ++i)
    {
      plHybridArray<HybridArrayTestDetail::NonMovableClass, 4> b;
      b.PushBack(HybridArrayTestDetail::NonMovableClass(i));

      a.PushBack(std::move(b));
    }

    for (int i = 0; i < 100; ++i)
    {
      auto& nonMoveable = a[i][0];

      PLASMA_TEST_INT(nonMoveable.m_val, i);
      PLASMA_TEST_BOOL(nonMoveable.m_pVal == &nonMoveable.m_val);
    }
  }
}

void TakesDynamicArray(plDynamicArray<int>& ref_ar, int iNum, int iStart)
{
  for (int i = 0; i < iNum; ++i)
  {
    ref_ar.PushBack(iStart + i);
  }
}
