#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Set.h>
#include <Foundation/Memory/CommonAllocators.h>

PLASMA_CREATE_SIMPLE_TEST(Containers, Set)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plSet<plUInt32> m;
    plSet<plConstructionCounter, plUInt32> m2;
    plSet<plConstructionCounter, plConstructionCounter> m3;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEmpty")
  {
    plSet<plUInt32> m;
    PLASMA_TEST_BOOL(m.IsEmpty());

    m.Insert(1);
    PLASMA_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    PLASMA_TEST_BOOL(m.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCount")
  {
    plSet<plUInt32> m;
    PLASMA_TEST_INT(m.GetCount(), 0);

    m.Insert(0);
    PLASMA_TEST_INT(m.GetCount(), 1);

    m.Insert(1);
    PLASMA_TEST_INT(m.GetCount(), 2);

    m.Insert(2);
    PLASMA_TEST_INT(m.GetCount(), 3);

    m.Insert(1);
    PLASMA_TEST_INT(m.GetCount(), 3);

    m.Clear();
    PLASMA_TEST_INT(m.GetCount(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plSet<plConstructionCounter> m1;
      m1.Insert(plConstructionCounter(1));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1));

      m1.Insert(plConstructionCounter(3));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1));

      m1.Insert(plConstructionCounter(1));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 2));
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    }

    {
      plSet<plConstructionCounter> m1;
      m1.Insert(plConstructionCounter(0));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(plConstructionCounter(1));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary

      m1.Insert(plConstructionCounter(0));
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 2));
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plSet<plUInt32> m;
    PLASMA_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    PLASMA_TEST_BOOL(m.Insert(1).IsValid());
    PLASMA_TEST_BOOL(m.Insert(1).IsValid());

    m.Insert(3);
    auto it7 = m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    PLASMA_TEST_BOOL(m.Insert(1).Key() == 1);
    PLASMA_TEST_BOOL(m.Insert(3).Key() == 3);
    PLASMA_TEST_BOOL(m.Insert(7) == it7);

    PLASMA_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(plUInt32) * 1 * 9);

    PLASMA_TEST_BOOL(m.Find(1).IsValid());
    PLASMA_TEST_BOOL(m.Find(2).IsValid());
    PLASMA_TEST_BOOL(m.Find(3).IsValid());
    PLASMA_TEST_BOOL(m.Find(4).IsValid());
    PLASMA_TEST_BOOL(m.Find(5).IsValid());
    PLASMA_TEST_BOOL(m.Find(6).IsValid());
    PLASMA_TEST_BOOL(m.Find(7).IsValid());
    PLASMA_TEST_BOOL(m.Find(8).IsValid());
    PLASMA_TEST_BOOL(m.Find(9).IsValid());

    PLASMA_TEST_BOOL(!m.Find(0).IsValid());
    PLASMA_TEST_BOOL(!m.Find(10).IsValid());

    PLASMA_TEST_INT(m.GetCount(), 9);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains")
  {
    plSet<plUInt32> m;
    m.Insert(1);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);
    m.Insert(4);
    m.Insert(2);
    m.Insert(8);
    m.Insert(5);
    m.Insert(6);

    PLASMA_TEST_BOOL(m.Contains(1));
    PLASMA_TEST_BOOL(m.Contains(2));
    PLASMA_TEST_BOOL(m.Contains(3));
    PLASMA_TEST_BOOL(m.Contains(4));
    PLASMA_TEST_BOOL(m.Contains(5));
    PLASMA_TEST_BOOL(m.Contains(6));
    PLASMA_TEST_BOOL(m.Contains(7));
    PLASMA_TEST_BOOL(m.Contains(8));
    PLASMA_TEST_BOOL(m.Contains(9));

    PLASMA_TEST_BOOL(!m.Contains(0));
    PLASMA_TEST_BOOL(!m.Contains(10));

    PLASMA_TEST_INT(m.GetCount(), 9);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Set Operations")
  {
    plSet<plUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    plSet<plUInt32> empty;

    plSet<plUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    plSet<plUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    plSet<plUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    plSet<plUInt32> nonDisjunctNonEmptySubSet;
    nonDisjunctNonEmptySubSet.Insert(1);
    nonDisjunctNonEmptySubSet.Insert(4);
    nonDisjunctNonEmptySubSet.Insert(5);

    // ContainsSet
    PLASMA_TEST_BOOL(base.ContainsSet(base));

    PLASMA_TEST_BOOL(base.ContainsSet(empty));
    PLASMA_TEST_BOOL(!empty.ContainsSet(base));

    PLASMA_TEST_BOOL(!base.ContainsSet(disjunct));
    PLASMA_TEST_BOOL(!disjunct.ContainsSet(base));

    PLASMA_TEST_BOOL(base.ContainsSet(subSet));
    PLASMA_TEST_BOOL(!subSet.ContainsSet(base));

    PLASMA_TEST_BOOL(!base.ContainsSet(superSet));
    PLASMA_TEST_BOOL(superSet.ContainsSet(base));

    PLASMA_TEST_BOOL(!base.ContainsSet(nonDisjunctNonEmptySubSet));
    PLASMA_TEST_BOOL(!nonDisjunctNonEmptySubSet.ContainsSet(base));

    // Union
    {
      plSet<plUInt32> res;

      res.Union(base);
      PLASMA_TEST_BOOL(res.ContainsSet(base));
      PLASMA_TEST_BOOL(base.ContainsSet(res));
      res.Union(subSet);
      PLASMA_TEST_BOOL(res.ContainsSet(base));
      PLASMA_TEST_BOOL(res.ContainsSet(subSet));
      PLASMA_TEST_BOOL(base.ContainsSet(res));
      res.Union(superSet);
      PLASMA_TEST_BOOL(res.ContainsSet(base));
      PLASMA_TEST_BOOL(res.ContainsSet(subSet));
      PLASMA_TEST_BOOL(res.ContainsSet(superSet));
      PLASMA_TEST_BOOL(superSet.ContainsSet(res));
    }

    // Difference
    {
      plSet<plUInt32> res;
      res.Union(base);
      res.Difference(empty);
      PLASMA_TEST_BOOL(res.ContainsSet(base));
      PLASMA_TEST_BOOL(base.ContainsSet(res));
      res.Difference(disjunct);
      PLASMA_TEST_BOOL(res.ContainsSet(base));
      PLASMA_TEST_BOOL(base.ContainsSet(res));
      res.Difference(subSet);
      PLASMA_TEST_INT(res.GetCount(), 1);
      PLASMA_TEST_BOOL(res.Contains(3));
    }

    // Intersection
    {
      plSet<plUInt32> res;
      res.Union(base);
      res.Intersection(disjunct);
      PLASMA_TEST_BOOL(res.IsEmpty());
      res.Union(base);
      res.Intersection(subSet);
      PLASMA_TEST_BOOL(base.ContainsSet(subSet));
      PLASMA_TEST_BOOL(res.ContainsSet(subSet));
      PLASMA_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(superSet);
      PLASMA_TEST_BOOL(superSet.ContainsSet(res));
      PLASMA_TEST_BOOL(res.ContainsSet(subSet));
      PLASMA_TEST_BOOL(subSet.ContainsSet(res));
      res.Intersection(empty);
      PLASMA_TEST_BOOL(res.IsEmpty());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Find")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m.Find(i).Key(), i);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (non-existing)")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      PLASMA_TEST_BOOL(!m.Remove(i));

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (plInt32 i = 0; i < 1000; ++i)
      PLASMA_TEST_BOOL(m.Remove(i + 500) == (i < 500));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (Iterator)")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (plInt32 i = 0; i < 1000 - 1; ++i)
    {
      plSet<plUInt32>::Iterator itNext = m.Remove(m.Find(i));
      PLASMA_TEST_BOOL(!m.Find(i).IsValid());
      PLASMA_TEST_BOOL(itNext.Key() == i + 1);

      PLASMA_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (Key)")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    for (plInt32 i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(m.Remove(i));
      PLASMA_TEST_BOOL(!m.Find(i).IsValid());

      PLASMA_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=")
  {
    plSet<plUInt32> m, m2;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    m2 = m;

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_BOOL(m2.Find(i).IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    plSet<plUInt32> m2(m);

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_BOOL(m2.Find(i).IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    plInt32 i = 0;
    for (plSet<plUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const plSet<plUInt32> m2(m);

    plInt32 i = 0;
    for (plSet<plUInt32>::Iterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLastIterator / Backward Iteration")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    plInt32 i = 1000 - 1;
    for (plSet<plUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      --i;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLastIterator / Backward Iteration (const)")
  {
    plSet<plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i);

    const plSet<plUInt32> m2(m);

    plInt32 i = 1000 - 1;
    for (plSet<plUInt32>::Iterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      --i;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LowerBound")
  {
    plSet<plInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    PLASMA_TEST_INT(m.LowerBound(-1).Key(), 0);
    PLASMA_TEST_INT(m.LowerBound(0).Key(), 0);
    PLASMA_TEST_INT(m.LowerBound(1).Key(), 3);
    PLASMA_TEST_INT(m.LowerBound(2).Key(), 3);
    PLASMA_TEST_INT(m.LowerBound(3).Key(), 3);
    PLASMA_TEST_INT(m.LowerBound(4).Key(), 7);
    PLASMA_TEST_INT(m.LowerBound(5).Key(), 7);
    PLASMA_TEST_INT(m.LowerBound(6).Key(), 7);
    PLASMA_TEST_INT(m.LowerBound(7).Key(), 7);
    PLASMA_TEST_INT(m.LowerBound(8).Key(), 9);
    PLASMA_TEST_INT(m.LowerBound(9).Key(), 9);

    PLASMA_TEST_BOOL(!m.LowerBound(10).IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "UpperBound")
  {
    plSet<plInt32> m, m2;

    m.Insert(0);
    m.Insert(3);
    m.Insert(7);
    m.Insert(9);

    PLASMA_TEST_INT(m.UpperBound(-1).Key(), 0);
    PLASMA_TEST_INT(m.UpperBound(0).Key(), 3);
    PLASMA_TEST_INT(m.UpperBound(1).Key(), 3);
    PLASMA_TEST_INT(m.UpperBound(2).Key(), 3);
    PLASMA_TEST_INT(m.UpperBound(3).Key(), 7);
    PLASMA_TEST_INT(m.UpperBound(4).Key(), 7);
    PLASMA_TEST_INT(m.UpperBound(5).Key(), 7);
    PLASMA_TEST_INT(m.UpperBound(6).Key(), 7);
    PLASMA_TEST_INT(m.UpperBound(7).Key(), 9);
    PLASMA_TEST_INT(m.UpperBound(8).Key(), 9);
    PLASMA_TEST_BOOL(!m.UpperBound(9).IsValid());
    PLASMA_TEST_BOOL(!m.UpperBound(10).IsValid());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert / Remove")
  {
    // Tests whether reusing of elements makes problems

    plSet<plInt32> m;

    for (plUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (plUInt32 i = 0; i < 10000; ++i)
        m.Insert(i);

      PLASMA_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (plUInt32 i = 0; i < 5000; ++i)
        PLASMA_TEST_BOOL(m.Remove(i));

      // Insert others
      for (plUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j);

      // Remove
      for (plUInt32 i = 0; i < 5000; ++i)
        PLASMA_TEST_BOOL(m.Remove(5000 + i));

      // Remove others
      for (plUInt32 j = 1; j < 1000; ++j)
      {
        PLASMA_TEST_BOOL(m.Find(20000 * j).IsValid());
        PLASMA_TEST_BOOL(m.Remove(20000 * j));
      }
    }

    PLASMA_TEST_BOOL(m.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Iterator")
  {
    plSet<plUInt32> m;
    for (plUInt32 i = 0; i < 1000; ++i)
      m.Insert(i + 1);

    PLASMA_TEST_INT(std::find(begin(m), end(m), 500).Key(), 500);

    auto itfound = std::find_if(begin(m), end(m), [](plUInt32 uiVal) { return uiVal == 500; });

    PLASMA_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    plUInt32 prev = *begin(m);
    for (plUInt32 val : m)
    {
      PLASMA_TEST_BOOL(val >= prev);
      prev = val;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=")
  {
    plSet<plUInt32> m, m2;

    PLASMA_TEST_BOOL(m == m2);

    for (plInt32 i = 0; i < 1000; ++i)
      m.Insert(i * 10);

    PLASMA_TEST_BOOL(m != m2);

    m2 = m;

    PLASMA_TEST_BOOL(m == m2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      plSet<plString> stringSet;
      const char* szChar = "Char";
      const char* szString = "ViewBla";
      plStringView sView(szString, szString + 4);
      plStringBuilder sBuilder("Builder");
      plString sString("String");
      stringSet.Insert(szChar);
      stringSet.Insert(sView);
      stringSet.Insert(sBuilder);
      stringSet.Insert(sString);

      PLASMA_TEST_BOOL(stringSet.Contains(szChar));
      PLASMA_TEST_BOOL(stringSet.Contains(sView));
      PLASMA_TEST_BOOL(stringSet.Contains(sBuilder));
      PLASMA_TEST_BOOL(stringSet.Contains(sString));

      PLASMA_TEST_BOOL(stringSet.Remove(szChar));
      PLASMA_TEST_BOOL(stringSet.Remove(sView));
      PLASMA_TEST_BOOL(stringSet.Remove(sBuilder));
      PLASMA_TEST_BOOL(stringSet.Remove(sString));
    }

    // dynamic array as key, check for allocations in comparisons
    {
      plProxyAllocator testAllocator("Test", plFoundation::GetDefaultAllocator());
      plLocalAllocatorWrapper allocWrapper(&testAllocator);
      using TestDynArray = plDynamicArray<int, plLocalAllocatorWrapper>;
      TestDynArray a;
      TestDynArray b;
      for (int i = 0; i < 10; ++i)
      {
        a.PushBack(i);
        b.PushBack(i * 2);
      }

      plSet<TestDynArray> arraySet;
      arraySet.Insert(a);
      arraySet.Insert(b);

      plArrayPtr<const int> aPtr = a.GetArrayPtr();
      plArrayPtr<const int> bPtr = b.GetArrayPtr();

      plUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      PLASMA_TEST_BOOL(arraySet.Contains(aPtr));
      PLASMA_TEST_BOOL(arraySet.Contains(bPtr));
      PLASMA_TEST_BOOL(arraySet.Contains(a));

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      PLASMA_TEST_BOOL(arraySet.Remove(aPtr));
      PLASMA_TEST_BOOL(arraySet.Remove(bPtr));

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  constexpr plUInt32 uiSetSize = sizeof(plSet<plString>);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plUInt8 set1Mem[uiSetSize];
    plUInt8 set2Mem[uiSetSize];
    plMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    plMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    plStringBuilder tmp;
    plSet<plString>* set1 = new (set1Mem)(plSet<plString>);
    plSet<plString>* set2 = new (set2Mem)(plSet<plString>);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      set1->Insert(tmp);

      tmp.Format("{0}{0}{0}", i);
      set2->Insert(tmp);
    }

    set1->Swap(*set2);

    // test swapped elements
    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(set2->Contains(tmp));

      tmp.Format("{0}{0}{0}", i);
      PLASMA_TEST_BOOL(set1->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set1)
      {
        PLASMA_TEST_BOOL(!set2->Contains(element));
      }

      for (const auto& element : *set2)
      {
        PLASMA_TEST_BOOL(!set1->Contains(element));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    set1->~plSet<plString>();
    // plMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    set2->~plSet<plString>();
    plMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap Empty")
  {
    plUInt8 set1Mem[uiSetSize];
    plUInt8 set2Mem[uiSetSize];
    plMemoryUtils::PatternFill(set1Mem, 0xCA, uiSetSize);
    plMemoryUtils::PatternFill(set2Mem, 0xCA, uiSetSize);

    plStringBuilder tmp;
    plSet<plString>* set1 = new (set1Mem)(plSet<plString>);
    plSet<plString>* set2 = new (set2Mem)(plSet<plString>);

    for (plUInt32 i = 0; i < 100; ++i)
    {
      tmp.Format("stuff{}bla", i);
      set1->Insert(tmp);
    }

    set1->Swap(*set2);
    PLASMA_TEST_BOOL(set1->IsEmpty());

    set1->~plSet<plString>();
    plMemoryUtils::PatternFill(set1Mem, 0xBA, uiSetSize);

    // test swapped elements
    for (plUInt32 i = 0; i < 100; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(set2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (const auto& element : *set2)
      {
        PLASMA_TEST_BOOL(set2->Contains(element));
      }
    }

    set2->~plSet<plString>();
    plMemoryUtils::PatternFill(set2Mem, 0xBA, uiSetSize);
  }
}
