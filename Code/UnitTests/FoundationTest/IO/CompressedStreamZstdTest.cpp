#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/CompressedStreamZstd.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/Stream.h>

#ifdef BUILDSYSTEM_ENABLE_ZSTD_SUPPORT

PLASMA_CREATE_SIMPLE_TEST(IO, CompressedStreamZstd)
{
  plDynamicArray<plUInt32> TestData;

  // create the test data
  // a repetition of a counting sequence that is getting longer and longer, ie:
  // 0, 0,1, 0,1,2, 0,1,2,3, 0,1,2,3,4, ...
  {
    TestData.SetCountUninitialized(1024 * 1024 * 8);

    const plUInt32 uiItems = TestData.GetCount();
    plUInt32 uiStartPos = 0;

    for (plUInt32 uiWrite = 1; uiWrite < uiItems; ++uiWrite)
    {
      uiWrite = plMath::Min(uiWrite, uiItems - uiStartPos);

      if (uiWrite == 0)
        break;

      for (plUInt32 i = 0; i < uiWrite; ++i)
      {
        TestData[uiStartPos + i] = i;
      }

      uiStartPos += uiWrite;
    }
  }


  plDefaultMemoryStreamStorage StreamStorage;

  plMemoryStreamWriter MemoryWriter(&StreamStorage);
  plMemoryStreamReader MemoryReader(&StreamStorage);

  plCompressedStreamReaderZstd CompressedReader;
  plCompressedStreamWriterZstd CompressedWriter;

  const float fExpectedCompressionRatio = 900.0f; // this is a guess that is based on the current input data and size

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compress Data")
  {
    CompressedWriter.SetOutputStream(&MemoryWriter);

    bool bFlush = true;

    plUInt32 uiWrite = 1;
    for (plUInt32 i = 0; i < TestData.GetCount();)
    {
      uiWrite = plMath::Min<plUInt32>(uiWrite, TestData.GetCount() - i);

      PLASMA_TEST_BOOL(CompressedWriter.WriteBytes(&TestData[i], sizeof(plUInt32) * uiWrite) == PLASMA_SUCCESS);

      if (bFlush)
      {
        // this actually hurts compression rates
        PLASMA_TEST_BOOL(CompressedWriter.Flush() == PLASMA_SUCCESS);
      }

      bFlush = !bFlush;

      i += uiWrite;
      uiWrite += 17; // try different sizes to write
    }

    // flush all data
    CompressedWriter.FinishCompressedStream().AssertSuccess();

    const plUInt64 uiCompressed = CompressedWriter.GetCompressedSize();
    const plUInt64 uiUncompressed = CompressedWriter.GetUncompressedSize();
    const plUInt64 uiBytesWritten = CompressedWriter.GetWrittenBytes();

    PLASMA_TEST_INT(uiUncompressed, TestData.GetCount() * sizeof(plUInt32));
    PLASMA_TEST_BOOL(uiBytesWritten > uiCompressed);
    PLASMA_TEST_BOOL(uiBytesWritten < uiUncompressed);

    const float fRatio = (float)uiUncompressed / (float)uiCompressed;
    PLASMA_TEST_BOOL(fRatio >= fExpectedCompressionRatio);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Uncompress Data")
  {
    CompressedReader.SetInputStream(&MemoryReader);

    bool bSkip = false;
    plUInt32 uiStartPos = 0;

    plDynamicArray<plUInt32> TestDataRead = TestData; // initialize with identical data, makes comparing the skipped parts easier

    // read the data in blocks that get larger and larger
    for (plUInt32 iRead = 1; iRead < TestData.GetCount(); ++iRead)
    {
      plUInt32 iToRead = plMath::Min(iRead, TestData.GetCount() - uiStartPos);

      if (iToRead == 0)
        break;

      if (bSkip)
      {
        const plUInt64 uiReadFromStream = CompressedReader.SkipBytes(sizeof(plUInt32) * iToRead);
        PLASMA_TEST_BOOL(uiReadFromStream == sizeof(plUInt32) * iToRead);
      }
      else
      {
        // overwrite part we are going to read from the stream, to make sure it re-reads the correct data
        for (plUInt32 i = 0; i < iToRead; ++i)
        {
          TestDataRead[uiStartPos + i] = 0;
        }

        const plUInt64 uiReadFromStream = CompressedReader.ReadBytes(&TestDataRead[uiStartPos], sizeof(plUInt32) * iToRead);
        PLASMA_TEST_BOOL(uiReadFromStream == sizeof(plUInt32) * iToRead);
      }

      bSkip = !bSkip;

      uiStartPos += iToRead;
    }

    PLASMA_TEST_BOOL(TestData == TestDataRead);

    // test reading after the end of the stream
    for (plUInt32 i = 0; i < 1000; ++i)
    {
      plUInt32 uiTemp = 0;
      PLASMA_TEST_BOOL(CompressedReader.ReadBytes(&uiTemp, sizeof(plUInt32)) == 0);
    }
  }
}

#endif
