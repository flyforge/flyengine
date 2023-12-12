#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Time/Stopwatch.h>

namespace
{
  struct SerializableStructWithMethods
  {

    PLASMA_DECLARE_POD_TYPE();

    plResult Serialize(plStreamWriter& inout_stream) const
    {
      inout_stream << m_uiMember1;
      inout_stream << m_uiMember2;

      return PLASMA_SUCCESS;
    }

    plResult Deserialize(plStreamReader& inout_stream)
    {
      inout_stream >> m_uiMember1;
      inout_stream >> m_uiMember2;

      return PLASMA_SUCCESS;
    }

    plInt32 m_uiMember1 = 0x42;
    plInt32 m_uiMember2 = 0x23;
  };
} // namespace

PLASMA_CREATE_SIMPLE_TEST(IO, StreamOperation)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Binary Stream Basic Operations (built-in types)")
  {
    plDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    StreamWriter << (plUInt8)0x42;
    StreamWriter << (plUInt16)0x4223;
    StreamWriter << (plUInt32)0x42232342;
    StreamWriter << (plUInt64)0x4223234242232342;
    StreamWriter << 42.0f;
    StreamWriter << 23.0;
    StreamWriter << (plInt8)0x23;
    StreamWriter << (plInt16)0x2342;
    StreamWriter << (plInt32)0x23422342;
    StreamWriter << (plInt64)0x2342234242232342;

    // Arrays
    {
      plDynamicArray<plUInt32> DynamicArray;
      DynamicArray.PushBack(42);
      DynamicArray.PushBack(23);
      DynamicArray.PushBack(13);
      DynamicArray.PushBack(5);
      DynamicArray.PushBack(0);

      StreamWriter.WriteArray(DynamicArray).IgnoreResult();
    }

    // Create reader
    plMemoryStreamReader StreamReader(&StreamStorage);

    // Read back
    {
      plUInt8 uiVal;
      StreamReader >> uiVal;
      PLASMA_TEST_BOOL(uiVal == (plUInt8)0x42);
    }
    {
      plUInt16 uiVal;
      StreamReader >> uiVal;
      PLASMA_TEST_BOOL(uiVal == (plUInt16)0x4223);
    }
    {
      plUInt32 uiVal;
      StreamReader >> uiVal;
      PLASMA_TEST_BOOL(uiVal == (plUInt32)0x42232342);
    }
    {
      plUInt64 uiVal;
      StreamReader >> uiVal;
      PLASMA_TEST_BOOL(uiVal == (plUInt64)0x4223234242232342);
    }

    {
      float fVal;
      StreamReader >> fVal;
      PLASMA_TEST_BOOL(fVal == 42.0f);
    }
    {
      double dVal;
      StreamReader >> dVal;
      PLASMA_TEST_BOOL(dVal == 23.0f);
    }


    {
      plInt8 iVal;
      StreamReader >> iVal;
      PLASMA_TEST_BOOL(iVal == (plInt8)0x23);
    }
    {
      plInt16 iVal;
      StreamReader >> iVal;
      PLASMA_TEST_BOOL(iVal == (plInt16)0x2342);
    }
    {
      plInt32 iVal;
      StreamReader >> iVal;
      PLASMA_TEST_BOOL(iVal == (plInt32)0x23422342);
    }
    {
      plInt64 iVal;
      StreamReader >> iVal;
      PLASMA_TEST_BOOL(iVal == (plInt64)0x2342234242232342);
    }

    {
      plDynamicArray<plUInt32> ReadBackDynamicArray;

      // This element will be removed by the ReadArray function
      ReadBackDynamicArray.PushBack(0xAAu);

      StreamReader.ReadArray(ReadBackDynamicArray).IgnoreResult();

      PLASMA_TEST_INT(ReadBackDynamicArray.GetCount(), 5);

      PLASMA_TEST_INT(ReadBackDynamicArray[0], 42);
      PLASMA_TEST_INT(ReadBackDynamicArray[1], 23);
      PLASMA_TEST_INT(ReadBackDynamicArray[2], 13);
      PLASMA_TEST_INT(ReadBackDynamicArray[3], 5);
      PLASMA_TEST_INT(ReadBackDynamicArray[4], 0);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Binary Stream Arrays of Structs")
  {
    plDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    // Write out a couple of the structs
    {
      plStaticArray<SerializableStructWithMethods, 16> WriteArray;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x5;
      WriteArray.ExpandAndGetRef().m_uiMember1 = 0x6;

      StreamWriter.WriteArray(WriteArray).IgnoreResult();
    }

    // Read back in
    {
      // Create reader
      plMemoryStreamReader StreamReader(&StreamStorage);

      // This intentionally uses a different array type for the read back
      // to verify that it is a) compatible and b) all arrays are somewhat tested
      plHybridArray<SerializableStructWithMethods, 1> ReadArray;

      StreamReader.ReadArray(ReadArray).IgnoreResult();

      PLASMA_TEST_INT(ReadArray.GetCount(), 2);

      PLASMA_TEST_INT(ReadArray[0].m_uiMember1, 0x5);
      PLASMA_TEST_INT(ReadArray[0].m_uiMember2, 0x23);

      PLASMA_TEST_INT(ReadArray[1].m_uiMember1, 0x6);
      PLASMA_TEST_INT(ReadArray[1].m_uiMember2, 0x23);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plSet Stream Operators")
  {
    plDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    plSet<plString> TestSet;
    TestSet.Insert("Hello");
    TestSet.Insert("World");
    TestSet.Insert("!");

    StreamWriter.WriteSet(TestSet).IgnoreResult();

    plSet<plString> TestSetReadBack;

    TestSetReadBack.Insert("Shouldn't be there after deserialization.");

    plMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadSet(TestSetReadBack).IgnoreResult();

    PLASMA_TEST_INT(TestSetReadBack.GetCount(), 3);

    PLASMA_TEST_BOOL(TestSetReadBack.Contains("Hello"));
    PLASMA_TEST_BOOL(TestSetReadBack.Contains("!"));
    PLASMA_TEST_BOOL(TestSetReadBack.Contains("World"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plMap Stream Operators")
  {
    plDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    plMap<plUInt64, plString> TestMap;
    TestMap.Insert(42, "Hello");
    TestMap.Insert(23, "World");
    TestMap.Insert(5, "!");

    StreamWriter.WriteMap(TestMap).IgnoreResult();

    plMap<plUInt64, plString> TestMapReadBack;

    TestMapReadBack.Insert(1, "Shouldn't be there after deserialization.");

    plMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestMapReadBack).IgnoreResult();

    PLASMA_TEST_INT(TestMapReadBack.GetCount(), 3);

    PLASMA_TEST_BOOL(TestMapReadBack.Contains(42));
    PLASMA_TEST_BOOL(TestMapReadBack.Contains(5));
    PLASMA_TEST_BOOL(TestMapReadBack.Contains(23));

    PLASMA_TEST_BOOL(TestMapReadBack.GetValue(42)->IsEqual("Hello"));
    PLASMA_TEST_BOOL(TestMapReadBack.GetValue(5)->IsEqual("!"));
    PLASMA_TEST_BOOL(TestMapReadBack.GetValue(23)->IsEqual("World"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "plHashTable Stream Operators")
  {
    plDefaultMemoryStreamStorage StreamStorage(4096);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    plHashTable<plUInt64, plString> TestHashTable;
    TestHashTable.Insert(42, "Hello");
    TestHashTable.Insert(23, "World");
    TestHashTable.Insert(5, "!");

    StreamWriter.WriteHashTable(TestHashTable).IgnoreResult();

    plMap<plUInt64, plString> TestHashTableReadBack;

    TestHashTableReadBack.Insert(1, "Shouldn't be there after deserialization.");

    plMemoryStreamReader StreamReader(&StreamStorage);

    StreamReader.ReadMap(TestHashTableReadBack).IgnoreResult();

    PLASMA_TEST_INT(TestHashTableReadBack.GetCount(), 3);

    PLASMA_TEST_BOOL(TestHashTableReadBack.Contains(42));
    PLASMA_TEST_BOOL(TestHashTableReadBack.Contains(5));
    PLASMA_TEST_BOOL(TestHashTableReadBack.Contains(23));

    PLASMA_TEST_BOOL(TestHashTableReadBack.GetValue(42)->IsEqual("Hello"));
    PLASMA_TEST_BOOL(TestHashTableReadBack.GetValue(5)->IsEqual("!"));
    PLASMA_TEST_BOOL(TestHashTableReadBack.GetValue(23)->IsEqual("World"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "String Deduplication")
  {
    plDefaultMemoryStreamStorage StreamStorageNonDeduplicated(4096);
    plDefaultMemoryStreamStorage StreamStorageDeduplicated(4096);

    plHybridString<4> str1 = "Hello World";
    plDynamicString str2 = "Hello World 2";
    plStringBuilder str3 = "Hello Schlumpf";

    // Non deduplicated serialization
    {
      plMemoryStreamWriter StreamWriter(&StreamStorageNonDeduplicated);

      StreamWriter << str1;
      StreamWriter << str2;
      StreamWriter << str1;
      StreamWriter << str3;
      StreamWriter << str1;
      StreamWriter << str2;
    }

    // Deduplicated serialization
    {
      plMemoryStreamWriter StreamWriter(&StreamStorageDeduplicated);

      plStringDeduplicationWriteContext StringDeduplicationContext(StreamWriter);
      auto& DeduplicationWriter = StringDeduplicationContext.Begin();

      DeduplicationWriter << str1;
      DeduplicationWriter << str2;
      DeduplicationWriter << str1;
      DeduplicationWriter << str3;
      DeduplicationWriter << str1;
      DeduplicationWriter << str2;

      StringDeduplicationContext.End().IgnoreResult();

      PLASMA_TEST_INT(StringDeduplicationContext.GetUniqueStringCount(), 3);
    }

    PLASMA_TEST_BOOL(StreamStorageDeduplicated.GetStorageSize64() < StreamStorageNonDeduplicated.GetStorageSize64());

    // Read the deduplicated strings back
    {
      plMemoryStreamReader StreamReader(&StreamStorageDeduplicated);

      plStringDeduplicationReadContext StringDeduplicationReadContext(StreamReader);

      plHybridString<16> szRead0, szRead1, szRead2;
      plStringBuilder szRead3, szRead4, szRead5;

      StreamReader >> szRead0;
      StreamReader >> szRead1;
      StreamReader >> szRead2;
      StreamReader >> szRead3;
      StreamReader >> szRead4;
      StreamReader >> szRead5;

      PLASMA_TEST_STRING(szRead0, szRead2);
      PLASMA_TEST_STRING(szRead0, szRead4);
      PLASMA_TEST_STRING(szRead1, szRead5);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Array Serialization Performance (bytes)")
  {
    constexpr plUInt32 uiCount = 1024 * 1024 * 10;

    plContiguousMemoryStreamStorage storage(uiCount + 16);

    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plDynamicArray<plUInt8> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i] = i & 0xFF;
    }

    {
      plStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      plTime t = sw.GetRunningTotal();
      plStringBuilder s;
      s.Format("Write {} byte array: {}", plArgFileSize(uiCount), t);
      plTestFramework::Output(plTestOutput::Details, s);
    }

    {
      plStopwatch sw;

      reader.ReadArray(DynamicArray).IgnoreResult();

      plTime t = sw.GetRunningTotal();
      plStringBuilder s;
      s.Format("Read {} byte array: {}", plArgFileSize(uiCount), t);
      plTestFramework::Output(plTestOutput::Details, s);
    }

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      PLASMA_TEST_INT(DynamicArray[i], i & 0xFF);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Array Serialization Performance (plVec3)")
  {
    constexpr plUInt32 uiCount = 1024 * 1024 * 10;

    plContiguousMemoryStreamStorage storage(uiCount * sizeof(plVec3) + 16);

    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plDynamicArray<plVec3> DynamicArray;
    DynamicArray.SetCountUninitialized(uiCount);

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      DynamicArray[i].Set(i, i + 1, i + 2);
    }

    {
      plStopwatch sw;

      writer.WriteArray(DynamicArray).AssertSuccess();

      plTime t = sw.GetRunningTotal();
      plStringBuilder s;
      s.Format("Write {} vec3 array: {}", plArgFileSize(uiCount * sizeof(plVec3)), t);
      plTestFramework::Output(plTestOutput::Details, s);
    }

    {
      plStopwatch sw;

      reader.ReadArray(DynamicArray).AssertSuccess();

      plTime t = sw.GetRunningTotal();
      plStringBuilder s;
      s.Format("Read {} vec3 array: {}", plArgFileSize(uiCount * sizeof(plVec3)), t);
      plTestFramework::Output(plTestOutput::Details, s);
    }

    for (plUInt32 i = 0; i < uiCount; ++i)
    {
      PLASMA_TEST_VEC3(DynamicArray[i], plVec3(i, i + 1, i + 2), 0.01f);
    }
  }
}
