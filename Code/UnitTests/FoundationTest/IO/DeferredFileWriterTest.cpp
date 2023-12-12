#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>

PLASMA_CREATE_SIMPLE_TEST(IO, DeferredFileWriter)
{
  PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory("", "", ":", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

  const plStringBuilder szOutputFolder = plTestFramework::GetInstance()->GetAbsOutputPath();
  plStringBuilder sOutputFolderResolved;
  plFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  plStringBuilder sTempFile = sOutputFolderResolved;
  sTempFile.AppendPath("Temp.tmp");

  // make sure the file does not exist
  plFileSystem::DeleteFile(sTempFile);
  PLASMA_TEST_BOOL(!plFileSystem::ExistsFile(sTempFile));

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DeferredFileWriter")
  {
    plDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (plUInt64 i = 0; i < 1'000'000; ++i)
    {
      writer << i;
    }

    // does not exist yet
    PLASMA_TEST_BOOL(!plFileSystem::ExistsFile(sTempFile));
  }

  // now it exists
  PLASMA_TEST_BOOL(plFileSystem::ExistsFile(sTempFile));

  // check content is correct
  {
    plFileReader reader;
    PLASMA_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (plUInt64 i = 0; i < 1'000'000; ++i)
    {
      plUInt64 v;
      reader >> v;
      PLASMA_TEST_BOOL(v == i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DeferredFileWriter2")
  {
    plDeferredFileWriter writer;
    writer.SetOutput(sTempFile);

    for (plUInt64 i = 1; i < 100'000; ++i)
    {
      writer << i;
    }

    // does exist from earlier
    PLASMA_TEST_BOOL(plFileSystem::ExistsFile(sTempFile));

    // check content is as previous correct
    {
      plFileReader reader;
      PLASMA_TEST_BOOL(reader.Open(sTempFile).Succeeded());

      for (plUInt64 i = 0; i < 1'000'000; ++i)
      {
        plUInt64 v;
        reader >> v;
        PLASMA_TEST_BOOL(v == i);
      }
    }
  }

  // exist but now was overwritten
  PLASMA_TEST_BOOL(plFileSystem::ExistsFile(sTempFile));

  // check content is as previous correct
  {
    plFileReader reader;
    PLASMA_TEST_BOOL(reader.Open(sTempFile).Succeeded());

    for (plUInt64 i = 1; i < 100'000; ++i)
    {
      plUInt64 v;
      reader >> v;
      PLASMA_TEST_BOOL(v == i);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Discard")
  {
    plStringBuilder sTempFile2 = sOutputFolderResolved;
    sTempFile2.AppendPath("Temp2.tmp");
    {
      plDeferredFileWriter writer;
      writer.SetOutput(sTempFile2);
      writer << 10;
      writer.Discard();
    }
    PLASMA_TEST_BOOL(!plFileSystem::ExistsFile(sTempFile2));
  }

  plFileSystem::DeleteFile(sTempFile);
  plFileSystem::ClearAllDataDirectories();
}
