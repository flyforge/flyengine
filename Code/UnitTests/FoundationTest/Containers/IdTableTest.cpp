#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Strings/String.h>

namespace
{
  using Id = plGenericId<32, 16>;
  using st = plConstructionCounter;

  struct TestObject
  {
    int x;
    plString s;
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Containers, IdTable)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Constructor")
  {
    plIdTable<Id, plInt32> table;

    PLASMA_TEST_BOOL(table.GetCount() == 0);
    PLASMA_TEST_BOOL(table.IsEmpty());

    plUInt32 counter = 0;
    for (plIdTable<Id, plInt32>::ConstIterator it = table.GetIterator(); it.IsValid(); ++it)
    {
      ++counter;
    }
    PLASMA_TEST_INT(counter, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy Constructor/Assignment/Iterator")
  {
    PLASMA_TEST_BOOL(st::HasAllDestructed());
    {
      plIdTable<Id, st> table1;

      for (plInt32 i = 0; i < 200; ++i)
      {
        table1.Insert(st(i));
      }

      PLASMA_TEST_BOOL(table1.Remove(Id(0, 1)));

      for (plInt32 i = 0; i < 99; ++i)
      {
        Id id;
        id.m_Generation = 1;

        do
        {
          id.m_InstanceIndex = rand() % 200;
        } while (!table1.Contains(id));

        PLASMA_TEST_BOOL(table1.Remove(id));
      }

      plIdTable<Id, st> table2;
      table2 = table1;
      plIdTable<Id, st> table3(table1);

      PLASMA_TEST_BOOL(table2.IsFreelistValid());
      PLASMA_TEST_BOOL(table3.IsFreelistValid());

      PLASMA_TEST_INT(table1.GetCount(), 100);
      PLASMA_TEST_INT(table2.GetCount(), 100);
      PLASMA_TEST_INT(table3.GetCount(), 100);

      plUInt32 uiCounter = 0;
      for (plIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        PLASMA_TEST_BOOL(table2.TryGetValue(it.Id(), value));
        PLASMA_TEST_BOOL(it.Value() == value);

        PLASMA_TEST_BOOL(table3.TryGetValue(it.Id(), value));
        PLASMA_TEST_BOOL(it.Value() == value);

        ++uiCounter;
      }
      PLASMA_TEST_INT(uiCounter, table1.GetCount());

      for (plIdTable<Id, st>::Iterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        it.Value() = st(42);
      }

      for (plIdTable<Id, st>::ConstIterator it = table1.GetIterator(); it.IsValid(); ++it)
      {
        st value;

        PLASMA_TEST_BOOL(table1.TryGetValue(it.Id(), value));
        PLASMA_TEST_BOOL(it.Value() == value);
        PLASMA_TEST_BOOL(value.m_iData == 42);
      }
    }
    PLASMA_TEST_BOOL(st::HasAllDestructed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Verify 0 is never valid")
  {
    plIdTable<Id, TestObject> table;

    plUInt32 count1 = 0, count2 = 0;

    TestObject x = {11, "Test"};

    while (true)
    {
      Id id = table.Insert(x);
      PLASMA_TEST_BOOL(id.m_Generation != 0);

      PLASMA_TEST_BOOL(table.Remove(id));

      if (id.m_Generation > 1) // until all elements in generation 1 have been used up
        break;

      ++count1;
    }

    PLASMA_TEST_BOOL(!table.Contains(Id(0, 0)));

    while (true)
    {
      Id id = table.Insert(x);
      PLASMA_TEST_BOOL(id.m_Generation != 0);

      PLASMA_TEST_BOOL(table.Remove(id));

      if (id.m_Generation == 1) // wrap around
        break;

      ++count2;
    }

    PLASMA_TEST_BOOL(!table.Contains(Id(0, 0)));

    PLASMA_TEST_INT(count1, 32);
    PLASMA_TEST_INT(count2, 2097087);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Insert/Remove")
  {
    plIdTable<Id, TestObject> table;

    for (int i = 0; i < 100; i++)
    {
      TestObject x = {rand(), "Test"};
      Id id = table.Insert(x);
      PLASMA_TEST_INT(id.m_InstanceIndex, i);
      PLASMA_TEST_INT(id.m_Generation, 1);

      PLASMA_TEST_BOOL(table.Contains(id));

      TestObject y = table[id];
      PLASMA_TEST_INT(x.x, y.x);
      PLASMA_TEST_BOOL(x.s == y.s);
    }
    PLASMA_TEST_INT(table.GetCount(), 100);

    Id ids[10] = {Id(13, 1), Id(0, 1), Id(16, 1), Id(34, 1), Id(56, 1), Id(57, 1), Id(79, 1), Id(85, 1), Id(91, 1), Id(97, 1)};


    for (int i = 0; i < 10; i++)
    {
      bool res = table.Remove(ids[i]);
      PLASMA_TEST_BOOL(res);
      PLASMA_TEST_BOOL(!table.Contains(ids[i]));
    }
    PLASMA_TEST_INT(table.GetCount(), 90);

    for (int i = 0; i < 40; i++)
    {
      TestObject x = {1000, "Bla. This is a very long string which does not fit into 32 byte and will cause memory allocations."};
      Id newId = table.Insert(x);

      PLASMA_TEST_BOOL(table.Contains(newId));

      TestObject y = table[newId];
      PLASMA_TEST_INT(x.x, y.x);
      PLASMA_TEST_BOOL(x.s == y.s);

      TestObject* pObj;
      PLASMA_TEST_BOOL(table.TryGetValue(newId, pObj));
      PLASMA_TEST_BOOL(pObj->s == x.s);
    }
    PLASMA_TEST_INT(table.GetCount(), 130);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Crash test")
  {
    plIdTable<Id, TestObject> table;
    plDynamicArray<Id> ids;

    for (plUInt32 i = 0; i < 100000; ++i)
    {
      int action = rand() % 2;
      if (action == 0)
      {
        TestObject x = {rand(), "Test"};
        ids.PushBack(table.Insert(x));
      }
      else
      {
        if (ids.GetCount() > 0)
        {
          plUInt32 index = rand() % ids.GetCount();
          PLASMA_TEST_BOOL(table.Remove(ids[index]));
          ids.RemoveAtAndSwap(index);
        }
      }

      PLASMA_TEST_BOOL(table.IsFreelistValid());
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Clear")
  {
    PLASMA_TEST_BOOL(st::HasAllDestructed());

    plIdTable<Id, st> m1;
    Id id0 = m1.Insert(st(1));
    PLASMA_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    Id id1 = m1.Insert(st(3));
    PLASMA_TEST_BOOL(st::HasDone(2, 1)); // for inserting new elements 1 temporary is created (and destroyed)

    m1[id0] = st(2);
    PLASMA_TEST_BOOL(st::HasDone(1, 1)); // nothing new to create, so only the one temporary is used

    m1.Clear();
    PLASMA_TEST_BOOL(st::HasDone(0, 2));
    PLASMA_TEST_BOOL(st::HasAllDestructed());

    PLASMA_TEST_BOOL(!m1.Contains(id0));
    PLASMA_TEST_BOOL(!m1.Contains(id1));
    PLASMA_TEST_BOOL(m1.IsFreelistValid());
  }

  /*PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Remove/Compact")
  {
    plIdTable<Id, st> a;

    for (plInt32 i = 0; i < 1000; ++i)
    {
      a.Insert(i);
      PLASMA_TEST_INT(a.GetCount(), i + 1);
    }

    a.Compact();
    PLASMA_TEST_BOOL(a.IsFreelistValid());

    {
      plUInt32 i = 0;
      for (plIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        PLASMA_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }

    for (plInt32 i = 500; i < 1000; ++i)
    {
      st oldValue;
      PLASMA_TEST_BOOL(a.Remove(Id(i, 0), &oldValue));
      PLASMA_TEST_INT(oldValue.m_iData, i);
    }

    a.Compact();
    PLASMA_TEST_BOOL(a.IsFreelistValid());

    {
      plUInt32 i = 0;
      for (plIdTable<Id, st>::Iterator it = a.GetIterator(); it.IsValid(); ++it)
      {
        PLASMA_TEST_INT(a[it.Id()].m_iData, i);
        ++i;
      }
    }
  }*/
}
