#include <FoundationTest/FoundationTestPCH.h>

//////////////////////////////////////////////////////////////////////
// Start of the definition of a example Enum
// It takes quite some lines of code to define a enum,
// but it could be encapsulated into an preprocessor macro if wanted
struct plTestEnumBase
{
  using StorageType = plUInt8; // The storage type for the enum

  enum Enum
  {
    No = 0,
    Yes = 1,
    Default = No // Default initialization
  };
};

using plTestEnum = plEnum<plTestEnumBase>; // The name of the final enum
// End of the definition of a example enum
///////////////////////////////////////////////////////////////////////

struct plTestEnum2Base
{
  using StorageType = plUInt16;

  enum Enum
  {
    Bit1 = PLASMA_BIT(0),
    Bit2 = PLASMA_BIT(1),
    Default = Bit1
  };
};

using plTestEnum2 = plEnum<plTestEnum2Base>;

// Test if the type actually has the requested size
PLASMA_CHECK_AT_COMPILETIME(sizeof(plTestEnum) == sizeof(plUInt8));
PLASMA_CHECK_AT_COMPILETIME(sizeof(plTestEnum2) == sizeof(plUInt16));

PLASMA_CREATE_SIMPLE_TEST_GROUP(Basics);

// This takes a c++ enum. Tests the implict conversion
void TakeEnum1(plTestEnum::Enum value) {}

// This takes our own enum type
void TakeEnum2(plTestEnum value) {}

PLASMA_CREATE_SIMPLE_TEST(Basics, Enum)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Default initialized enum") { plTestEnum e1; }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enum with explicit initialization") { plTestEnum e2(plTestEnum::Yes); }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "This tests if the default initialization works and if the implicit conversion works")
  {
    plTestEnum e1;
    plTestEnum e2(plTestEnum::Yes);

    PLASMA_TEST_BOOL(e1 == plTestEnum::No);
    PLASMA_TEST_BOOL(e2 == plTestEnum::Yes);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Function call tests")
  {
    plTestEnum e1;

    TakeEnum1(e1);
    TakeEnum2(e1);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetValue and SetValue")
  {
    plTestEnum e1;
    PLASMA_TEST_INT(e1.GetValue(), 0);
    e1.SetValue(17);
    PLASMA_TEST_INT(e1.GetValue(), 17);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Assignment of different values")
  {
    plTestEnum e1, e2;

    e1 = plTestEnum::Yes;
    e2 = plTestEnum::No;
    PLASMA_TEST_BOOL(e1 == plTestEnum::Yes);
    PLASMA_TEST_BOOL(e2 == plTestEnum::No);

    e1 = e2;
    PLASMA_TEST_BOOL(e1 == plTestEnum::No);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Test the | operator")
  {
    plTestEnum2 e3(plTestEnum2::Bit1);
    plTestEnum2 e4(plTestEnum2::Bit2);
    plUInt16 uiBits = (e3 | e4).GetValue();
    PLASMA_TEST_BOOL(uiBits == (plTestEnum2::Bit1 | plTestEnum2::Bit2));
  }


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Test the & operator")
  {
    plTestEnum2 e3(plTestEnum2::Bit1);
    plTestEnum2 e4(plTestEnum2::Bit2);
    plUInt16 uiBits = ((e3 | e4) & e4).GetValue();
    PLASMA_TEST_BOOL(uiBits == plTestEnum2::Bit2);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Test conversion to int")
  {
    plTestEnum e1;
    int iTest = e1.GetValue();
    PLASMA_TEST_BOOL(iTest == plTestEnum::No);
  }
}
