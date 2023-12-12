#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashHelperString.h>
#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Algorithm/HashableStruct.h>
#include <Foundation/Strings/HashedString.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(Algorithm);

// Warning for overflow in compile time executed static_assert(plHashingUtils::MurmurHash32...)
// Todo: Why is this not happening elsewhere?
#pragma warning(disable : 4307)

PLASMA_CREATE_SIMPLE_TEST(Algorithm, Hashing)
{
  // check whether compile time hashing gives the same value as runtime hashing
  const char* szString = "This is a test string. 1234";
  const char* szStringLower = "this is a test string. 1234";
  const char* szString2 = "THiS iS A TESt sTrInG. 1234";
  plStringBuilder sb = szString;

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Hashfunction")
  {
    plUInt32 uiHashRT = plHashingUtils::MurmurHash32String(sb.GetData());
    constexpr plUInt32 uiHashCT = plHashingUtils::MurmurHash32String("This is a test string. 1234");
    PLASMA_TEST_INT(uiHashRT, 0xb999d6c4);
    PLASMA_TEST_INT(uiHashRT, uiHashCT);

    // Static assert to ensure this is happening at compile time!
    static_assert(plHashingUtils::MurmurHash32String("This is a test string. 1234") == static_cast<plUInt32>(0xb999d6c4), "Error in compile time murmur hash calculation!");

    {
      // Test short inputs (< 16 characters) of xx hash at compile time
      plUInt32 uixxHashRT = plHashingUtils::xxHash32("Test string", 11, 0);
      plUInt32 uixxHashCT = plHashingUtils::xxHash32String("Test string", 0);
      PLASMA_TEST_INT(uixxHashRT, uixxHashCT);
      static_assert(plHashingUtils::xxHash32String("Test string") == 0x1b50ee03);

      // Test long inputs ( > 16 characters) of xx hash at compile time
      plUInt32 uixxHashRTLong = plHashingUtils::xxHash32String(sb.GetData());
      plUInt32 uixxHashCTLong = plHashingUtils::xxHash32String("This is a test string. 1234");
      PLASMA_TEST_INT(uixxHashRTLong, uixxHashCTLong);
      static_assert(plHashingUtils::xxHash32String("This is a test string. 1234") == 0xff35b049);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      plUInt64 uixxHash64RT = plHashingUtils::xxHash64("Test string", 11, 0);
      plUInt64 uixxHash64CT = plHashingUtils::xxHash64String("Test string", 0);
      PLASMA_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(plHashingUtils::xxHash64String("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      plUInt64 uixxHash64RTLong = plHashingUtils::xxHash64String(plStringView("This is a longer test string for 64-bit. 123456"));
      plUInt64 uixxHash64CTLong = plHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456");
      PLASMA_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(plHashingUtils::xxHash64String("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    {
      // Test short inputs (< 32 characters) of xx hash 64 at compile time
      plUInt64 uixxHash64RT = plHashingUtils::StringHash(plStringView("Test string"));
      plUInt64 uixxHash64CT = plHashingUtils::StringHash("Test string");
      PLASMA_TEST_INT(uixxHash64RT, uixxHash64CT);
      static_assert(plHashingUtils::StringHash("Test string") == 0xcf0f91eece7c88feULL);

      // Test long inputs ( > 32 characters) of xx hash 64 at compile time
      plUInt64 uixxHash64RTLong = plHashingUtils::StringHash(plStringView("This is a longer test string for 64-bit. 123456"));
      plUInt64 uixxHash64CTLong = plHashingUtils::StringHash("This is a longer test string for 64-bit. 123456");
      PLASMA_TEST_INT(uixxHash64RTLong, uixxHash64CTLong);
      static_assert(plHashingUtils::StringHash("This is a longer test string for 64-bit. 123456") == 0xb85d007925299bacULL);
    }

    // Check MurmurHash for unaligned inputs
    const char* alignmentTestString = "12345678_12345678__12345678___12345678";
    plUInt32 uiHash1 = plHashingUtils::MurmurHash32(alignmentTestString, 8);
    plUInt32 uiHash2 = plHashingUtils::MurmurHash32(alignmentTestString + 9, 8);
    plUInt32 uiHash3 = plHashingUtils::MurmurHash32(alignmentTestString + 19, 8);
    plUInt32 uiHash4 = plHashingUtils::MurmurHash32(alignmentTestString + 30, 8);
    PLASMA_TEST_INT(uiHash1, uiHash2);
    PLASMA_TEST_INT(uiHash1, uiHash3);
    PLASMA_TEST_INT(uiHash1, uiHash4);

    // check 64bit hashes
    const plUInt64 uiMurmurHash64 = plHashingUtils::MurmurHash64(sb.GetData(), sb.GetElementCount());
    PLASMA_TEST_INT(uiMurmurHash64, 0xf8ebc5e8cb110786);

    // Check MurmurHash64 for unaligned inputs
    plUInt64 uiHash1_64 = plHashingUtils::MurmurHash64(alignmentTestString, 8);
    plUInt64 uiHash2_64 = plHashingUtils::MurmurHash64(alignmentTestString + 9, 8);
    plUInt64 uiHash3_64 = plHashingUtils::MurmurHash64(alignmentTestString + 19, 8);
    plUInt64 uiHash4_64 = plHashingUtils::MurmurHash64(alignmentTestString + 30, 8);
    PLASMA_TEST_INT(uiHash1_64, uiHash2_64);
    PLASMA_TEST_INT(uiHash1_64, uiHash3_64);
    PLASMA_TEST_INT(uiHash1_64, uiHash4_64);

    // test crc32
    const plUInt32 uiCrc32 = plHashingUtils::CRC32Hash(sb.GetData(), sb.GetElementCount());
    PLASMA_TEST_INT(uiCrc32, 0x73b5e898);

    // Check crc32 for unaligned inputs
    uiHash1 = plHashingUtils::CRC32Hash(alignmentTestString, 8);
    uiHash2 = plHashingUtils::CRC32Hash(alignmentTestString + 9, 8);
    uiHash3 = plHashingUtils::CRC32Hash(alignmentTestString + 19, 8);
    uiHash4 = plHashingUtils::CRC32Hash(alignmentTestString + 30, 8);
    PLASMA_TEST_INT(uiHash1, uiHash2);
    PLASMA_TEST_INT(uiHash1, uiHash3);
    PLASMA_TEST_INT(uiHash1, uiHash4);

    // 32 Bit xxHash
    const plUInt32 uiXXHash32 = plHashingUtils::xxHash32(sb.GetData(), sb.GetElementCount());
    PLASMA_TEST_INT(uiXXHash32, 0xff35b049);

    // Check xxHash for unaligned inputs
    uiHash1 = plHashingUtils::xxHash32(alignmentTestString, 8);
    uiHash2 = plHashingUtils::xxHash32(alignmentTestString + 9, 8);
    uiHash3 = plHashingUtils::xxHash32(alignmentTestString + 19, 8);
    uiHash4 = plHashingUtils::xxHash32(alignmentTestString + 30, 8);
    PLASMA_TEST_INT(uiHash1, uiHash2);
    PLASMA_TEST_INT(uiHash1, uiHash3);
    PLASMA_TEST_INT(uiHash1, uiHash4);

    // 64 Bit xxHash
    const plUInt64 uiXXHash64 = plHashingUtils::xxHash64(sb.GetData(), sb.GetElementCount());
    PLASMA_TEST_INT(uiXXHash64, 0x141fb89c0bf32020);
    // Check xxHash64 for unaligned inputs
    uiHash1_64 = plHashingUtils::xxHash64(alignmentTestString, 8);
    uiHash2_64 = plHashingUtils::xxHash64(alignmentTestString + 9, 8);
    uiHash3_64 = plHashingUtils::xxHash64(alignmentTestString + 19, 8);
    uiHash4_64 = plHashingUtils::xxHash64(alignmentTestString + 30, 8);
    PLASMA_TEST_INT(uiHash1_64, uiHash2_64);
    PLASMA_TEST_INT(uiHash1_64, uiHash3_64);
    PLASMA_TEST_INT(uiHash1_64, uiHash4_64);

    plUInt32 uixxHash32RTEmpty = plHashingUtils::xxHash32("", 0, 0);
    plUInt32 uixxHash32CTEmpty = plHashingUtils::xxHash32String("", 0);
    PLASMA_TEST_BOOL(uixxHash32RTEmpty == uixxHash32CTEmpty);

    plUInt64 uixxHash64RTEmpty = plHashingUtils::xxHash64("", 0, 0);
    plUInt64 uixxHash64CTEmpty = plHashingUtils::xxHash64String("", 0);
    PLASMA_TEST_BOOL(uixxHash64RTEmpty == uixxHash64CTEmpty);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HashHelper")
  {
    plUInt32 uiHash = plHashHelper<plStringBuilder>::Hash(sb);
    PLASMA_TEST_INT(uiHash, 0x0bf32020);

    const char* szTest = "This is a test string. 1234";
    uiHash = plHashHelper<const char*>::Hash(szTest);
    PLASMA_TEST_INT(uiHash, 0x0bf32020);
    PLASMA_TEST_BOOL(plHashHelper<const char*>::Equal(szTest, sb.GetData()));

    plHashedString hs;
    hs.Assign(szTest);
    uiHash = plHashHelper<plHashedString>::Hash(hs);
    PLASMA_TEST_INT(uiHash, 0x0bf32020);

    plTempHashedString ths(szTest);
    uiHash = plHashHelper<plHashedString>::Hash(ths);
    PLASMA_TEST_INT(uiHash, 0x0bf32020);
    PLASMA_TEST_BOOL(plHashHelper<plHashedString>::Equal(hs, ths));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HashHelperString_NoCase")
  {
    const plUInt32 uiHash = plHashHelper<const char*>::Hash(szStringLower);
    PLASMA_TEST_INT(uiHash, 0x19404167);
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(szString));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(szStringLower));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(szString2));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(sb));
    plStringBuilder sb2 = szString2;
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(sb2));
    plString sL = szStringLower;
    plString s1 = sb;
    plString s2 = sb2;
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(s1));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(s2));
    plStringView svL = szStringLower;
    plStringView sv1 = szString;
    plStringView sv2 = szString2;
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(svL));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(sv1));
    PLASMA_TEST_INT(uiHash, plHashHelperString_NoCase::Hash(sv2));

    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sb, sb2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sb, szString2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sb, sv2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(s1, sb2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(s1, szString2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(s1, sv2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sv1, sb2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sv1, szString2));
    PLASMA_TEST_BOOL(plHashHelperString_NoCase::Equal(sv1, sv2));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HashStream32")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, plUInt32* pHash) {
      plHashStreamWriter32 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();
      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const plUInt32 uiHash1 = writer1.GetHashValue();

      plHashStreamWriter32 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
      {
        writer2.Flush().IgnoreResult();
      }

      const plUInt32 uiHash2 = writer2.GetHashValue();

      plHashStreamWriter32 writer3;
      for (plUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();

        if (bFlush)
        {
          writer3.Flush().IgnoreResult();
        }
      }
      const plUInt32 uiHash3 = writer3.GetHashValue();

      PLASMA_TEST_INT(uiHash1, uiHash2);
      PLASMA_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    plUInt32 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    PLASMA_TEST_INT(uiHash1, uiHash2);

    const plUInt64 uiHash3 = plHashingUtils::xxHash32(szTest, std::strlen(szTest));
    PLASMA_TEST_INT(uiHash1, uiHash3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "HashStream64")
  {
    const char* szTest = "This is a test string. 1234";
    const char* szTestHalf1 = "This is a test";
    const char* szTestHalf2 = " string. 1234";

    auto test = [szTest, szTestHalf1, szTestHalf2](bool bFlush, plUInt64* pHash) {
      plHashStreamWriter64 writer1;
      writer1.WriteBytes(szTest, std::strlen(szTest)).IgnoreResult();

      if (bFlush)
      {
        writer1.Flush().IgnoreResult();
      }

      const plUInt64 uiHash1 = writer1.GetHashValue();

      plHashStreamWriter64 writer2;
      writer2.WriteBytes(szTestHalf1, std::strlen(szTestHalf1)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();
      writer2.WriteBytes(szTestHalf2, std::strlen(szTestHalf2)).IgnoreResult();
      if (bFlush)
        writer2.Flush().IgnoreResult();

      const plUInt64 uiHash2 = writer2.GetHashValue();

      plHashStreamWriter64 writer3;
      for (plUInt64 i = 0; szTest[i] != 0; ++i)
      {
        writer3.WriteBytes(szTest + i, 1).IgnoreResult();
        if (bFlush)
          writer3.Flush().IgnoreResult();
      }
      const plUInt64 uiHash3 = writer3.GetHashValue();

      PLASMA_TEST_INT(uiHash1, uiHash2);
      PLASMA_TEST_INT(uiHash1, uiHash3);

      *pHash = uiHash1;
    };

    plUInt64 uiHash1 = 0, uiHash2 = 1;
    test(true, &uiHash1);
    test(false, &uiHash2);
    PLASMA_TEST_INT(uiHash1, uiHash2);

    const plUInt64 uiHash3 = plHashingUtils::xxHash64(szTest, std::strlen(szTest));
    PLASMA_TEST_INT(uiHash1, uiHash3);
  }
}

struct SimpleHashableStruct : public plHashableStruct<SimpleHashableStruct>
{
  plUInt32 m_uiTestMember1;
  plUInt8 m_uiTestMember2;
  plUInt64 m_uiTestMember3;
};

struct SimpleStruct
{
  plUInt32 m_uiTestMember1;
  plUInt8 m_uiTestMember2;
  plUInt64 m_uiTestMember3;
};

PLASMA_CREATE_SIMPLE_TEST(Algorithm, HashableStruct)
{
  SimpleHashableStruct AutomaticInst;
  PLASMA_TEST_INT(AutomaticInst.m_uiTestMember1, 0);
  PLASMA_TEST_INT(AutomaticInst.m_uiTestMember2, 0);
  PLASMA_TEST_INT(AutomaticInst.m_uiTestMember3, 0);

  SimpleStruct NonAutomaticInst;
  plMemoryUtils::ZeroFill(&NonAutomaticInst, 1);

  PLASMA_CHECK_AT_COMPILETIME(sizeof(AutomaticInst) == sizeof(NonAutomaticInst));

  PLASMA_TEST_INT(plMemoryUtils::Compare<plUInt8>((plUInt8*)&AutomaticInst, (plUInt8*)&NonAutomaticInst, sizeof(AutomaticInst)), 0);

  AutomaticInst.m_uiTestMember2 = 0x42u;
  AutomaticInst.m_uiTestMember3 = 0x23u;

  plUInt32 uiAutomaticHash = AutomaticInst.CalculateHash();

  NonAutomaticInst.m_uiTestMember2 = 0x42u;
  NonAutomaticInst.m_uiTestMember3 = 0x23u;

  plUInt32 uiNonAutomaticHash = plHashingUtils::xxHash32(&NonAutomaticInst, sizeof(NonAutomaticInst));

  PLASMA_TEST_INT(uiAutomaticHash, uiNonAutomaticHash);

  AutomaticInst.m_uiTestMember1 = 0x5u;
  uiAutomaticHash = AutomaticInst.CalculateHash();

  PLASMA_TEST_BOOL(uiAutomaticHash != uiNonAutomaticHash);
}
