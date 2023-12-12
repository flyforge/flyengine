#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/ArrayMap.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

PLASMA_CREATE_SIMPLE_TEST(Containers, ArrayMap)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Iterator")
  {
    plArrayMap<plUInt32, plUInt32> m;
    for (plUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // non-const
    {
      // findable
      auto itfound = std::find_if(begin(m), end(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 500; });
      PLASMA_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(begin(m), end(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 1001; });
      PLASMA_TEST_BOOL(end(m) == itfound);
    }

    // const
    {
      // findable
      auto itfound = std::find_if(cbegin(m), cend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 500; });
      PLASMA_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(cbegin(m), cend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 1001; });
      PLASMA_TEST_BOOL(cend(m) == itfound);
    }

    // non-const reverse
    {
      // findable
      auto itfound = std::find_if(rbegin(m), rend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 500; });
      PLASMA_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(rbegin(m), rend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 1001; });
      PLASMA_TEST_BOOL(rend(m) == itfound);
    }

    // const reverse
    {
      // findable
      auto itfound = std::find_if(crbegin(m), crend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 500; });
      PLASMA_TEST_BOOL(findable == itfound->key);

      // unfindable
      itfound = std::find_if(crbegin(m), crend(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 1001; });
      PLASMA_TEST_BOOL(crend(m) == itfound);
    }

    // forward
    plUInt32 prev = begin(m)->key;
    for (const auto& elem : m)
    {
      PLASMA_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    PLASMA_TEST_BOOL(prev == 1000);

    // backward
    prev = (rbegin(m))->value + 1;
    for (auto it = rbegin(m); it < rend(m); ++it)
    {
      PLASMA_TEST_BOOL(it->value == prev - 1);
      prev = it->value;
    }

    PLASMA_TEST_BOOL(prev == 1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetData with Iterators")
  {
    plArrayMap<plUInt32, plUInt32> m;
    for (plUInt32 i = 0; i < 1000; ++i)
      m[i] = i + 1;

    // element with the given key (and such, value "key + 1")
    auto findable = m.Find(499u);

    // check if modification of the keys via direct data access
    // keeps iterability and access via keys intact

    // modify
    auto& data = m.GetData();
    for (auto& p : data)
    {
      p.key += 1000;
    }

    // ...and test with new key
    PLASMA_TEST_BOOL(m[findable + 1000] == 500);

    // and index...
    PLASMA_TEST_BOOL(m.GetValue(499u) == 500);

    // and old key.
    PLASMA_TEST_BOOL(m.Find(499u) == plInvalidIndex);

    // findable
    auto itfound = std::find_if(begin(m), end(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 500; });
    PLASMA_TEST_BOOL((findable + 1000) == itfound->key);

    // unfindable
    itfound = std::find_if(begin(m), end(m), [](const plArrayMap<plUInt32, plUInt32>::Pair& val) { return val.value == 1001; });
    PLASMA_TEST_BOOL(end(m) == itfound);

    // forward
    plUInt32 prev = 0;
    for (const auto& elem : m)
    {
      PLASMA_TEST_BOOL(elem.value == prev + 1);
      prev = elem.value;
    }

    PLASMA_TEST_BOOL(prev == 1000);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert / Find / Reserve / Clear / IsEmpty / Compact / GetCount")
  {
    plArrayMap<plString, plInt32> sa;

    PLASMA_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);

    PLASMA_TEST_INT(sa.GetCount(), 0);
    PLASMA_TEST_BOOL(sa.IsEmpty());

    sa.Reserve(10);

    PLASMA_TEST_BOOL(sa.GetHeapMemoryUsage() >= 10 * (sizeof(plString) + sizeof(plInt32)));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);
    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    PLASMA_TEST_INT(sa.GetCount(), 6);
    PLASMA_TEST_BOOL(!sa.IsEmpty());

    PLASMA_TEST_INT(sa.Find("a"), 0);
    PLASMA_TEST_INT(sa.Find("b"), 1);
    PLASMA_TEST_INT(sa.Find("c"), 2);
    PLASMA_TEST_INT(sa.Find("x"), 3);
    PLASMA_TEST_INT(sa.Find("y"), 4);
    PLASMA_TEST_INT(sa.Find("z"), 5);

    PLASMA_TEST_INT(sa.GetPair(sa.Find("a")).value, 5);
    PLASMA_TEST_INT(sa.GetPair(sa.Find("b")).value, 4);
    PLASMA_TEST_INT(sa.GetPair(sa.Find("c")).value, 3);
    PLASMA_TEST_INT(sa.GetPair(sa.Find("x")).value, 2);
    PLASMA_TEST_INT(sa.GetPair(sa.Find("y")).value, 1);
    PLASMA_TEST_INT(sa.GetPair(sa.Find("z")).value, 0);

    sa.Clear();
    PLASMA_TEST_BOOL(sa.IsEmpty());

    PLASMA_TEST_BOOL(sa.GetHeapMemoryUsage() > 0);
    sa.Compact();
    PLASMA_TEST_BOOL(sa.GetHeapMemoryUsage() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert / Find / = / == / != ")
  {
    plArrayMap<plInt32, plInt32> sa, sa2;

    sa.Insert(20, 0);
    sa.Insert(19, 1);
    sa.Insert(18, 2);
    sa.Insert(12, 3);
    sa.Insert(11, 4);

    sa2 = sa;

    PLASMA_TEST_BOOL(sa == sa2);

    sa.Insert(10, 5);

    PLASMA_TEST_BOOL(sa != sa2);

    PLASMA_TEST_INT(sa.Find(10), 0);
    PLASMA_TEST_INT(sa.Find(11), 1);
    PLASMA_TEST_INT(sa.Find(12), 2);
    PLASMA_TEST_INT(sa.Find(18), 3);
    PLASMA_TEST_INT(sa.Find(19), 4);
    PLASMA_TEST_INT(sa.Find(20), 5);

    sa2.Insert(10, 5);

    PLASMA_TEST_BOOL(sa == sa2);

    PLASMA_TEST_INT(sa.GetValue(sa.Find(10)), 5);
    PLASMA_TEST_INT(sa.GetValue(sa.Find(11)), 4);
    PLASMA_TEST_INT(sa.GetValue(sa.Find(12)), 3);
    PLASMA_TEST_INT(sa.GetValue(sa.Find(18)), 2);
    PLASMA_TEST_INT(sa.GetValue(sa.Find(19)), 1);
    PLASMA_TEST_INT(sa.GetValue(sa.Find(20)), 0);

    PLASMA_TEST_BOOL(sa == sa2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains")
  {
    plArrayMap<plString, plInt32> sa;

    PLASMA_TEST_BOOL(!sa.Contains("a"));
    PLASMA_TEST_BOOL(!sa.Contains("z"));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    PLASMA_TEST_BOOL(!sa.Contains("a"));
    PLASMA_TEST_BOOL(sa.Contains("z"));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    PLASMA_TEST_BOOL(sa.Contains("a"));
    PLASMA_TEST_BOOL(sa.Contains("z"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Contains")
  {
    plArrayMap<plString, plInt32> sa;

    PLASMA_TEST_BOOL(!sa.Contains("a", 0));
    PLASMA_TEST_BOOL(!sa.Contains("z", 0));

    sa.Insert("z", 0);
    sa.Insert("y", 1);
    sa.Insert("x", 2);

    PLASMA_TEST_BOOL(!sa.Contains("a", 0));
    PLASMA_TEST_BOOL(sa.Contains("z", 0));
    PLASMA_TEST_BOOL(sa.Contains("y", 1));
    PLASMA_TEST_BOOL(sa.Contains("x", 2));

    sa.Insert("c", 3);
    sa.Insert("b", 4);
    sa.Insert("a", 5);

    PLASMA_TEST_BOOL(sa.Contains("a", 5));
    PLASMA_TEST_BOOL(sa.Contains("b", 4));
    PLASMA_TEST_BOOL(sa.Contains("c", 3));
    PLASMA_TEST_BOOL(sa.Contains("z", 0));
    PLASMA_TEST_BOOL(sa.Contains("y", 1));
    PLASMA_TEST_BOOL(sa.Contains("x", 2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValue / GetKey / Copy Constructor")
  {
    plArrayMap<plString, plInt32> sa;

    sa.Insert("z", 1);
    sa.Insert("y", 3);
    sa.Insert("x", 5);
    sa.Insert("c", 7);
    sa.Insert("b", 9);
    sa.Insert("a", 11);

    sa.Sort();

    const plArrayMap<plString, plInt32> sa2(sa);

    PLASMA_TEST_INT(sa.GetValue(0), 11);
    PLASMA_TEST_INT(sa.GetValue(2), 7);

    PLASMA_TEST_INT(sa2.GetValue(0), 11);
    PLASMA_TEST_INT(sa2.GetValue(2), 7);

    PLASMA_TEST_STRING(sa.GetKey(1), "b");
    PLASMA_TEST_STRING(sa.GetKey(3), "x");

    PLASMA_TEST_INT(sa["b"], 9);
    PLASMA_TEST_INT(sa["y"], 3);

    PLASMA_TEST_INT(sa.GetPair(2).value, 7);
    PLASMA_TEST_STRING(sa.GetPair(4).key, "y");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove")
  {
    plArrayMap<plString, plInt32> sa;

    bool bExisted = true;

    sa.FindOrAdd("a", &bExisted) = 2;
    PLASMA_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 4;
    PLASMA_TEST_BOOL(!bExisted);

    sa.FindOrAdd("c", &bExisted) = 6;
    PLASMA_TEST_BOOL(!bExisted);

    sa.FindOrAdd("b", &bExisted) = 5;
    PLASMA_TEST_BOOL(bExisted);

    PLASMA_TEST_INT(sa.GetCount(), 3);

    PLASMA_TEST_INT(sa.Find("a"), 0);
    PLASMA_TEST_INT(sa.Find("c"), 2);

    sa.RemoveAndCopy("b");
    PLASMA_TEST_INT(sa.GetCount(), 2);

    PLASMA_TEST_INT(sa.Find("b"), plInvalidIndex);

    PLASMA_TEST_INT(sa.Find("a"), 0);
    PLASMA_TEST_INT(sa.Find("c"), 1);

    sa.RemoveAtAndCopy(1);
    PLASMA_TEST_INT(sa.GetCount(), 1);

    PLASMA_TEST_INT(sa.Find("a"), 0);
    PLASMA_TEST_INT(sa.Find("c"), plInvalidIndex);

    sa.RemoveAtAndCopy(0);
    PLASMA_TEST_INT(sa.GetCount(), 0);

    PLASMA_TEST_INT(sa.Find("a"), plInvalidIndex);
    PLASMA_TEST_INT(sa.Find("c"), plInvalidIndex);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Stresstest")
  {
    // Interestingly the map is not really slower than the sorted array, at least not in debug builds

    plStopwatch s;
    plArrayMap<plInt32, plInt32> sa;
    plMap<plInt32, plInt32> map;

    const plInt32 uiElements = 100000;

    // const plTime t0 = s.Checkpoint();

    {
      sa.Reserve(uiElements);

      for (plUInt32 i = 0; i < uiElements; ++i)
      {
        sa.Insert(uiElements - i, i * 2);
      }

      sa.Sort();
    }

    // const plTime t1 = s.Checkpoint();

    {
      for (plInt32 i = 0; i < uiElements; ++i)
      {
        PLASMA_TEST_INT(sa.GetValue(sa.Find(uiElements - i)), i * 2);
      }
    }

    // const plTime t2 = s.Checkpoint();

    {
      for (plUInt32 i = 0; i < uiElements; ++i)
      {
        map.Insert(uiElements - i, i * 2);
      }
    }

    // const plTime t3 = s.Checkpoint();

    {
      for (plUInt32 i = 0; i < uiElements; ++i)
      {
        PLASMA_TEST_INT(map[uiElements - i], i * 2);
      }
    }

    // const plTime t4 = s.Checkpoint();

    // int breakpoint = 0;
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Lower Bound / Upper Bound")
  {
    plArrayMap<plInt32, plInt32> sa;
    sa[1] = 23;
    sa[3] = 23;
    sa[4] = 23;
    sa[6] = 23;
    sa[7] = 23;
    sa[9] = 23;
    sa[11] = 23;
    sa[14] = 23;
    sa[17] = 23;

    PLASMA_TEST_INT(sa.LowerBound(0), 0);
    PLASMA_TEST_INT(sa.LowerBound(1), 0);
    PLASMA_TEST_INT(sa.LowerBound(2), 1);
    PLASMA_TEST_INT(sa.LowerBound(3), 1);
    PLASMA_TEST_INT(sa.LowerBound(4), 2);
    PLASMA_TEST_INT(sa.LowerBound(5), 3);
    PLASMA_TEST_INT(sa.LowerBound(6), 3);
    PLASMA_TEST_INT(sa.LowerBound(7), 4);
    PLASMA_TEST_INT(sa.LowerBound(8), 5);
    PLASMA_TEST_INT(sa.LowerBound(9), 5);
    PLASMA_TEST_INT(sa.LowerBound(10), 6);
    PLASMA_TEST_INT(sa.LowerBound(11), 6);
    PLASMA_TEST_INT(sa.LowerBound(12), 7);
    PLASMA_TEST_INT(sa.LowerBound(13), 7);
    PLASMA_TEST_INT(sa.LowerBound(14), 7);
    PLASMA_TEST_INT(sa.LowerBound(15), 8);
    PLASMA_TEST_INT(sa.LowerBound(16), 8);
    PLASMA_TEST_INT(sa.LowerBound(17), 8);
    PLASMA_TEST_INT(sa.LowerBound(18), plInvalidIndex);
    PLASMA_TEST_INT(sa.LowerBound(19), plInvalidIndex);
    PLASMA_TEST_INT(sa.LowerBound(20), plInvalidIndex);

    PLASMA_TEST_INT(sa.UpperBound(0), 0);
    PLASMA_TEST_INT(sa.UpperBound(1), 1);
    PLASMA_TEST_INT(sa.UpperBound(2), 1);
    PLASMA_TEST_INT(sa.UpperBound(3), 2);
    PLASMA_TEST_INT(sa.UpperBound(4), 3);
    PLASMA_TEST_INT(sa.UpperBound(5), 3);
    PLASMA_TEST_INT(sa.UpperBound(6), 4);
    PLASMA_TEST_INT(sa.UpperBound(7), 5);
    PLASMA_TEST_INT(sa.UpperBound(8), 5);
    PLASMA_TEST_INT(sa.UpperBound(9), 6);
    PLASMA_TEST_INT(sa.UpperBound(10), 6);
    PLASMA_TEST_INT(sa.UpperBound(11), 7);
    PLASMA_TEST_INT(sa.UpperBound(12), 7);
    PLASMA_TEST_INT(sa.UpperBound(13), 7);
    PLASMA_TEST_INT(sa.UpperBound(14), 8);
    PLASMA_TEST_INT(sa.UpperBound(15), 8);
    PLASMA_TEST_INT(sa.UpperBound(16), 8);
    PLASMA_TEST_INT(sa.UpperBound(17), plInvalidIndex);
    PLASMA_TEST_INT(sa.UpperBound(18), plInvalidIndex);
    PLASMA_TEST_INT(sa.UpperBound(19), plInvalidIndex);
    PLASMA_TEST_INT(sa.UpperBound(20), plInvalidIndex);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Duplicate Keys")
  {
    plArrayMap<plInt32, plInt32> sa;

    sa.Insert(32, 1);
    sa.Insert(31, 1);
    sa.Insert(33, 1);

    sa.Insert(40, 1);
    sa.Insert(44, 1);
    sa.Insert(46, 1);

    sa.Insert(11, 1);
    sa.Insert(15, 1);
    sa.Insert(19, 1);

    sa.Insert(11, 2);
    sa.Insert(15, 2);
    sa.Insert(31, 2);
    sa.Insert(44, 2);

    sa.Insert(11, 3);
    sa.Insert(15, 3);
    sa.Insert(44, 3);

    sa.Insert(60, 1);
    sa.Insert(60, 2);
    sa.Insert(60, 3);
    sa.Insert(60, 4);
    sa.Insert(60, 5);
    sa.Insert(60, 6);
    sa.Insert(60, 7);
    sa.Insert(60, 8);
    sa.Insert(60, 9);
    sa.Insert(60, 10);

    sa.Sort();

    PLASMA_TEST_INT(sa.LowerBound(11), 0);
    PLASMA_TEST_INT(sa.LowerBound(15), 3);
    PLASMA_TEST_INT(sa.LowerBound(19), 6);

    PLASMA_TEST_INT(sa.LowerBound(31), 7);
    PLASMA_TEST_INT(sa.LowerBound(32), 9);
    PLASMA_TEST_INT(sa.LowerBound(33), 10);

    PLASMA_TEST_INT(sa.LowerBound(40), 11);
    PLASMA_TEST_INT(sa.LowerBound(44), 12);
    PLASMA_TEST_INT(sa.LowerBound(46), 15);

    PLASMA_TEST_INT(sa.LowerBound(60), 16);


    PLASMA_TEST_INT(sa.UpperBound(11), 3);
    PLASMA_TEST_INT(sa.UpperBound(15), 6);
    PLASMA_TEST_INT(sa.UpperBound(19), 7);

    PLASMA_TEST_INT(sa.UpperBound(31), 9);
    PLASMA_TEST_INT(sa.UpperBound(32), 10);
    PLASMA_TEST_INT(sa.UpperBound(33), 11);

    PLASMA_TEST_INT(sa.UpperBound(40), 12);
    PLASMA_TEST_INT(sa.UpperBound(44), 15);
    PLASMA_TEST_INT(sa.UpperBound(46), 16);

    PLASMA_TEST_INT(sa.UpperBound(60), plInvalidIndex);
  }
}
