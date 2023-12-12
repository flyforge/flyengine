#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Memory/EndianHelper.h>

namespace
{
  struct TempStruct
  {
    float fVal;
    plUInt32 uiDVal;
    plUInt16 uiWVal1;
    plUInt16 uiWVal2;
    char pad[4];
  };

  struct FloatAndInt
  {
    union
    {
      float fVal;
      plUInt32 uiVal;
    };
  };
} // namespace


PLASMA_CREATE_SIMPLE_TEST(Memory, Endian)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Basics")
  {
// Test if the IsBigEndian() delivers the same result as the #define
#if PLASMA_ENABLED(PLASMA_PLATFORM_LITTLE_ENDIAN)
    PLASMA_TEST_BOOL(!plEndianHelper::IsBigEndian());
#elif PLASMA_ENABLED(PLASMA_PLATFORM_BIG_ENDIAN)
    PLASMA_TEST_BOOL(plEndianHelper::IsBigEndian());
#endif

    // Test conversion functions for single elements
    PLASMA_TEST_BOOL(plEndianHelper::Switch(static_cast<plUInt16>(0x15FF)) == 0xFF15);
    PLASMA_TEST_BOOL(plEndianHelper::Switch(static_cast<plUInt32>(0x34AA12FF)) == 0xFF12AA34);
    PLASMA_TEST_BOOL(plEndianHelper::Switch(static_cast<plUInt64>(0x34AA12FFABC3421E)) == 0x1E42C3ABFF12AA34);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Switching Arrays")
  {
    plArrayPtr<plUInt16> p16BitArray = PLASMA_DEFAULT_NEW_ARRAY(plUInt16, 1024);
    plArrayPtr<plUInt16> p16BitArrayCopy = PLASMA_DEFAULT_NEW_ARRAY(plUInt16, 1024);

    plArrayPtr<plUInt32> p32BitArray = PLASMA_DEFAULT_NEW_ARRAY(plUInt32, 1024);
    plArrayPtr<plUInt32> p32BitArrayCopy = PLASMA_DEFAULT_NEW_ARRAY(plUInt32, 1024);

    plArrayPtr<plUInt64> p64BitArray = PLASMA_DEFAULT_NEW_ARRAY(plUInt64, 1024);
    plArrayPtr<plUInt64> p64BitArrayCopy = PLASMA_DEFAULT_NEW_ARRAY(plUInt64, 1024);

    for (plUInt32 i = 0; i < 1024; i++)
    {
      plInt32 iRand = rand();
      p16BitArray[i] = static_cast<plUInt16>(iRand);
      p32BitArray[i] = static_cast<plUInt32>(iRand);
      p64BitArray[i] = static_cast<plUInt64>(iRand | static_cast<plUInt64>((iRand % 3)) << 32);
    }

    p16BitArrayCopy.CopyFrom(p16BitArray);
    p32BitArrayCopy.CopyFrom(p32BitArray);
    p64BitArrayCopy.CopyFrom(p64BitArray);

    plEndianHelper::SwitchWords(p16BitArray.GetPtr(), 1024);
    plEndianHelper::SwitchDWords(p32BitArray.GetPtr(), 1024);
    plEndianHelper::SwitchQWords(p64BitArray.GetPtr(), 1024);

    for (plUInt32 i = 0; i < 1024; i++)
    {
      PLASMA_TEST_BOOL(p16BitArray[i] == plEndianHelper::Switch(p16BitArrayCopy[i]));
      PLASMA_TEST_BOOL(p32BitArray[i] == plEndianHelper::Switch(p32BitArrayCopy[i]));
      PLASMA_TEST_BOOL(p64BitArray[i] == plEndianHelper::Switch(p64BitArrayCopy[i]));

      // Test in place switcher
      plEndianHelper::SwitchInPlace(&p16BitArrayCopy[i]);
      PLASMA_TEST_BOOL(p16BitArray[i] == p16BitArrayCopy[i]);

      plEndianHelper::SwitchInPlace(&p32BitArrayCopy[i]);
      PLASMA_TEST_BOOL(p32BitArray[i] == p32BitArrayCopy[i]);

      plEndianHelper::SwitchInPlace(&p64BitArrayCopy[i]);
      PLASMA_TEST_BOOL(p64BitArray[i] == p64BitArrayCopy[i]);
    }


    PLASMA_DEFAULT_DELETE_ARRAY(p16BitArray);
    PLASMA_DEFAULT_DELETE_ARRAY(p16BitArrayCopy);

    PLASMA_DEFAULT_DELETE_ARRAY(p32BitArray);
    PLASMA_DEFAULT_DELETE_ARRAY(p32BitArrayCopy);

    PLASMA_DEFAULT_DELETE_ARRAY(p64BitArray);
    PLASMA_DEFAULT_DELETE_ARRAY(p64BitArrayCopy);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Switching Structs")
  {
    TempStruct instance = {42.0f, 0x34AA12FF, 0x15FF, 0x23FF, {'E', 'Z', 'F', 'T'}};

    plEndianHelper::SwitchStruct(&instance, "ddwwcccc");

    plIntFloatUnion floatHelper(42.0f);
    plIntFloatUnion floatHelper2(instance.fVal);

    PLASMA_TEST_BOOL(floatHelper2.i == plEndianHelper::Switch(floatHelper.i));
    PLASMA_TEST_BOOL(instance.uiDVal == plEndianHelper::Switch(static_cast<plUInt32>(0x34AA12FF)));
    PLASMA_TEST_BOOL(instance.uiWVal1 == plEndianHelper::Switch(static_cast<plUInt16>(0x15FF)));
    PLASMA_TEST_BOOL(instance.uiWVal2 == plEndianHelper::Switch(static_cast<plUInt16>(0x23FF)));
    PLASMA_TEST_BOOL(instance.pad[0] == 'E');
    PLASMA_TEST_BOOL(instance.pad[1] == 'Z');
    PLASMA_TEST_BOOL(instance.pad[2] == 'F');
    PLASMA_TEST_BOOL(instance.pad[3] == 'T');
  }
}
