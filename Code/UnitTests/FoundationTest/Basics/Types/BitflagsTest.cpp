#include <FoundationTest/FoundationTestPCH.h>

namespace
{
  // declare bitflags using macro magic
  PLASMA_DECLARE_FLAGS(plUInt32, AutoFlags, Bit1, Bit2, Bit3, Bit4);

  // declare bitflags manually
  struct ManualFlags
  {
    using StorageType = plUInt32;

    enum Enum
    {
      Bit1 = PLASMA_BIT(0),
      Bit2 = PLASMA_BIT(1),
      Bit3 = PLASMA_BIT(2),
      Bit4 = PLASMA_BIT(3),

      Default = Bit1 | Bit2
    };

    struct Bits
    {
      StorageType Bit1 : 1;
      StorageType Bit2 : 1;
      StorageType Bit3 : 1;
      StorageType Bit4 : 1;
    };
  };

  PLASMA_DECLARE_FLAGS_OPERATORS(ManualFlags);
} // namespace

PLASMA_CHECK_AT_COMPILETIME(sizeof(plBitflags<AutoFlags>) == 4);

PLASMA_CREATE_SIMPLE_TEST(Basics, Bitflags)
{
  PLASMA_TEST_BOOL(AutoFlags::Count == 4);

  {
    plBitflags<AutoFlags> flags = AutoFlags::Bit1 | AutoFlags::Bit4;

    PLASMA_TEST_BOOL(flags.IsSet(AutoFlags::Bit4));
    PLASMA_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit1 | AutoFlags::Bit4));
    PLASMA_TEST_BOOL(flags.IsAnySet(AutoFlags::Bit1 | AutoFlags::Bit2));
    PLASMA_TEST_BOOL(!flags.IsAnySet(AutoFlags::Bit2 | AutoFlags::Bit3));
    PLASMA_TEST_BOOL(flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit3));
    PLASMA_TEST_BOOL(!flags.AreNoneSet(AutoFlags::Bit2 | AutoFlags::Bit4));

    flags.Add(AutoFlags::Bit3);
    PLASMA_TEST_BOOL(flags.IsSet(AutoFlags::Bit3));

    flags.Remove(AutoFlags::Bit1);
    PLASMA_TEST_BOOL(!flags.IsSet(AutoFlags::Bit1));

    flags.Toggle(AutoFlags::Bit4);
    PLASMA_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit3));

    flags.AddOrRemove(AutoFlags::Bit2, true);
    flags.AddOrRemove(AutoFlags::Bit3, false);
    PLASMA_TEST_BOOL(flags.AreAllSet(AutoFlags::Bit2));

    flags.Add(AutoFlags::Bit1);

    plBitflags<ManualFlags> manualFlags = ManualFlags::Default;
    PLASMA_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Bit1 | ManualFlags::Bit2));
    PLASMA_TEST_BOOL(manualFlags.GetValue() == flags.GetValue());
    PLASMA_TEST_BOOL(manualFlags.AreAllSet(ManualFlags::Default & ManualFlags::Bit2));

    PLASMA_TEST_BOOL(flags.IsAnyFlagSet());
    flags.Clear();
    PLASMA_TEST_BOOL(flags.IsNoFlagSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator&")
  {
    plBitflags<AutoFlags> flags2 = AutoFlags::Bit1 & AutoFlags::Bit4;
    PLASMA_TEST_BOOL(flags2.GetValue() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetValue")
  {
    plBitflags<AutoFlags> flags;
    flags.SetValue(17);
    PLASMA_TEST_BOOL(flags.GetValue() == 17);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator|=")
  {
    plBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2;
    f |= AutoFlags::Bit3;

    PLASMA_TEST_BOOL(f.GetValue() == (AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3).GetValue());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator&=")
  {
    plBitflags<AutoFlags> f = AutoFlags::Bit1 | AutoFlags::Bit2 | AutoFlags::Bit3;
    f &= AutoFlags::Bit3;

    PLASMA_TEST_BOOL(f.GetValue() == AutoFlags::Bit3);
  }
}


//////////////////////////////////////////////////////////////////////////

namespace
{
  struct TypelessFlags1
  {
    enum Enum
    {
      Bit1 = PLASMA_BIT(0),
      Bit2 = PLASMA_BIT(1),
    };
  };

  struct TypelessFlags2
  {
    enum Enum
    {
      Bit3 = PLASMA_BIT(2),
      Bit4 = PLASMA_BIT(3),
    };
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(Basics, TypelessBitflags)
{
  {
    plTypelessBitflags<plUInt32> flags = TypelessFlags1::Bit1 | TypelessFlags2::Bit4;

    PLASMA_TEST_BOOL(flags.IsAnySet(TypelessFlags2::Bit4));
    PLASMA_TEST_BOOL(flags.AreAllSet(TypelessFlags1::Bit1 | TypelessFlags2::Bit4));
    PLASMA_TEST_BOOL(flags.IsAnySet(TypelessFlags1::Bit1 | TypelessFlags1::Bit2));
    PLASMA_TEST_BOOL(!flags.IsAnySet(TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
    PLASMA_TEST_BOOL(flags.AreNoneSet(TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
    PLASMA_TEST_BOOL(!flags.AreNoneSet(TypelessFlags1::Bit2 | TypelessFlags2::Bit4));

    flags.Add(TypelessFlags2::Bit3);
    PLASMA_TEST_BOOL(flags.IsAnySet(TypelessFlags2::Bit3));

    flags.Remove(TypelessFlags1::Bit1);
    PLASMA_TEST_BOOL(!flags.IsAnySet(TypelessFlags1::Bit1));

    flags.Toggle(TypelessFlags2::Bit4);
    PLASMA_TEST_BOOL(flags.AreAllSet(TypelessFlags2::Bit3));

    flags.AddOrRemove(TypelessFlags1::Bit2, true);
    flags.AddOrRemove(TypelessFlags2::Bit3, false);
    PLASMA_TEST_BOOL(flags.AreAllSet(TypelessFlags1::Bit2));

    PLASMA_TEST_BOOL(!flags.IsNoFlagSet());
    flags.Clear();
    PLASMA_TEST_BOOL(flags.IsNoFlagSet());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator&")
  {
    plTypelessBitflags<plUInt32> flags2 = TypelessFlags1::Bit1 & TypelessFlags2::Bit4;
    PLASMA_TEST_BOOL(flags2.GetValue() == 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "SetValue")
  {
    plTypelessBitflags<plUInt32> flags;
    PLASMA_TEST_BOOL(flags.IsNoFlagSet());
    PLASMA_TEST_BOOL(flags.GetValue() == 0);
    flags.SetValue(17);
    PLASMA_TEST_BOOL(flags.GetValue() == 17);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator|=")
  {
    plTypelessBitflags<plUInt32> f = TypelessFlags1::Bit1 | TypelessFlags1::Bit2;
    f |= TypelessFlags2::Bit3;

    PLASMA_TEST_BOOL(f.GetValue() == (TypelessFlags1::Bit1 | TypelessFlags1::Bit2 | TypelessFlags2::Bit3));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "operator&=")
  {
    plTypelessBitflags<plUInt32> f = TypelessFlags1::Bit1 | TypelessFlags1::Bit2 | TypelessFlags2::Bit3;
    f &= TypelessFlags2::Bit3;

    PLASMA_TEST_BOOL(f.GetValue() == TypelessFlags2::Bit3);
  }
}
