#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>

namespace
{
  using st = plConstructionCounter;

  struct Collision
  {
    plUInt32 hash;
    int key;

    inline Collision(plUInt32 uiHash, int iKey)
    {
      this->hash = uiHash;
      this->key = iKey;
    }

    inline bool operator==(const Collision& other) const { return key == other.key; }

    PLASMA_DECLARE_POD_TYPE();
  };

  class OnlyMovable
  {
  public:
    OnlyMovable(plUInt32 uiHash)
      : hash(uiHash)

    {
    }
    OnlyMovable(OnlyMovable&& other) { *this = std::move(other); }

    void operator=(OnlyMovable&& other)
    {
      hash = other.hash;
      m_NumTimesMoved = 0;
      ++other.m_NumTimesMoved;
    }

    bool operator==(const OnlyMovable& other) const { return hash == other.hash; }

    int m_NumTimesMoved = 0;
    plUInt32 hash;

  private:
    OnlyMovable(const OnlyMovable&);
    void operator=(const OnlyMovable&);
  };
} // namespace

template <>
struct plHashHelper<Collision>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const Collision& value) { return value.hash; }

  PLASMA_ALWAYS_INLINE static bool Equal(const Collision& a, const Collision& b) { return a == b; }
};

template <>
struct plHashHelper<OnlyMovable>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const OnlyMovable& value) { return value.hash; }

  PLASMA_ALWAYS_INLINE static bool Equal(const OnlyMovable& a, const OnlyMovable& b) { return a.hash == b.hash; }
};

PLASMA_CREATE_SIMPLE_TEST(Containers, HashSet)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plHashSet<plInt32> table1;

    PLASMA_TEST_BOOL(table1.GetCount() == 0);
    PLASMA_TEST_BOOL(table1.IsEmpty());

    plUInt32 counter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    PLASMA_TEST_INT(counter, 0);

    PLASMA_TEST_BOOL(begin(table1) == end(table1));
    PLASMA_TEST_BOOL(cbegin(table1) == cend(table1));
    table1.Reserve(10);
    PLASMA_TEST_BOOL(begin(table1) == end(table1));
    PLASMA_TEST_BOOL(cbegin(table1) == cend(table1));

    for (auto value : table1)
    {
      ++counter;
    }
    PLASMA_TEST_INT(counter, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    plHashSet<plInt32> table1;

    for (plInt32 i = 0; i < 64; ++i)
    {
      plInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key);
    }

    // insert an element at the very end
    table1.Insert(47);

    plHashSet<plInt32> table2;
    table2 = table1;
    plHashSet<plInt32> table3(table1);

    PLASMA_TEST_INT(table1.GetCount(), 65);
    PLASMA_TEST_INT(table2.GetCount(), 65);
    PLASMA_TEST_INT(table3.GetCount(), 65);
    PLASMA_TEST_BOOL(begin(table1) != end(table1));
    PLASMA_TEST_BOOL(cbegin(table1) != cend(table1));

    plUInt32 uiCounter = 0;
    for (auto it = table1.GetIterator(); it.IsValid(); ++it)
    {
      plConstructionCounter value;
      PLASMA_TEST_BOOL(table2.Contains(it.Key()));
      PLASMA_TEST_BOOL(table3.Contains(it.Key()));
      ++uiCounter;
    }
    PLASMA_TEST_INT(uiCounter, table1.GetCount());

    uiCounter = 0;
    for (const auto& value : table1)
    {
      PLASMA_TEST_BOOL(table2.Contains(value));
      PLASMA_TEST_BOOL(table3.Contains(value));
      ++uiCounter;
    }
    PLASMA_TEST_INT(uiCounter, table1.GetCount());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    plHashSet<st> set1;
    for (plInt32 i = 0; i < 64; ++i)
    {
      set1.Insert(plConstructionCounter(i));
    }

    plUInt64 memoryUsage = set1.GetHeapMemoryUsage();

    plHashSet<st> set2;
    set2 = std::move(set1);

    PLASMA_TEST_INT(set1.GetCount(), 0);
    PLASMA_TEST_INT(set1.GetHeapMemoryUsage(), 0);
    PLASMA_TEST_INT(set2.GetCount(), 64);
    PLASMA_TEST_INT(set2.GetHeapMemoryUsage(), memoryUsage);

    plHashSet<st> set3(std::move(set2));

    PLASMA_TEST_INT(set2.GetCount(), 0);
    PLASMA_TEST_INT(set2.GetHeapMemoryUsage(), 0);
    PLASMA_TEST_INT(set3.GetCount(), 64);
    PLASMA_TEST_INT(set3.GetHeapMemoryUsage(), memoryUsage);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Collision Tests")
  {
    plHashSet<Collision> set2;

    set2.Insert(Collision(0, 0));
    set2.Insert(Collision(1, 1));
    set2.Insert(Collision(0, 2));
    set2.Insert(Collision(1, 3));
    set2.Insert(Collision(1, 4));
    set2.Insert(Collision(0, 5));

    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 0)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 1)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 2)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 3)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 4)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 5)));

    PLASMA_TEST_BOOL(set2.Remove(Collision(0, 0)));
    PLASMA_TEST_BOOL(set2.Remove(Collision(1, 1)));

    PLASMA_TEST_BOOL(!set2.Contains(Collision(0, 0)));
    PLASMA_TEST_BOOL(!set2.Contains(Collision(1, 1)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 2)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 3)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 4)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 5)));

    set2.Insert(Collision(0, 6));
    set2.Insert(Collision(1, 7));

    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 2)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 3)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 4)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 5)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 6)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 7)));

    PLASMA_TEST_BOOL(set2.Remove(Collision(1, 4)));
    PLASMA_TEST_BOOL(set2.Remove(Collision(0, 6)));

    PLASMA_TEST_BOOL(!set2.Contains(Collision(1, 4)));
    PLASMA_TEST_BOOL(!set2.Contains(Collision(0, 6)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 2)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 3)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(0, 5)));
    PLASMA_TEST_BOOL(set2.Contains(Collision(1, 7)));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    PLASMA_TEST_BOOL(st::HasAllDestructed());

    {
      plHashSet<st> m1;
      m1.Insert(st(1));
      PLASMA_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1.Insert(st(3));
      PLASMA_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1.Insert(st(1));
      PLASMA_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(st::HasDone(0, 2));
      PLASMA_TEST_BOOL(st::HasAllDestructed());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert")
  {
    plHashSet<plInt32> a1;

    for (plInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(!a1.Insert(i));
    }

    for (plInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(a1.Insert(i));
    }
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Insert")
  {
    OnlyMovable noCopyObject(42);

    plHashSet<OnlyMovable> noCopyKey;
    // noCopyKey.Insert(noCopyObject); // Should not compile
    noCopyKey.Insert(std::move(noCopyObject));
    PLASMA_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
    PLASMA_TEST_BOOL(noCopyKey.Contains(noCopyObject));
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove/Compact")
  {
    plHashSet<plInt32> a;

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      PLASMA_TEST_INT(a.GetCount(), i + 1);
    }

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(plInt32)));

    a.Compact();

    for (plInt32 i = 0; i < 500; ++i)
    {
      PLASMA_TEST_BOOL(a.Remove(i));
    }

    a.Compact();

    for (plInt32 i = 500; i < 1000; ++i)
    {
      PLASMA_TEST_BOOL(a.Contains(i));
    }

    a.Clear();
    a.Compact();

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove (Iterator)")
  {
    plHashSet<plInt32> a;

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
    for (plInt32 i = 0; i < 1000; ++i)
      a.Insert(i);

    plHashSet<plInt32>::ConstIterator it = a.GetIterator();

    for (plInt32 i = 0; i < 1000 - 1; ++i)
    {
      plInt32 value = it.Key();
      it = a.Remove(it);
      PLASMA_TEST_BOOL(!a.Contains(value));
      PLASMA_TEST_BOOL(it.IsValid());
      PLASMA_TEST_INT(a.GetCount(), 1000 - 1 - i);
    }
    it = a.Remove(it);
    PLASMA_TEST_BOOL(!it.IsValid());
    PLASMA_TEST_BOOL(a.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Set Operations")
  {
    plHashSet<plUInt32> base;
    base.Insert(1);
    base.Insert(3);
    base.Insert(5);

    plHashSet<plUInt32> empty;

    plHashSet<plUInt32> disjunct;
    disjunct.Insert(2);
    disjunct.Insert(4);
    disjunct.Insert(6);

    plHashSet<plUInt32> subSet;
    subSet.Insert(1);
    subSet.Insert(5);

    plHashSet<plUInt32> superSet;
    superSet.Insert(1);
    superSet.Insert(3);
    superSet.Insert(5);
    superSet.Insert(7);

    plHashSet<plUInt32> nonDisjunctNonEmptySubSet;
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
      plHashSet<plUInt32> res;

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
      plHashSet<plUInt32> res;
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
      plHashSet<plUInt32> res;
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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator==/!=")
  {
    plStaticArray<plInt32, 64> keys[2];

    for (plUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    plHashSet<plInt32> t[2];

    for (plUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const plUInt32 uiIndex = rand() % keys[i].GetCount();
        const plInt32 key = keys[i][uiIndex];
        t[i].Insert(key);

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    PLASMA_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32);
    PLASMA_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32);
    PLASMA_TEST_BOOL(t[0] == t[1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompatibleKeyType")
  {
    plProxyAllocator testAllocator("Test", plFoundation::GetDefaultAllocator());
    plLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = plHybridString<32, plLocalAllocatorWrapper>;

    plHashSet<TestString> stringSet;
    const char* szChar = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char* szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    plStringView sView(szString);
    plStringBuilder sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    plString sString("String");
    PLASMA_TEST_BOOL(!stringSet.Insert(szChar));
    PLASMA_TEST_BOOL(!stringSet.Insert(sView));
    PLASMA_TEST_BOOL(!stringSet.Insert(sBuilder));
    PLASMA_TEST_BOOL(!stringSet.Insert(sString));
    PLASMA_TEST_BOOL(stringSet.Insert(szString));

    plUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    PLASMA_TEST_BOOL(stringSet.Contains(szChar));
    PLASMA_TEST_BOOL(stringSet.Contains(sView));
    PLASMA_TEST_BOOL(stringSet.Contains(sBuilder));
    PLASMA_TEST_BOOL(stringSet.Contains(sString));

    PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    PLASMA_TEST_BOOL(stringSet.Remove(szChar));
    PLASMA_TEST_BOOL(stringSet.Remove(sView));
    PLASMA_TEST_BOOL(stringSet.Remove(sBuilder));
    PLASMA_TEST_BOOL(stringSet.Remove(sString));

    PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plStringBuilder tmp;
    plHashSet<plString> set1;
    plHashSet<plString> set2;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      set1.Insert(tmp);

      tmp.Format("{0}{0}{0}", i);
      set2.Insert(tmp);
    }

    set1.Swap(set2);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      PLASMA_TEST_BOOL(set2.Contains(tmp));

      tmp.Format("{0}{0}{0}", i);
      PLASMA_TEST_BOOL(set1.Contains(tmp));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "foreach")
  {
    plStringBuilder tmp;
    plHashSet<plString> set;
    plHashSet<plString> set2;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      set.Insert(tmp);
    }

    PLASMA_TEST_INT(set.GetCount(), 1000);

    set2 = set;
    PLASMA_TEST_INT(set2.GetCount(), set.GetCount());

    for (plHashSet<plString>::ConstIterator it = begin(set); it != end(set); ++it)
    {
      const plString& k = it.Key();
      set2.Remove(k);
    }

    PLASMA_TEST_BOOL(set2.IsEmpty());
    set2 = set;

    for (auto key : set)
    {
      set2.Remove(key);
    }

    PLASMA_TEST_BOOL(set2.IsEmpty());
  }
}
