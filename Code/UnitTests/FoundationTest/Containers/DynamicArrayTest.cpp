#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/UniquePtr.h>

static plInt32 iCallPodConstructor = 0;
static plInt32 iCallPodDestructor = 0;
static plInt32 iCallNonPodConstructor = 0;
static plInt32 iCallNonPodDestructor = 0;

namespace DynamicArrayTestDetail
{
  using st = plConstructionCounter;

  static int g_iDummyCounter = 0;

  class Dummy
  {
  public:
    int a;
    int b;
    std::string s;

    Dummy()
      : a(0)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }
    Dummy(int a)
      : a(a)
      , b(g_iDummyCounter++)
      , s("Test")
    {
    }

    bool operator<=(const Dummy& dummy) const { return a <= dummy.a; }
    bool operator>=(const Dummy& dummy) const { return a >= dummy.a; }
    bool operator>(const Dummy& dummy) const { return a > dummy.a; }
    bool operator<(const Dummy& dummy) const { return a < dummy.a; }
    bool operator==(const Dummy& dummy) const { return a == dummy.a; }
  };

  plAllocatorBase* g_pTestAllocator;

  struct plTestAllocatorWrapper
  {
    static plAllocatorBase* GetAllocator() { return g_pTestAllocator; }
  };

  template <typename T = st, typename AllocatorWrapper = plTestAllocatorWrapper>
  static plDynamicArray<T, AllocatorWrapper> CreateArray(plUInt32 uiSize, plUInt32 uiOffset)
  {
    plDynamicArray<T, AllocatorWrapper> a;
    a.SetCount(uiSize);

    for (plUInt32 i = 0; i < uiSize; ++i)
      a[i] = T(uiOffset + i);

    return a;
  }
} // namespace DynamicArrayTestDetail

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)
PLASMA_CHECK_AT_COMPILETIME(sizeof(plDynamicArray<plInt32>) == 24);
#else
PLASMA_CHECK_AT_COMPILETIME(sizeof(plDynamicArray<plInt32>) == 16);
#endif

PLASMA_CREATE_SIMPLE_TEST_GROUP(Containers);

PLASMA_CREATE_SIMPLE_TEST(Containers, DynamicArray)
{
  plProxyAllocator proxy("DynamicArrayTestAllocator", plFoundation::GetDefaultAllocator());
  DynamicArrayTestDetail::g_pTestAllocator = &proxy;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plDynamicArray<plInt32> a1;
    plDynamicArray<DynamicArrayTestDetail::st> a2;

    PLASMA_TEST_BOOL(a1.GetCount() == 0);
    PLASMA_TEST_BOOL(a2.GetCount() == 0);
    PLASMA_TEST_BOOL(a1.IsEmpty());
    PLASMA_TEST_BOOL(a2.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plDynamicArray<plInt32, DynamicArrayTestDetail::plTestAllocatorWrapper> a1;

    PLASMA_TEST_BOOL(a1.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 32; ++i)
      a1.PushBack(rand() % 100000);

    PLASMA_TEST_BOOL(a1.GetHeapMemoryUsage() >= 32 * sizeof(plInt32));

    plDynamicArray<plInt32> a2 = a1;
    plDynamicArray<plInt32> a3(a1);

    PLASMA_TEST_BOOL(a1 == a2);
    PLASMA_TEST_BOOL(a1 == a3);
    PLASMA_TEST_BOOL(a2 == a3);

    plInt32 test[] = {1, 2, 3, 4};
    plArrayPtr<plInt32> aptr(test);

    plDynamicArray<plInt32> a4(aptr);

    PLASMA_TEST_BOOL(a4 == aptr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Constructor / Operator")
  {
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move constructor
      plDynamicArray<DynamicArrayTestDetail::st, DynamicArrayTestDetail::plTestAllocatorWrapper> a1(DynamicArrayTestDetail::CreateArray(100, 20));

      PLASMA_TEST_INT(a1.GetCount(), 100);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 20 + i);

      // move operator
      a1 = DynamicArrayTestDetail::CreateArray(200, 50);

      PLASMA_TEST_INT(a1.GetCount(), 200);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i].m_iData, 50 + i);
    }

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    {
      // move assignment with different allocators
      plConstructionCounterRelocatable::Reset();
      plProxyAllocator proxyAllocator("test allocator", plFoundation::GetDefaultAllocator());
      {
        plDynamicArray<plConstructionCounterRelocatable> a1(&proxyAllocator);

        a1 = DynamicArrayTestDetail::CreateArray<plConstructionCounterRelocatable, plDefaultAllocatorWrapper>(8, 70);
        PLASMA_TEST_BOOL(plConstructionCounterRelocatable::HasDone(8, 0));
        PLASMA_TEST_BOOL(a1.GetAllocator() == &proxyAllocator); // allocator must not change

        PLASMA_TEST_INT(a1.GetCount(), 8);
        for (plUInt32 i = 0; i < a1.GetCount(); ++i)
          PLASMA_TEST_INT(a1[i].m_iData, 70 + i);

        a1 = DynamicArrayTestDetail::CreateArray<plConstructionCounterRelocatable, plDefaultAllocatorWrapper>(32, 100);
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
    plDynamicArray<plInt32> a1;

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
    plDynamicArray<plInt32, DynamicArrayTestDetail::plTestAllocatorWrapper> a1;
    plDynamicArray<plInt32> a2;

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    a2 = a1;

    PLASMA_TEST_BOOL(a1 == a2);

    plArrayPtr<plInt32> arrayPtr(a1);

    a2 = arrayPtr;

    PLASMA_TEST_BOOL(a2 == arrayPtr);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=/ <")
  {
    plDynamicArray<plInt32> a1, a2;

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

    PLASMA_TEST_BOOL((a1 < a2) == false);
    a2.PushBack(100);
    PLASMA_TEST_BOOL(a1 < a2);
    a1.PushBack(99);
    PLASMA_TEST_BOOL(a1 < a2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Index operator")
  {
    plDynamicArray<plInt32> a1;
    a1.SetCountUninitialized(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1[i] = i;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    plDynamicArray<plInt32> ca1;
    ca1 = a1;

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(ca1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / GetCount / IsEmpty")
  {
    plDynamicArray<plInt32> a1;

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
      plDynamicArray<plInt32> a2;
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
    plDynamicArray<plInt32> a2;
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "EnsureCount")
  {
    plDynamicArray<plInt32> a1;

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
    plDynamicArray<plInt32> a1;
    a1.Clear();

    a1.PushBack(3);
    a1.Clear();

    PLASMA_TEST_BOOL(a1.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains / IndexOf / LastIndexOf")
  {
    plDynamicArray<plInt32> a1;

    for (plInt32 i = -100; i < 100; ++i)
      PLASMA_TEST_BOOL(!a1.Contains(i));

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);
    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBack(i);

    for (plInt32 i = 0; i < 100; ++i)
    {
      PLASMA_TEST_BOOL(a1.Contains(i));
      PLASMA_TEST_INT(a1.IndexOf(i), i);
      PLASMA_TEST_INT(a1.IndexOf(i, 100), i + 100);
      PLASMA_TEST_INT(a1.LastIndexOf(i), i + 100);
      PLASMA_TEST_INT(a1.LastIndexOf(i, 100), i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBackUnchecked / PushBackRange")
  {
    plDynamicArray<plInt32> a1;
    a1.Reserve(100);

    for (plInt32 i = 0; i < 100; ++i)
      a1.PushBackUnchecked(i);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], i);

    plInt32 temp[] = {100, 101, 102, 103, 104};
    plArrayPtr<plInt32> range(temp);

    a1.PushBackRange(range);

    PLASMA_TEST_INT(a1.GetCount(), 105);
    for (plUInt32 i = 0; i < a1.GetCount(); ++i)
      PLASMA_TEST_INT(a1[i], i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plDynamicArray<plInt32> a1;

    // always inserts at the front
    for (plInt32 i = 0; i < 100; ++i)
      a1.Insert(i, 0);

    for (plInt32 i = 0; i < 100; ++i)
      PLASMA_TEST_INT(a1[i], 99 - i);

    plUniquePtr<DynamicArrayTestDetail::st> ptr = PLASMA_DEFAULT_NEW(DynamicArrayTestDetail::st);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      plDynamicArray<plUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (plUInt32 i = 0; i < 10; ++i)
        a2.Insert(plUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.Insert(std::move(ptr), 0);
      PLASMA_TEST_BOOL(ptr == nullptr);
      PLASMA_TEST_BOOL(a2[0] != nullptr);

      for (plUInt32 i = 1; i < a2.GetCount(); ++i)
        PLASMA_TEST_BOOL(a2[i] == nullptr);
    }

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "InsertRange")
  {
    // Pod element tests
    plDynamicArray<plInt32> intTestRange;
    plDynamicArray<plInt32> a1;

    plInt32 intTemp1[] = {91, 92, 93, 94, 95};
    plArrayPtr<plInt32> intRange1(intTemp1);

    plInt32 intTemp2[] = {96, 97, 98, 99, 100};
    plArrayPtr<plInt32> intRange2(intTemp2);

    plInt32 intTemp3[] = {100, 101, 102, 103, 104};
    plArrayPtr<plInt32> intRange3(intTemp3);

    {
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange3, 0);

      PLASMA_TEST_INT(a1.GetCount(), 5);

      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange1, 0);

      PLASMA_TEST_INT(a1.GetCount(), 10);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i], intTestRange[i]);
    }

    {
      intTestRange.Clear();
      intTestRange.PushBackRange(intRange1);
      intTestRange.PushBackRange(intRange2);
      intTestRange.PushBackRange(intRange3);

      a1.InsertRange(intRange2, 5);

      PLASMA_TEST_INT(a1.GetCount(), 15);
      for (plUInt32 i = 0; i < a1.GetCount(); ++i)
        PLASMA_TEST_INT(a1[i], intTestRange[i]);
    }

    // Class element tests
    plDynamicArray<plDeque<plString>> classTestRange;
    plDynamicArray<plDeque<plString>> a2;

    plDeque<plString> strTemp1[4];
    {
      strTemp1[0].PushBack("One");
      strTemp1[1].PushBack("Two");
      strTemp1[2].PushBack("Three");
      strTemp1[3].PushBack("Four");
    }
    plArrayPtr<plDeque<plString>> classRange1(strTemp1);

    plDeque<plString> strTemp2[3];
    {
      strTemp2[0].PushBack("Five");
      strTemp2[1].PushBack("Six");
      strTemp2[2].PushBack("Seven");
    }
    plArrayPtr<plDeque<plString>> classRange2(strTemp2);

    plDeque<plString> strTemp3[3];
    {
      strTemp3[0].PushBack("Eight");
      strTemp3[1].PushBack("Nine");
      strTemp3[2].PushBack("Ten");
    }
    plArrayPtr<plDeque<plString>> classRange3(strTemp3);

    {
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange3, 0);

      PLASMA_TEST_INT(a2.GetCount(), 3);

      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange1, 0);

      PLASMA_TEST_INT(a2.GetCount(), 7);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }

    {
      classTestRange.Clear();
      classTestRange.PushBackRange(classRange1);
      classTestRange.PushBackRange(classRange2);
      classTestRange.PushBackRange(classRange3);

      a2.InsertRange(classRange2, 4);

      PLASMA_TEST_INT(a2.GetCount(), 10);
      for (plUInt32 i = 0; i < a2.GetCount(); ++i)
        PLASMA_TEST_STRING(a2[i].PeekFront(), classTestRange[i].PeekFront());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAndCopy")
  {
    plDynamicArray<plInt32> a1;

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
    plDynamicArray<plInt32> a1;

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
    plDynamicArray<plInt32> a1;

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

    plUniquePtr<DynamicArrayTestDetail::st> ptr = PLASMA_DEFAULT_NEW(DynamicArrayTestDetail::st);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      plDynamicArray<plUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (plUInt32 i = 0; i < 10; ++i)
        a2.Insert(plUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.PushBack(std::move(ptr));
      PLASMA_TEST_BOOL(ptr == nullptr);
      PLASMA_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndCopy(0);
      PLASMA_TEST_BOOL(a2[9] != nullptr);
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));
    }

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "RemoveAtAndSwap")
  {
    plDynamicArray<plInt32> a1;

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

    plUniquePtr<DynamicArrayTestDetail::st> ptr = PLASMA_DEFAULT_NEW(DynamicArrayTestDetail::st);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasConstructed(1));

    {
      plDynamicArray<plUniquePtr<DynamicArrayTestDetail::st>> a2;
      for (plUInt32 i = 0; i < 10; ++i)
        a2.Insert(plUniquePtr<DynamicArrayTestDetail::st>(), 0);

      a2.PushBack(std::move(ptr));
      PLASMA_TEST_BOOL(ptr == nullptr);
      PLASMA_TEST_BOOL(a2[10] != nullptr);

      a2.RemoveAtAndSwap(0);
      PLASMA_TEST_BOOL(a2[0] != nullptr);
    }

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "PushBack / PopBack / PeekBack")
  {
    plDynamicArray<plInt32> a1;

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
    plDynamicArray<plInt32> a1;

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
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      plDynamicArray<DynamicArrayTestDetail::st> a1;
      plDynamicArray<DynamicArrayTestDetail::st> a2;

      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

      a1.PushBack(DynamicArrayTestDetail::st(1));
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a1.Insert(DynamicArrayTestDetail::st(2), 0);
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 1)); // one temporary, one final (copy constructed)

      a2 = a1;
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(2, 0)); // two copies

      a1.Clear();
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 2));

      a1.PushBack(DynamicArrayTestDetail::st(3));
      a1.PushBack(DynamicArrayTestDetail::st(4));
      a1.PushBack(DynamicArrayTestDetail::st(5));
      a1.PushBack(DynamicArrayTestDetail::st(6));

      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(8, 4)); // four temporaries

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 2)); // one temporary, one destroyed

      a1.RemoveAndCopy(DynamicArrayTestDetail::st(3));
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(1, 1)); // one temporary, none destroyed

      a1.RemoveAtAndCopy(0);
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed

      a1.RemoveAtAndSwap(0);
      PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 1)); // one destroyed
    }

    // tests the destructor of a2 and a1
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingPrimitives")
  {
    plDynamicArray<plUInt32> list;

    list.Sort();

    for (plUInt32 i = 0; i < 450; i++)
    {
      list.PushBack(std::rand());
    }
    list.Sort();

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1] <= list[i]);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingObjects")
  {
    plDynamicArray<DynamicArrayTestDetail::Dummy> list;
    list.Reserve(128);

    for (plUInt32 i = 0; i < 100; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.Sort();

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1] <= list[i]);
      PLASMA_TEST_BOOL(list[i].s == "Test");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortingMovableObjects")
  {
    {
      plDynamicArray<plUniquePtr<DynamicArrayTestDetail::st>> list;
      list.Reserve(128);

      for (plUInt32 i = 0; i < 100; i++)
      {
        list.PushBack(PLASMA_DEFAULT_NEW(DynamicArrayTestDetail::st));
      }
      list.Sort();

      for (plUInt32 i = 1; i < list.GetCount(); i++)
      {
        PLASMA_TEST_BOOL(list[i - 1] <= list[i]);
      }
    }

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Various")
  {
    plDynamicArray<DynamicArrayTestDetail::Dummy> list;
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
    DynamicArrayTestDetail::Dummy d = list.PeekBack();
    list.PopBack();
    PLASMA_TEST_BOOL(d.a == 5);
    PLASMA_TEST_BOOL(list.GetCount() == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment")
  {
    plDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    plDynamicArray<DynamicArrayTestDetail::Dummy> list2;
    for (int i = 0; i < 8; i++)
    {
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
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
      list2.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }

    list = list2;
    PLASMA_TEST_BOOL(list.PeekBack() == list2.PeekBack());
    PLASMA_TEST_BOOL(list == list2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Count")
  {
    plDynamicArray<DynamicArrayTestDetail::Dummy> list;
    for (int i = 0; i < 16; i++)
    {
      list.PushBack(DynamicArrayTestDetail::Dummy(rand()));
    }
    list.SetCount(32);
    list.SetCount(16);

    list.Compact();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Reserve")
  {
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    plDynamicArray<DynamicArrayTestDetail::st> a;

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.Reserve(100);

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(10);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 0));

    a.Reserve(100);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0));

    a.SetCount(100);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(90, 0));

    a.Reserve(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 100)); // had to copy some elements over

    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compact")
  {
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    plDynamicArray<DynamicArrayTestDetail::st> a;

    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 0)); // nothing has been constructed / destructed in between
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasAllDestructed());

    a.SetCount(100);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));

    a.SetCount(10);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // no reallocations and copying, if the memory is already available
    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(190, 0));

    a.SetCount(10);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 190));

    // now we remove the spare memory
    a.Compact();
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(10, 10));

    // this time the array needs to be relocated, and thus the already present elements need to be copied
    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 10));

    // this does not deallocate memory
    a.Clear();
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    a.SetCount(100);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // therefore no object relocation
    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    a.Clear();
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(0, 200));

    // this will deallocate ALL memory
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() > 0);
    a.Compact();
    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    a.SetCount(100);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(100, 0));

    // this time objects need to be relocated
    a.SetCount(200);
    PLASMA_TEST_BOOL(DynamicArrayTestDetail::st::HasDone(200, 100));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Iterator")
  {
    plDynamicArray<plInt32> a1;

    for (plInt32 i = 0; i < 1000; ++i)
      a1.PushBack(1000 - i - 1);

    // STL sort
    std::sort(begin(a1), end(a1));

    for (plInt32 i = 1; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(a1[i - 1] <= a1[i]);
    }

    // foreach
    plInt32 prev = 0;
    plInt32 sum1 = 0;
    for (plInt32 val : a1)
    {
      PLASMA_TEST_BOOL(prev <= val);
      prev = val;
      sum1 += val;
    }

    prev = 1000;
    const auto endIt = rend(a1);
    plInt32 sum2 = 0;
    for (auto it = rbegin(a1); it != endIt; ++it)
    {
      PLASMA_TEST_BOOL(prev > (*it));
      prev = (*it);
      sum2 += (*it);
    }

    PLASMA_TEST_BOOL(sum1 == sum2);

    // const array
    const plDynamicArray<plInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(begin(a2), end(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[400]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "STL Reverse Iterator")
  {
    plDynamicArray<plInt32> a1;

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
    const plDynamicArray<plInt32>& a2 = a1;

    // STL lower bound
    auto lb = std::lower_bound(rbegin(a2), rend(a2), 400);
    PLASMA_TEST_BOOL(*lb == a2[1000 - 400 - 1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetArrayPtr")
  {
    plDynamicArray<plInt32> a1;
    a1.SetCountUninitialized(10);

    PLASMA_TEST_BOOL(a1.GetArrayPtr().GetCount() == 10);
    PLASMA_TEST_BOOL(a1.GetArrayPtr().GetPtr() == a1.GetData());

    const plDynamicArray<plInt32>& a1ref = a1;

    PLASMA_TEST_BOOL(a1ref.GetArrayPtr().GetCount() == 10);
    PLASMA_TEST_BOOL(a1ref.GetArrayPtr().GetPtr() == a1ref.GetData());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plDynamicArray<plInt32> a1, a2;

    plInt32 content1[] = {1, 2, 3, 4};
    plInt32 content2[] = {5, 6, 7, 8, 9};

    a1 = plMakeArrayPtr(content1);
    a2 = plMakeArrayPtr(content2);

    plInt32* a1Ptr = a1.GetData();
    plInt32* a2Ptr = a2.GetData();

    a1.Swap(a2);

    // The pointers should be simply swapped
    PLASMA_TEST_BOOL(a2Ptr == a1.GetData());
    PLASMA_TEST_BOOL(a1Ptr == a2.GetData());

    // The data should be swapped
    PLASMA_TEST_BOOL(a1.GetArrayPtr() == plMakeArrayPtr(content2));
    PLASMA_TEST_BOOL(a2.GetArrayPtr() == plMakeArrayPtr(content1));
  }

#if PLASMA_ENABLED(PLASMA_PLATFORM_64BIT)

  // disabled, because this is a very slow test
  PLASMA_TEST_BLOCK(plTestBlock::DisabledNoWarning, "Large Allocation")
  {
    const plUInt32 uiMaxNumElements = 0xFFFFFFFF - 16; // max supported elements due to alignment restrictions

    // this will allocate about 16 GB memory, the pure allocation is really fast
    plDynamicArray<plUInt32> byteArray;
    byteArray.SetCountUninitialized(uiMaxNumElements);

    const plUInt32 uiCheckElements = byteArray.GetCount();
    const plUInt32 uiSkipElements = 1024;

    // this will touch the memory and thus enforce that it is indeed made available by the OS
    // this takes a while
    for (plUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const plUInt32 idx = i & 0xFFFFFFFF;
      byteArray[idx] = idx;
    }

    // check that the assigned values are all correct
    // again, this takes quite a while
    for (plUInt64 i = 0; i < uiCheckElements; i += uiSkipElements)
    {
      const plUInt32 idx = i & 0xFFFFFFFF;
      PLASMA_TEST_INT(byteArray[idx], idx);
    }
  }
#endif


  const plUInt32 uiNumSortItems = 1'000'000;

  struct Item
  {
    bool operator<(const Item& rhs) const { return m_iKey < rhs.m_iKey; }

    plInt32 m_iKey = 0;
    plInt32 m_iIndex = 0;
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortLargeArray (pl-sort)")
  {
    plDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (plUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = std::rand();
    }

    plStopwatch sw;
    list.Sort();

    plTime t = sw.GetRunningTotal();
    plStringBuilder s;
    s.Format("pl-sort (random keys): {}", t);
    plTestFramework::Output(plTestOutput::Details, s);

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortLargeArray (std::sort)")
  {
    plDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (plUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = std::rand();
    }

    plStopwatch sw;
    std::sort(begin(list), end(list));

    plTime t = sw.GetRunningTotal();
    plStringBuilder s;
    s.Format("std::sort (random keys): {}", t);
    plTestFramework::Output(plTestOutput::Details, s);

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortLargeArray (equal keys) (pl-sort)")
  {
    plDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (plUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = 42;
    }

    plStopwatch sw;
    list.Sort();

    plTime t = sw.GetRunningTotal();
    plStringBuilder s;
    s.Format("pl-sort (equal keys): {}", t);
    plTestFramework::Output(plTestOutput::Details, s);

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SortLargeArray (equal keys) (std::sort)")
  {
    plDynamicArray<Item> list;
    list.Reserve(uiNumSortItems);

    for (plUInt32 i = 0; i < uiNumSortItems; i++)
    {
      auto& item = list.ExpandAndGetRef();
      item.m_iIndex = i;
      item.m_iKey = 42;
    }

    plStopwatch sw;
    std::sort(begin(list), end(list));

    plTime t = sw.GetRunningTotal();
    plStringBuilder s;
    s.Format("std::sort (equal keys): {}", t);
    plTestFramework::Output(plTestOutput::Details, s);

    for (plUInt32 i = 1; i < list.GetCount(); i++)
    {
      PLASMA_TEST_BOOL(list[i - 1].m_iKey <= list[i].m_iKey);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCountUninitialized")
  {
    struct POD
    {
      PLASMA_DECLARE_POD_TYPE();

      plUInt32 a = 2;
      plUInt32 b = 4;

      POD()
      {
        iCallPodConstructor++;
      }

      // this isn't allowed anymore in types that use PLASMA_DECLARE_POD_TYPE
      // unfortunately that means we can't do this kind of check either
      //~POD()
      //{
      //  iCallPodDestructor++;
      //}
    };

    static_assert(std::is_trivial<POD>::value == 0);
    static_assert(plIsPodType<POD>::value == 1);

    struct NonPOD
    {
      plUInt32 a = 3;
      plUInt32 b = 5;

      NonPOD()
      {
        iCallNonPodConstructor++;
      }

      ~NonPOD()
      {
        iCallNonPodDestructor++;
      }
    };

    static_assert(std::is_trivial<NonPOD>::value == 0);
    static_assert(plIsPodType<NonPOD>::value == 0);

    // check that SetCountUninitialized doesn't construct and Clear doesn't destruct POD types
    {
      plDynamicArray<POD> s1a;

      s1a.SetCountUninitialized(16);
      PLASMA_TEST_INT(iCallPodConstructor, 0);
      PLASMA_TEST_INT(iCallPodDestructor, 0);

      s1a.Clear();
      PLASMA_TEST_INT(iCallPodConstructor, 0);
      PLASMA_TEST_INT(iCallPodDestructor, 0);
    }

    // check that SetCount constructs and Clear destructs Non-POD types
    {
      plDynamicArray<NonPOD> s2a;

      s2a.SetCount(16);
      PLASMA_TEST_INT(iCallNonPodConstructor, 16);
      PLASMA_TEST_INT(iCallNonPodDestructor, 0);

      s2a.Clear();
      PLASMA_TEST_INT(iCallNonPodConstructor, 16);
      PLASMA_TEST_INT(iCallNonPodDestructor, 16);
    }
  }
}
