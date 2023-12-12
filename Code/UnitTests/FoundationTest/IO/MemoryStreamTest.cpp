#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/MemoryStream.h>

PLASMA_CREATE_SIMPLE_TEST_GROUP(IO);

PLASMA_CREATE_SIMPLE_TEST(IO, MemoryStream)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Memory Stream Reading / Writing")
  {
    plDefaultMemoryStreamStorage StreamStorage;

    // Create reader
    plMemoryStreamReader StreamReader(&StreamStorage);

    // Create writer
    plMemoryStreamWriter StreamWriter(&StreamStorage);

    // Temp read pointer
    plUInt8* pPointer = reinterpret_cast<plUInt8*>(0x41); // Should crash when accessed

    // Try reading from an empty stream (should not crash, just return 0 bytes read)
    plUInt64 uiBytesRead = StreamReader.ReadBytes(pPointer, 128);

    PLASMA_TEST_BOOL(uiBytesRead == 0);


    // Now try writing data to the stream and reading it back
    plUInt32 uiData[1024];
    for (plUInt32 i = 0; i < 1024; i++)
      uiData[i] = rand();

    // Calculate the hash so we can reuse the array
    const plUInt32 uiHashBeforeWriting = plHashingUtils::xxHash32(uiData, sizeof(plUInt32) * 1024);

    // Write the data
    PLASMA_TEST_BOOL(StreamWriter.WriteBytes(reinterpret_cast<const plUInt8*>(uiData), sizeof(plUInt32) * 1024) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(StreamWriter.GetByteCount64() == sizeof(plUInt32) * 1024);
    PLASMA_TEST_BOOL(StreamWriter.GetByteCount64() == StreamReader.GetByteCount64());
    PLASMA_TEST_BOOL(StreamWriter.GetByteCount64() == StreamStorage.GetStorageSize64());


    // Clear the array for the read back
    plMemoryUtils::ZeroFill(uiData, 1024);

    uiBytesRead = StreamReader.ReadBytes(reinterpret_cast<plUInt8*>(uiData), sizeof(plUInt32) * 1024);

    PLASMA_TEST_BOOL(uiBytesRead == sizeof(plUInt32) * 1024);

    const plUInt32 uiHashAfterReading = plHashingUtils::xxHash32(uiData, sizeof(plUInt32) * 1024);

    PLASMA_TEST_BOOL(uiHashAfterReading == uiHashBeforeWriting);

    // Modify data and test the Rewind() functionality of the writer
    uiData[0] = 0x42;
    uiData[1] = 0x23;

    const plUInt32 uiHashOfModifiedData = plHashingUtils::xxHash32(uiData, sizeof(plUInt32) * 4); // Only test the first 4 elements now

    StreamWriter.SetWritePosition(0);

    StreamWriter.WriteBytes(uiData, sizeof(plUInt32) * 4).IgnoreResult();

    // Clear the array for the read back
    plMemoryUtils::ZeroFill(uiData, 4);

    // Test the rewind of the reader as well
    StreamReader.SetReadPosition(0);

    uiBytesRead = StreamReader.ReadBytes(uiData, sizeof(plUInt32) * 4);

    PLASMA_TEST_BOOL(uiBytesRead == sizeof(plUInt32) * 4);

    const plUInt32 uiHashAfterReadingOfModifiedData = plHashingUtils::xxHash32(uiData, sizeof(plUInt32) * 4);

    PLASMA_TEST_BOOL(uiHashAfterReadingOfModifiedData == uiHashOfModifiedData);

    // Test skipping
    StreamReader.SetReadPosition(0);

    StreamReader.SkipBytes(sizeof(plUInt32));

    plUInt32 uiTemp;

    uiBytesRead = StreamReader.ReadBytes(&uiTemp, sizeof(plUInt32));

    PLASMA_TEST_BOOL(uiBytesRead == sizeof(plUInt32));

    // We skipped over the first 0x42 element, so this should be 0x23
    PLASMA_TEST_BOOL(uiTemp == 0x23);

    // Skip more bytes than available
    plUInt64 uiBytesSkipped = StreamReader.SkipBytes(0xFFFFFFFFFF);

    PLASMA_TEST_BOOL(uiBytesSkipped < 0xFFFFFFFFFF);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Raw Memory Stream Reading")
  {
    plDynamicArray<plUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      OrigStorage[i] = i % 256;
    }

    {
      plRawMemoryStreamReader reader(OrigStorage);

      plDynamicArray<plUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<plUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      PLASMA_TEST_BOOL(OrigStorage == CopyStorage);
    }

    {
      plRawMemoryStreamReader reader(OrigStorage.GetData() + 510, 490);

      plDynamicArray<plUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(static_cast<plUInt32>(reader.GetByteCount()));
      reader.ReadBytes(CopyStorage.GetData(), reader.GetByteCount());

      PLASMA_TEST_BOOL(OrigStorage != CopyStorage);

      for (plUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }

    {
      plRawMemoryStreamReader reader(OrigStorage.GetData(), 1000);
      reader.SkipBytes(510);

      plDynamicArray<plUInt8> CopyStorage;
      CopyStorage.SetCountUninitialized(490);
      reader.ReadBytes(CopyStorage.GetData(), 490);

      PLASMA_TEST_BOOL(OrigStorage != CopyStorage);

      for (plUInt32 i = 0; i < 490; ++i)
      {
        CopyStorage[i] = (i + 10) % 256;
      }
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Raw Memory Stream Writing")
  {
    plDynamicArray<plUInt8> OrigStorage;
    OrigStorage.SetCountUninitialized(1000);

    plRawMemoryStreamWriter writer0;
    PLASMA_TEST_INT(writer0.GetNumWrittenBytes(), 0);
    PLASMA_TEST_INT(writer0.GetStorageSize(), 0);

    plRawMemoryStreamWriter writer(OrigStorage.GetData(), OrigStorage.GetCount());

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      writer << static_cast<plUInt8>(i % 256);

      PLASMA_TEST_INT(writer.GetNumWrittenBytes(), i + 1);
      PLASMA_TEST_INT(writer.GetStorageSize(), 1000);
    }

    for (plUInt32 i = 0; i < 1000; ++i)
    {
      PLASMA_TEST_INT(OrigStorage[i], i % 256);
    }

    {
      plRawMemoryStreamWriter writer2(OrigStorage);
      PLASMA_TEST_INT(writer2.GetNumWrittenBytes(), 0);
      PLASMA_TEST_INT(writer2.GetStorageSize(), 1000);
    }
  }
}

PLASMA_CREATE_SIMPLE_TEST(IO, LargeMemoryStream)
{
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Large Memory Stream Reading / Writing")
  {
    plDefaultMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    const plUInt8 pattern[] = {11, 10, 27, 4, 14, 3, 21, 6};

    plUInt64 uiSize = 0;
    constexpr plUInt64 bytesToTest = 0x8000000llu; // tested with up to 8 GB, but that just takes too long

    // writes n gigabyte
    for (plUInt32 n = 0; n < 8; ++n)
    {
      // writes one gigabyte
      for (plUInt32 gb = 0; gb < 1024; ++gb)
      {
        // writes one megabyte
        for (plUInt32 mb = 0; mb < 1024 * 1024 / PLASMA_ARRAY_SIZE(pattern); ++mb)
        {
          writer.WriteBytes(pattern, PLASMA_ARRAY_SIZE(pattern)).IgnoreResult();
          uiSize += PLASMA_ARRAY_SIZE(pattern);

          if (uiSize == bytesToTest)
            goto check;
        }
      }
    }

  check:
    PLASMA_TEST_BOOL(uiSize == bytesToTest);
    PLASMA_TEST_BOOL(writer.GetWritePosition() == bytesToTest);
    uiSize = 0;

    // reads n gigabyte
    for (plUInt32 n = 0; n < 8; ++n)
    {
      // reads one gigabyte
      for (plUInt32 gb = 0; gb < 1024; ++gb)
      {
        // reads one megabyte
        for (plUInt32 mb = 0; mb < 1024 * 1024 / PLASMA_ARRAY_SIZE(pattern); ++mb)
        {
          plUInt8 pattern2[PLASMA_ARRAY_SIZE(pattern)];

          const plUInt64 uiRead = reader.ReadBytes(pattern2, PLASMA_ARRAY_SIZE(pattern));

          if (uiRead != PLASMA_ARRAY_SIZE(pattern))
          {
            PLASMA_TEST_BOOL(uiRead == 0);
            PLASMA_TEST_BOOL(uiSize == bytesToTest);
            goto endTest;
          }

          uiSize += uiRead;

          if (plMemoryUtils::RawByteCompare(pattern, pattern2, PLASMA_ARRAY_SIZE(pattern)) != 0)
          {
            PLASMA_TEST_BOOL_MSG(false, "Memory read comparison failed.");
            goto endTest;
          }
        }
      }
    }

  endTest:;
    PLASMA_TEST_BOOL(reader.GetReadPosition() == bytesToTest);
  }
}
