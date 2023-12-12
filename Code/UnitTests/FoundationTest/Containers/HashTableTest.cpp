#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/String.h>

namespace HashTableTestDetail
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
} // namespace HashTableTestDetail

template <>
struct plHashHelper<HashTableTestDetail::Collision>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const HashTableTestDetail::Collision& value) { return value.hash; }

  PLASMA_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::Collision& a, const HashTableTestDetail::Collision& b) { return a == b; }
};

template <>
struct plHashHelper<HashTableTestDetail::OnlyMovable>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(const HashTableTestDetail::OnlyMovable& value) { return value.hash; }

  PLASMA_ALWAYS_INLINE static bool Equal(const HashTableTestDetail::OnlyMovable& a, const HashTableTestDetail::OnlyMovable& b)
  {
    return a.hash == b.hash;
  }
};

PLASMA_CREATE_SIMPLE_TEST(Containers, HashTable)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plHashTable<plInt32, HashTableTestDetail::st> table1;

    PLASMA_TEST_BOOL(table1.GetCount() == 0);
    PLASMA_TEST_BOOL(table1.IsEmpty());

    plUInt32 counter = 0;
    for (plHashTable<plInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    PLASMA_TEST_INT(counter, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    plHashTable<plInt32, HashTableTestDetail::st> table1;

    for (plInt32 i = 0; i < 64; ++i)
    {
      plInt32 key;

      do
      {
        key = rand() % 100000;
      } while (table1.Contains(key));

      table1.Insert(key, plConstructionCounter(i));
    }

    // insert an element at the very end
    table1.Insert(47, plConstructionCounter(64));

    plHashTable<plInt32, HashTableTestDetail::st> table2;
    table2 = table1;
    plHashTable<plInt32, HashTableTestDetail::st> table3(table1);

    PLASMA_TEST_INT(table1.GetCount(), 65);
    PLASMA_TEST_INT(table2.GetCount(), 65);
    PLASMA_TEST_INT(table3.GetCount(), 65);

    plUInt32 uiCounter = 0;
    for (plHashTable<plInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      plConstructionCounter value;

      PLASMA_TEST_BOOL(table2.TryGetValue(it.Key(), value));
      PLASMA_TEST_BOOL(it.Value() == value);
      PLASMA_TEST_BOOL(*table2.GetValue(it.Key()) == it.Value());

      PLASMA_TEST_BOOL(table3.TryGetValue(it.Key(), value));
      PLASMA_TEST_BOOL(it.Value() == value);
      PLASMA_TEST_BOOL(*table3.GetValue(it.Key()) == it.Value());

      ++uiCounter;
    }
    PLASMA_TEST_INT(uiCounter, table1.GetCount());

    for (plHashTable<plInt32, HashTableTestDetail::st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      it.Value() = HashTableTestDetail::st(42);
    }

    for (plHashTable<plInt32, HashTableTestDetail::st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
    {
      plConstructionCounter value;

      PLASMA_TEST_BOOL(table1.TryGetValue(it.Key(), value));
      PLASMA_TEST_BOOL(it.Value() == value);
      PLASMA_TEST_BOOL(value.m_iData == 42);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Copy Constructor/Assignment")
  {
    plHashTable<plInt32, HashTableTestDetail::st> table1;
    for (plInt32 i = 0; i < 64; ++i)
    {
      table1.Insert(i, plConstructionCounter(i));
    }

    plUInt64 memoryUsage = table1.GetHeapMemoryUsage();

    plHashTable<plInt32, HashTableTestDetail::st> table2;
    table2 = std::move(table1);

    PLASMA_TEST_INT(table1.GetCount(), 0);
    PLASMA_TEST_INT(table1.GetHeapMemoryUsage(), 0);
    PLASMA_TEST_INT(table2.GetCount(), 64);
    PLASMA_TEST_INT(table2.GetHeapMemoryUsage(), memoryUsage);

    plHashTable<plInt32, HashTableTestDetail::st> table3(std::move(table2));

    PLASMA_TEST_INT(table2.GetCount(), 0);
    PLASMA_TEST_INT(table2.GetHeapMemoryUsage(), 0);
    PLASMA_TEST_INT(table3.GetCount(), 64);
    PLASMA_TEST_INT(table3.GetHeapMemoryUsage(), memoryUsage);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Move Insert")
  {
    HashTableTestDetail::OnlyMovable noCopyObject(42);

    {
      plHashTable<HashTableTestDetail::OnlyMovable, int> noCopyKey;
      // noCopyKey.Insert(noCopyObject, 10); // Should not compile
      noCopyKey.Insert(std::move(noCopyObject), 10);
      PLASMA_TEST_INT(noCopyObject.m_NumTimesMoved, 1);
      PLASMA_TEST_BOOL(noCopyKey.Contains(noCopyObject));
    }

    {
      plHashTable<int, HashTableTestDetail::OnlyMovable> noCopyValue;
      // noCopyValue.Insert(10, noCopyObject); // Should not compile
      noCopyValue.Insert(10, std::move(noCopyObject));
      PLASMA_TEST_INT(noCopyObject.m_NumTimesMoved, 2);
      PLASMA_TEST_BOOL(noCopyValue.Contains(10));
    }

    {
      plHashTable<HashTableTestDetail::OnlyMovable, HashTableTestDetail::OnlyMovable> noCopyAnything;
      // noCopyAnything.Insert(10, noCopyObject); // Should not compile
      // noCopyAnything.Insert(noCopyObject, 10); // Should not compile
      noCopyAnything.Insert(std::move(noCopyObject), std::move(noCopyObject));
      PLASMA_TEST_INT(noCopyObject.m_NumTimesMoved, 4);
      PLASMA_TEST_BOOL(noCopyAnything.Contains(noCopyObject));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Collision Tests")
  {
    plHashTable<HashTableTestDetail::Collision, int> map2;

    map2[HashTableTestDetail::Collision(0, 0)] = 0;
    map2[HashTableTestDetail::Collision(1, 1)] = 1;
    map2[HashTableTestDetail::Collision(0, 2)] = 2;
    map2[HashTableTestDetail::Collision(1, 3)] = 3;
    map2[HashTableTestDetail::Collision(1, 4)] = 4;
    map2[HashTableTestDetail::Collision(0, 5)] = 5;

    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 0)] == 0);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 1)] == 1);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 0)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 1)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    PLASMA_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 0)));
    PLASMA_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 1)));

    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);

    PLASMA_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 0)));
    PLASMA_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 1)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));

    map2[HashTableTestDetail::Collision(0, 6)] = 6;
    map2[HashTableTestDetail::Collision(1, 7)] = 7;

    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 4)] == 4);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 6)] == 6);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 4)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 6)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    PLASMA_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(1, 4)));
    PLASMA_TEST_BOOL(map2.Remove(HashTableTestDetail::Collision(0, 6)));

    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 2);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 3);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 5);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 7)] == 7);

    PLASMA_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(1, 4)));
    PLASMA_TEST_BOOL(!map2.Contains(HashTableTestDetail::Collision(0, 6)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 2)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 3)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(0, 5)));
    PLASMA_TEST_BOOL(map2.Contains(HashTableTestDetail::Collision(1, 7)));

    map2[HashTableTestDetail::Collision(0, 2)] = 3;
    map2[HashTableTestDetail::Collision(0, 5)] = 6;
    map2[HashTableTestDetail::Collision(1, 3)] = 4;

    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 2)] == 3);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(0, 5)] == 6);
    PLASMA_TEST_BOOL(map2[HashTableTestDetail::Collision(1, 3)] == 4);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    PLASMA_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());

    {
      plHashTable<plUInt32, HashTableTestDetail::st> m1;
      m1[0] = HashTableTestDetail::st(1);
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

      m1[1] = HashTableTestDetail::st(3);
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // for inserting new elements 2 temporary is created (and destroyed)

      m1[0] = HashTableTestDetail::st(2);
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }

    {
      plHashTable<HashTableTestDetail::st, plUInt32> m1;
      m1[HashTableTestDetail::st(0)] = 1;
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(1)] = 3;
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(2, 1)); // one temporary

      m1[HashTableTestDetail::st(0)] = 2;
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

      m1.Clear();
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasDone(0, 2));
      PLASMA_TEST_BOOL(HashTableTestDetail::st::HasAllDestructed());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert/TryGetValue/GetValue")
  {
    plHashTable<plInt32, HashTableTestDetail::st> a1;

    for (plInt32 i = 0; i < 10; ++i)
    {
      PLASMA_TEST_BOOL(!a1.Insert(i, i - 20));
    }

    for (plInt32 i = 0; i < 10; ++i)
    {
      HashTableTestDetail::st oldValue;
      PLASMA_TEST_BOOL(a1.Insert(i, i, &oldValue));
      PLASMA_TEST_INT(oldValue.m_iData, i - 20);
    }

    HashTableTestDetail::st value;
    PLASMA_TEST_BOOL(a1.TryGetValue(9, value));
    PLASMA_TEST_INT(value.m_iData, 9);
    PLASMA_TEST_INT(a1.GetValue(9)->m_iData, 9);

    PLASMA_TEST_BOOL(!a1.TryGetValue(11, value));
    PLASMA_TEST_INT(value.m_iData, 9);
    PLASMA_TEST_BOOL(a1.GetValue(11) == nullptr);

    HashTableTestDetail::st* pValue;
    PLASMA_TEST_BOOL(a1.TryGetValue(9, pValue));
    PLASMA_TEST_INT(pValue->m_iData, 9);

    pValue->m_iData = 20;
    PLASMA_TEST_INT(a1[9].m_iData, 20);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove/Compact")
  {
    plHashTable<plInt32, HashTableTestDetail::st> a;

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);

    for (plInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i, i);
      PLASMA_TEST_INT(a.GetCount(), i + 1);
    }

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() >= 1000 * (sizeof(plInt32) + sizeof(HashTableTestDetail::st)));

    a.Compact();

    for (plInt32 i = 0; i < 1000; ++i)
      PLASMA_TEST_INT(a[i].m_iData, i);


    for (plInt32 i = 0; i < 250; ++i)
    {
      HashTableTestDetail::st oldValue;
      PLASMA_TEST_BOOL(a.Remove(i, &oldValue));
      PLASMA_TEST_INT(oldValue.m_iData, i);
    }
    PLASMA_TEST_INT(a.GetCount(), 750);

    for (plHashTable<plInt32, HashTableTestDetail::st>::Iterator it = a.GetIterator(); it.IsValid();)
    {
      if (it.Key() < 500)
        it = a.Remove(it);
      else
        ++it;
    }
    PLASMA_TEST_INT(a.GetCount(), 500);
    a.Compact();

    for (plInt32 i = 500; i < 1000; ++i)
      PLASMA_TEST_INT(a[i].m_iData, i);

    a.Clear();
    a.Compact();

    PLASMA_TEST_BOOL(a.GetHeapMemoryUsage() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator[]")
  {
    plHashTable<plInt32, plInt32> a;

    a.Insert(4, 20);
    a[2] = 30;

    PLASMA_TEST_INT(a[4], 20);
    PLASMA_TEST_INT(a[2], 30);
    PLASMA_TEST_INT(a[1], 0); // new values are default constructed
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator==/!=")
  {
    plStaticArray<plInt32, 64> keys[2];

    for (plUInt32 i = 0; i < 64; ++i)
    {
      keys[0].PushBack(rand());
    }

    keys[1] = keys[0];

    plHashTable<plInt32, HashTableTestDetail::st> t[2];

    for (plUInt32 i = 0; i < 2; ++i)
    {
      while (!keys[i].IsEmpty())
      {
        const plUInt32 uiIndex = rand() % keys[i].GetCount();
        const plInt32 key = keys[i][uiIndex];
        t[i].Insert(key, HashTableTestDetail::st(key * 3456));

        keys[i].RemoveAtAndSwap(uiIndex);
      }
    }

    PLASMA_TEST_BOOL(t[0] == t[1]);

    t[0].Insert(32, HashTableTestDetail::st(64));
    PLASMA_TEST_BOOL(t[0] != t[1]);

    t[1].Insert(32, HashTableTestDetail::st(47));
    PLASMA_TEST_BOOL(t[0] != t[1]);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "CompatibleKeyType")
  {
    plProxyAllocator testAllocator("Test", plFoundation::GetDefaultAllocator());
    plLocalAllocatorWrapper allocWrapper(&testAllocator);
    using TestString = plHybridString<32, plLocalAllocatorWrapper>;

    plHashTable<TestString, int> stringTable;
    const char* szChar = "VeryLongStringDefinitelyMoreThan32Chars1111elf!!!!";
    const char* szString = "AnotherVeryLongStringThisTimeUsedForStringView!!!!";
    plStringView sView(szString);
    plStringBuilder sBuilder("BuilderAlsoNeedsToBeAVeryLongStringToTriggerAllocation");
    plString sString("String");
    PLASMA_TEST_BOOL(!stringTable.Insert(szChar, 1));
    PLASMA_TEST_BOOL(!stringTable.Insert(sView, 2));
    PLASMA_TEST_BOOL(!stringTable.Insert(sBuilder, 3));
    PLASMA_TEST_BOOL(!stringTable.Insert(sString, 4));
    PLASMA_TEST_BOOL(stringTable.Insert(szString, 2));

    plUInt64 oldAllocCount = testAllocator.GetStats().m_uiNumAllocations;

    PLASMA_TEST_BOOL(stringTable.Contains(szChar));
    PLASMA_TEST_BOOL(stringTable.Contains(sView));
    PLASMA_TEST_BOOL(stringTable.Contains(sBuilder));
    PLASMA_TEST_BOOL(stringTable.Contains(sString));

    PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    PLASMA_TEST_INT(*stringTable.GetValue(szChar), 1);
    PLASMA_TEST_INT(*stringTable.GetValue(sView), 2);
    PLASMA_TEST_INT(*stringTable.GetValue(sBuilder), 3);
    PLASMA_TEST_INT(*stringTable.GetValue(sString), 4);

    PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);

    PLASMA_TEST_BOOL(stringTable.Remove(szChar));
    PLASMA_TEST_BOOL(stringTable.Remove(sView));
    PLASMA_TEST_BOOL(stringTable.Remove(sBuilder));
    PLASMA_TEST_BOOL(stringTable.Remove(sString));

    PLASMA_TEST_INT(testAllocator.GetStats().m_uiNumAllocations, oldAllocCount);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Swap")
  {
    plStringBuilder tmp;
    plHashTable<plString, plInt32> map1;
    plHashTable<plString, plInt32> map2;

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

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "foreach")
  {
    plStringBuilder tmp;
    plHashTable<plString, plInt32> map;
    plHashTable<plString, plInt32> map2;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map[tmp] = i;
    }

    PLASMA_TEST_INT(map.GetCount(), 1000);

    map2 = map;
    PLASMA_TEST_INT(map2.GetCount(), map.GetCount());

    for (plHashTable<plString, plInt32>::Iterator it = begin(map); it != end(map); ++it)
    {
      const plString& k = it.Key();
      plInt32 v = it.Value();

      map2.Remove(k);
    }

    PLASMA_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    for (auto it : map)
    {
      const plString& k = it.Key();
      plInt32 v = it.Value();

      map2.Remove(k);
    }

    PLASMA_TEST_BOOL(map2.IsEmpty());
    map2 = map;

    // just check that this compiles
    for (auto it : static_cast<const plHashTable<plString, plInt32>&>(map))
    {
      const plString& k = it.Key();
      plInt32 v = it.Value();

      map2.Remove(k);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Find")
  {
    plStringBuilder tmp;
    plHashTable<plString, plInt32> map;

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      tmp.Format("stuff{}bla", i);
      map[tmp] = i;
    }

    for (plInt32 i = map.GetCount() - 1; i > 0; --i)
    {
      tmp.Format("stuff{}bla", i);

      auto it = map.Find(tmp);
      auto cit = static_cast<const plHashTable<plString, plInt32>&>(map).Find(tmp);

      PLASMA_TEST_STRING(it.Key(), tmp);
      PLASMA_TEST_INT(it.Value(), i);

      PLASMA_TEST_STRING(cit.Key(), tmp);
      PLASMA_TEST_INT(cit.Value(), i);

      int allowedIterations = map.GetCount();
      for (auto it2 = it; it2.IsValid(); ++it2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        PLASMA_TEST_BOOL(allowedIterations >= 0);
      }

      allowedIterations = map.GetCount();
      for (auto cit2 = cit; cit2.IsValid(); ++cit2)
      {
        // just test that iteration is possible and terminates correctly
        --allowedIterations;
        PLASMA_TEST_BOOL(allowedIterations >= 0);
      }

      map.Remove(it);
    }
  }
}
