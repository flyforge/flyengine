#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/Containers/Deque.h>
#include <Foundation/IO/ChunkStream.h>
#include <Foundation/IO/MemoryStream.h>

PLASMA_CREATE_SIMPLE_TEST(IO, ChunkStream)
{
  plDefaultMemoryStreamStorage StreamStorage;

  plMemoryStreamWriter MemoryWriter(&StreamStorage);
  plMemoryStreamReader MemoryReader(&StreamStorage);

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Write Format")
  {
    plChunkStreamWriter writer(MemoryWriter);

    writer.BeginStream(1);

    {
      writer.BeginChunk("Chunk1", 1);

      writer << (plUInt32)4;
      writer << (float)5.6f;
      writer << (double)7.8;
      writer << "nine";
      writer << plVec3(10, 11.2f, 13.4f);

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk2", 2);

      writer << "chunk 2 content";

      writer.EndChunk();
    }

    {
      writer.BeginChunk("Chunk3", 3);

      writer << "chunk 3 content";

      writer.EndChunk();
    }

    writer.EndStream();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Read Format")
  {
    plChunkStreamReader reader(MemoryReader);

    reader.BeginStream();

    // Chunk 1
    {
      PLASMA_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      PLASMA_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk1");
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 1);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 36);

      plUInt32 i;
      float f;
      double d;
      plString s;

      reader >> i;
      reader >> f;
      reader >> d;
      reader >> s;

      PLASMA_TEST_INT(i, 4);
      PLASMA_TEST_FLOAT(f, 5.6f, 0);
      PLASMA_TEST_DOUBLE(d, 7.8, 0);
      PLASMA_TEST_STRING(s.GetData(), "nine");

      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 12);
      reader.NextChunk();
    }

    // Chunk 2
    {
      PLASMA_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      PLASMA_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk2");
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 2);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      plString s;

      reader >> s;

      PLASMA_TEST_STRING(s.GetData(), "chunk 2 content");

      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    // Chunk 3
    {
      PLASMA_TEST_BOOL(reader.GetCurrentChunk().m_bValid);
      PLASMA_TEST_STRING(reader.GetCurrentChunk().m_sChunkName.GetData(), "Chunk3");
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkVersion, 3);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, reader.GetCurrentChunk().m_uiUnreadChunkBytes);
      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiChunkBytes, 19);

      plString s;

      reader >> s;

      PLASMA_TEST_STRING(s.GetData(), "chunk 3 content");

      PLASMA_TEST_INT(reader.GetCurrentChunk().m_uiUnreadChunkBytes, 0);
      reader.NextChunk();
    }

    PLASMA_TEST_BOOL(!reader.GetCurrentChunk().m_bValid);

    reader.SetEndChunkFileMode(plChunkStreamReader::EndChunkFileMode::SkipToEnd);
    reader.EndStream();

    plUInt8 Temp[1024];
    PLASMA_TEST_INT(MemoryReader.ReadBytes(Temp, 1024), 0); // nothing left to read
  }
}
