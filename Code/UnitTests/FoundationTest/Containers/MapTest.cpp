#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Map.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>
#include <algorithm>
#include <iterator>

PLASMA_CREATE_SIMPLE_TEST(Containers, Map)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Iterator")
  {
    plMap<plUInt32, plUInt32> m;
    for (plUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // PLASMA_TEST_INT(std::find(begin(m), end(m), 500).Key(), 499);

    auto itfound = std::find_if(begin(m), end(m), [](plMap<plUInt32, plUInt32>::ConstIterator val) { return val.Value() == 500; });

    // PLASMA_TEST_BOOL(std::find(begin(m), end(m), 500) == itfound);

    plUInt32 prev = begin(m).Key();
    for (auto it : m)
    {
      PLASMA_TEST_BOOL(it.Value() >= prev);
      prev = it.Value();
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plMap<plUInt32, plUInt32> m;
    plMap<plConstructionCounter, plUInt32> m2;
    plMap<plConstructionCounter, plConstructionCounter> m3;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsEmpty")
  {
    plMap<plUInt32, plUInt32> m;
    PLASMA_TEST_BOOL(m.IsEmpty());

    m[1] = 2;
    PLASMA_TEST_BOOL(!m.IsEmpty());

    m.Clear();
    PLASMA_TEST_BOOL(m.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCount")
  {
    plMap<plUInt32, plUInt32> m;
    PLASMA_TEST_INT(m.GetCount(), 0);

    m[0] = 1;
    PLASMA_TEST_INT(m.GetCount(), 1);

    m[1] = 2;
    PLASMA_TEST_INT(m.GetCount(), 2);

    m[2] = 3;
    PLASMA_TEST_INT(m.GetCount(), 3);

    m[0] = 1;
    PLASMA_TEST_INT(m.GetCount(), 3);

    m.Clear();
    PLASMA_TEST_INT(m.GetCount(), 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());

    {
      plMap<plUInt32, plConstructionCounter> m1;
      m1[0] = plConstructionCounter(1);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[1] = plConstructionCounter(3);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(3, 2)); // for inserting new elements 2 temporaries are created (and destroyed)

      m1[0] = plConstructionCounter(2);
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 2));
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    }

    {
      plMap<plConstructionCounter, plUInt32> m1;
      m1[plConstructionCounter(0)] = 1;
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary

      m1[plConstructionCounter(1)] = 3;
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(2, 1)); // one temporary

      m1[plConstructionCounter(0)] = 2;
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(plConstructionCounter::HasDone(0, 2));
      PLASMA_TEST_BOOL(plConstructionCounter::HasAllDestructed());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plMap<plUInt32, plUInt32> m;

    PLASMA_TEST_BOOL(m.GetHeapMemoryUsage() == 0);

    PLASMA_TEST_BOOL(m.Insert(1, 10).IsValid());
    PLASMA_TEST_BOOL(m.Insert(1, 10).IsValid());
    m.Insert(3, 30);
    auto it7 = m.Insert(7, 70);
    m.Insert(9, 90);
    m.Insert(4, 40);
    m.Insert(2, 20);
    m.Insert(8, 80);
    m.Insert(5, 50);
    m.Insert(6, 60);

    PLASMA_TEST_BOOL(m.Insert(7, 70).Value() == 70);
    PLASMA_TEST_BOOL(m.Insert(7, 70) == it7);

    PLASMA_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(plUInt32) * 2 * 9);

    PLASMA_TEST_INT(m[1], 10);
    PLASMA_TEST_INT(m[2], 20);
    PLASMA_TEST_INT(m[3], 30);
    PLASMA_TEST_INT(m[4], 40);
    PLASMA_TEST_INT(m[5], 50);
    PLASMA_TEST_INT(m[6], 60);
    PLASMA_TEST_INT(m[7], 70);
    PLASMA_TEST_INT(m[8], 80);
    PLASMA_TEST_INT(m[9], 90);

    PLASMA_TEST_INT(m.GetCount(), 9);

    for (plUInt32 i = 0; i < 1000000; ++i)
      m[i] = i;

    PLASMA_TEST_BOOL(m.GetHeapMemoryUsage() >= sizeof(plUInt32) * 2 * 1000000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Find")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m.Find(i).Value(), i * 10);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValue/TryGetValue")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (plInt32 i = 100 - 1; i >= 0; --i)
    {
      PLASMA_TEST_INT(*m.GetValue(i), i * 10);

      plUInt32 v = 0;
      PLASMA_TEST_BOOL(m.TryGetValue(i, v));
      PLASMA_TEST_INT(v, i * 10);

      plUInt32* pV = nullptr;
      PLASMA_TEST_BOOL(m.TryGetValue(i, pV));
      PLASMA_TEST_INT(*pV, i * 10);
    }

    PLASMA_TEST_BOOL(m.GetValue(101) == nullptr);

    plUInt32 v = 0;
    PLASMA_TEST_BOOL(m.TryGetValue(101, v) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValue/TryGetValue (const)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    const plMap<plUInt32, plUInt32>& mConst = m;

    for (plInt32 i = 100 - 1; i >= 0; --i)
    {
      PLASMA_TEST_INT(*mConst.GetValue(i), i * 10);

      plUInt32 v = 0;
      PLASMA_TEST_BOOL(m.TryGetValue(i, v));
      PLASMA_TEST_INT(v, i * 10);

      plUInt32* pV = nullptr;
      PLASMA_TEST_BOOL(m.TryGetValue(i, pV));
      PLASMA_TEST_INT(*pV, i * 10);
    }

    PLASMA_TEST_BOOL(mConst.GetValue(101) == nullptr);

    plUInt32 v = 0;
    PLASMA_TEST_BOOL(mConst.TryGetValue(101, v) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValueOrDefault")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 100; ++i)
      m[i] = i * 10;

    for (plInt32 i = 100 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m.GetValueOrDefault(i, 999), i * 10);

    PLASMA_TEST_BOOL(m.GetValueOrDefault(101, 999) == 999);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; i += 2)
      m[i] = i * 10;

    for (plInt32 i = 0; i < 1000; i += 2)
    {
      PLASMA_TEST_BOOL(m.Contains(i));
      PLASMA_TEST_BOOL(!m.Contains(i + 1));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindOrAdd")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
    {
      bool bExisted = true;
      m.FindOrAdd(i, &bExisted).Value() = i * 10;
      PLASMA_TEST_BOOL(!bExisted);
    }

    for (plInt32 i = 1000 - 1; i >= 0; --i)
    {
      bool bExisted = false;
      PLASMA_TEST_INT(m.FindOrAdd(i, &bExisted).Value(), i * 10);
      PLASMA_TEST_BOOL(bExisted);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator[]")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m[i], i * 10);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (non-existing)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(!m.Remove(i));
    }

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (plInt32 i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(m.Remove(i + 500) == (i < 500));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (Iterator)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (plInt32 i = 0; i < 1000 - 1; ++i)
    {
      plMap<plUInt32, plUInt32>::Iterator itNext = m.Remove(m.Find(i));
      PLASMA_TEST_BOOL(!m.Find(i).IsValid());
      PLASMA_TEST_BOOL(itNext.Key() == i + 1);

      PLASMA_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (Key)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    for (plInt32 i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(m.Remove(i));
      PLASMA_TEST_BOOL(!m.Find(i).IsValid());

      PLASMA_TEST_INT(m.GetCount(), 1000 - 1 - i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator=")
  {
    plMap<plUInt32, plUInt32> m, m2;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    m2 = m;

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m2[i], i * 10);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    plMap<plUInt32, plUInt32> m2(m);

    for (plInt32 i = 1000 - 1; i >= 0; --i)
      PLASMA_TEST_INT(m2[i], i * 10);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIterator / Forward Iteration")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    plInt32 i = 0;
    for (plMap<plUInt32, plUInt32>::Iterator it = m.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      PLASMA_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetIterator / Forward Iteration (const)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const plMap<plUInt32, plUInt32> m2(m);

    plInt32 i = 0;
    for (plMap<plUInt32, plUInt32>::ConstIterator it = m2.GetIterator(); it.IsValid(); ++it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      PLASMA_TEST_INT(it.Value(), i * 10);
      ++i;
    }

    PLASMA_TEST_INT(i, 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLastIterator / Backward Iteration")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    plInt32 i = 1000 - 1;
    for (plMap<plUInt32, plUInt32>::Iterator it = m.GetLastIterator(); it.IsValid(); --it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      PLASMA_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetLastIterator / Backward Iteration (const)")
  {
    plMap<plUInt32, plUInt32> m;

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    const plMap<plUInt32, plUInt32> m2(m);

    plInt32 i = 1000 - 1;
    for (plMap<plUInt32, plUInt32>::ConstIterator it = m2.GetLastIterator(); it.IsValid(); --it)
    {
      PLASMA_TEST_INT(it.Key(), i);
      PLASMA_TEST_INT(it.Value(), i * 10);
      --i;
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "LowerBound")
  {
    plMap<plInt32, plInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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
    plMap<plInt32, plInt32> m, m2;

    m[0] = 0;
    m[3] = 30;
    m[7] = 70;
    m[9] = 90;

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

    plMap<plInt32, plInt32> m;

    for (plUInt32 r = 0; r < 5; ++r)
    {
      // Insert
      for (plUInt32 i = 0; i < 10000; ++i)
        m.Insert(i, i * 10);

      PLASMA_TEST_INT(m.GetCount(), 10000);

      // Remove
      for (plUInt32 i = 0; i < 5000; ++i)
        PLASMA_TEST_BOOL(m.Remove(i));

      // Insert others
      for (plUInt32 j = 1; j < 1000; ++j)
        m.Insert(20000 * j, j);

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator == / !=")
  {
    plMap<plUInt32, plUInt32> m, m2;

    PLASMA_TEST_BOOL(m == m2);

    for (plInt32 i = 0; i < 1000; ++i)
      m[i] = i * 10;

    PLASMA_TEST_BOOL(m != m2);

    m2 = m;

    PLASMA_TEST_BOOL(m == m2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompatibleKeyType")
  {
    {
      plMap<plString, int> stringTable;
      const char* szChar = "Char";
      const char* szString = "ViewBla";
      plStringView sView(szString, szString + 4);
      plStringBuilder sBuilder("Builder");
      plString sString("String");
      stringTable.Insert(szChar, 1);
      stringTable.Insert(sView, 2);
      stringTable.Insert(sBuilder, 3);
      stringTable.Insert(sString, 4);

      PLASMA_TEST_BOOL(stringTable.Contains(szChar));
      PLASMA_TEST_BOOL(stringTable.Contains(sView));
      PLASMA_TEST_BOOL(stringTable.Contains(sBuilder));
      PLASMA_TEST_BOOL(stringTable.Contains(sString));

      PLASMA_TEST_INT(*stringTable.GetValue(szChar), 1);
      PLASMA_TEST_INT(*stringTable.GetValue(sView), 2);
      PLASMA_TEST_INT(*stringTable.GetValue(sBuilder), 3);
      PLASMA_TEST_INT(*stringTable.GetValue(sString), 4);

      PLASMA_TEST_BOOL(stringTable.Remove(szChar));
      PLASMA_TEST_BOOL(stringTable.Remove(sView));
      PLASMA_TEST_BOOL(stringTable.Remove(sBuilder));
      PLASMA_TEST_BOOL(stringTable.Remove(sString));
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

      plMap<TestDynArray, int> arrayTable;
      arrayTable.Insert(a, 1);
      arrayTable.Insert(b, 2);

      plArrayPtr<const int> aPtr = a.GetArrayPtr();
      plArrayPtr<const int> bPtr = b.GetArrayPtr();

      plUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

      bool existed;
      auto it = arrayTable.FindOrAdd(aPtr, &existed);
      PLASMA_TEST_BOOL(existed);

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      PLASMA_TEST_BOOL(arrayTable.Contains(aPtr));
      PLASMA_TEST_BOOL(arrayTable.Contains(bPtr));
      PLASMA_TEST_BOOL(arrayTable.Contains(a));

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      PLASMA_TEST_INT(*arrayTable.GetValue(aPtr), 1);
      PLASMA_TEST_INT(*arrayTable.GetValue(bPtr), 2);
      PLASMA_TEST_INT(*arrayTable.GetValue(a), 1);

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

      PLASMA_TEST_BOOL(arrayTable.Remove(aPtr));
      PLASMA_TEST_BOOL(arrayTable.Remove(bPtr));

      PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plStringBuilder tmp;
    plMap<plString, plInt32> map1;
    plMap<plString, plInt32> map2;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map1[tmp] = i;

      tmp.Format("{0}{0}{0}", i);
      map2[tmp] = i;
    }

    map1.Swap(map2);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(map2.Contains(tmp));
      PLASMA_TEST_INT(map2[tmp], i);

      tmp.Format("{0}{0}{0}", i);
      PLASMA_TEST_BOOL(map1.Contains(tmp));
      PLASMA_TEST_INT(map1[tmp], i);
    }
  }

  constexpr plUInt32 uiMapSize = sizeof(plMap<plString, plInt32>);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plUInt8 map1Mem[uiMapSize];
    plUInt8 map2Mem[uiMapSize];
    plMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    plMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    plStringBuilder tmp;
    plMap<plString, plInt32>* map1 = new (map1Mem)(plMap<plString, plInt32>);
    plMap<plString, plInt32>* map2 = new (map2Mem)(plMap<plString, plInt32>);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map1->Insert(tmp, i);

      tmp.Format("{0}{0}{0}", i);
      map2->Insert(tmp, i);
    }

    map1->Swap(*map2);

    // test swapped elements
    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(map2->Contains(tmp));
      PLASMA_TEST_INT((*map2)[tmp], i);

      tmp.Format("{0}{0}{0}", i);
      PLASMA_TEST_BOOL(map1->Contains(tmp));
      PLASMA_TEST_INT((*map1)[tmp], i);
    }

    // test iterators after swap
    {
      for (auto it : *map1)
      {
        PLASMA_TEST_BOOL(!map2->Contains(it.Key()));
      }

      for (auto it : *map2)
      {
        PLASMA_TEST_BOOL(!map1->Contains(it.Key()));
      }
    }

    // due to a compiler bug in VS 2017, PatternFill cannot be called here, because it will move the memset BEFORE the destructor call!
    // seems to be fixed in VS 2019 though

    map1->~plMap<plString, plInt32>();
    // plMemoryUtils::PatternFill(map1Mem, 0xBA, uiSetSize);

    map2->~plMap<plString, plInt32>();
    plMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap Empty")
  {
    plUInt8 map1Mem[uiMapSize];
    plUInt8 map2Mem[uiMapSize];
    plMemoryUtils::PatternFill(map1Mem, 0xCA, uiMapSize);
    plMemoryUtils::PatternFill(map2Mem, 0xCA, uiMapSize);

    plStringBuilder tmp;
    plMap<plString, plInt32>* map1 = new (map1Mem)(plMap<plString, plInt32>);
    plMap<plString, plInt32>* map2 = new (map2Mem)(plMap<plString, plInt32>);

    for (plUInt32 i = 0; i < 100; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map1->Insert(tmp, i);
    }

    map1->Swap(*map2);
    PLASMA_TEST_BOOL(map1->IsEmpty());

    map1->~plMap<plString, plInt32>();
    plMemoryUtils::PatternFill(map1Mem, 0xBA, uiMapSize);

    // test swapped elements
    for (plUInt32 i = 0; i < 100; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(map2->Contains(tmp));
    }

    // test iterators after swap
    {
      for (auto it : *map2)
      {
        PLASMA_TEST_BOOL(map2->Contains(it.Key()));
      }
    }

    map2->~plMap<plString, plInt32>();
    plMemoryUtils::PatternFill(map2Mem, 0xBA, uiMapSize);
  }
}