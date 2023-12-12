#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Bitfield.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Strings/String.h>

PLASMA_CREATE_SIMPLE_TEST(Containers, Bitfield)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCount / IsEmpty / Clear")
  {
    plDynamicBitfield bf; // using a dynamic array

    PLASMA_TEST_INT(bf.GetCount(), 0);
    PLASMA_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(15, false);

    PLASMA_TEST_INT(bf.GetCount(), 15);
    PLASMA_TEST_BOOL(!bf.IsEmpty());

    bf.Clear();

    PLASMA_TEST_INT(bf.GetCount(), 0);
    PLASMA_TEST_BOOL(bf.IsEmpty());

    bf.SetCount(37, false);

    PLASMA_TEST_INT(bf.GetCount(), 37);
    PLASMA_TEST_BOOL(!bf.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / SetAllBits / ClearAllBits")
  {
    plHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(249, false);
    PLASMA_TEST_INT(bf.GetCount(), 249);

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();
    PLASMA_TEST_INT(bf.GetCount(), 249);

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();
    PLASMA_TEST_INT(bf.GetCount(), 249);

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));


    bf.SetCount(349, true);
    PLASMA_TEST_INT(bf.GetCount(), 349);

    for (plUInt32 i = 0; i < 249; ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));

    for (plUInt32 i = 249; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(bf.IsBitSet(i));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetCount / SetBit / ClearBit / SetBitValue / SetCountUninitialized")
  {
    plHybridBitfield<512> bf; // using a hybrid array

    bf.SetCount(100, false);
    PLASMA_TEST_INT(bf.GetCount(), 100);

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetCount(200, true);
    PLASMA_TEST_INT(bf.GetCount(), 200);

    for (plUInt32 i = 100; i < bf.GetCount(); ++i)
      PLASMA_TEST_BOOL(bf.IsBitSet(i));

    bf.SetCountUninitialized(250);
    PLASMA_TEST_INT(bf.GetCount(), 250);

    bf.ClearAllBits();

    for (plUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    for (plUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      PLASMA_TEST_BOOL(bf.IsBitSet(i));
      PLASMA_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (plUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (plUInt32 i = 0; i < bf.GetCount(); i += 2)
    {
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));
      PLASMA_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (plUInt32 i = 0; i < bf.GetCount(); ++i)
    {
      PLASMA_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetBitRange")
  {
    for (plUInt32 size = 1; size < 1024; ++size)
    {
      plBitfield<plDeque<plUInt32>> bf; // using a deque
      bf.SetCount(size, false);

      PLASMA_TEST_INT(bf.GetCount(), size);

      for (plUInt32 count = 0; count < bf.GetCount(); ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));

      plUInt32 uiStart = size / 2;
      plUInt32 uiEnd = plMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (plUInt32 count = 0; count < uiStart; ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
      for (plUInt32 count = uiStart; count <= uiEnd; ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
      for (plUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClearBitRange")
  {
    for (plUInt32 size = 1; size < 1024; ++size)
    {
      plBitfield<plDeque<plUInt32>> bf; // using a deque
      bf.SetCount(size, true);

      PLASMA_TEST_INT(bf.GetCount(), size);

      for (plUInt32 count = 0; count < bf.GetCount(); ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));

      plUInt32 uiStart = size / 2;
      plUInt32 uiEnd = plMath::Min(uiStart + (size / 3 * 2), size - 1);

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (plUInt32 count = 0; count < uiStart; ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
      for (plUInt32 count = uiStart; count <= uiEnd; ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
      for (plUInt32 count = uiEnd + 1; count < bf.GetCount(); ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    plHybridBitfield<512> bf; // using a hybrid array

    PLASMA_TEST_BOOL(bf.IsEmpty() == true);
    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == false); // empty
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == true);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    bf.SetCount(250, false);

    PLASMA_TEST_BOOL(bf.IsEmpty() == false);
    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == false);
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == true);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (plUInt32 i = 0; i < bf.GetCount(); i += 2)
      bf.SetBit(i);

    PLASMA_TEST_BOOL(bf.IsEmpty() == false);
    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == true);
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == false);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (plUInt32 i = 0; i < bf.GetCount(); i++)
      bf.SetBit(i);

    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == true);
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == false);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == true);
  }
}


PLASMA_CREATE_SIMPLE_TEST(Containers, StaticBitfield)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetAllBits / ClearAllBits")
  {
    plStaticBitfield64 bf;

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));

    bf.SetAllBits();

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
      PLASMA_TEST_BOOL(bf.IsBitSet(i));

    bf.ClearAllBits();

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetBit / ClearBit / SetBitValue")
  {
    plStaticBitfield32 bf;

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));

    for (plUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    for (plUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      PLASMA_TEST_BOOL(bf.IsBitSet(i));
      PLASMA_TEST_BOOL(!bf.IsBitSet(i + 1));
    }

    for (plUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      bf.ClearBit(i);
      bf.SetBit(i + 1);
    }

    for (plUInt32 i = 0; i < bf.GetNumBits(); i += 2)
    {
      PLASMA_TEST_BOOL(!bf.IsBitSet(i));
      PLASMA_TEST_BOOL(bf.IsBitSet(i + 1));
    }

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
    {
      bf.SetBitValue(i, (i % 3) == 0);
    }

    for (plUInt32 i = 0; i < bf.GetNumBits(); ++i)
    {
      PLASMA_TEST_BOOL(bf.IsBitSet(i) == ((i % 3) == 0));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetBitRange")
  {
    for (plUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      plStaticBitfield64 bf;

      for (plUInt32 count = 0; count < bf.GetNumBits(); ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));

      plUInt32 uiEnd = uiStart + 3;

      bf.SetBitRange(uiStart, uiEnd - uiStart + 1);

      for (plUInt32 count = 0; count < uiStart; ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
      for (plUInt32 count = uiStart; count <= uiEnd; ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
      for (plUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ClearBitRange")
  {
    for (plUInt32 uiStart = 0; uiStart < 61; ++uiStart)
    {
      plStaticBitfield64 bf;
      bf.SetAllBits();

      for (plUInt32 count = 0; count < bf.GetNumBits(); ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));

      plUInt32 uiEnd = uiStart + 3;

      bf.ClearBitRange(uiStart, uiEnd - uiStart + 1);

      for (plUInt32 count = 0; count < uiStart; ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
      for (plUInt32 count = uiStart; count <= uiEnd; ++count)
        PLASMA_TEST_BOOL(!bf.IsBitSet(count));
      for (plUInt32 count = uiEnd + 1; count < bf.GetNumBits(); ++count)
        PLASMA_TEST_BOOL(bf.IsBitSet(count));
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "IsAnyBitSet / IsNoBitSet / AreAllBitsSet")
  {
    plStaticBitfield8 bf;

    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == false); // empty
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == true);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == false); // empty

    for (plUInt32 i = 0; i < bf.GetNumBits(); i += 2)
      bf.SetBit(i);

    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == true);
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == false);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == false);

    for (plUInt32 i = 0; i < bf.GetNumBits(); i++)
      bf.SetBit(i);

    PLASMA_TEST_BOOL(bf.IsAnyBitSet() == true);
    PLASMA_TEST_BOOL(bf.IsNoBitSet() == false);
    PLASMA_TEST_BOOL(bf.AreAllBitsSet() == true);
  }
}
