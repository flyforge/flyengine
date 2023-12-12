#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/Archive/Archive.h>
#include <Foundation/IO/Archive/DataDirTypeArchive.h>
#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/System/Process.h>
#include <Foundation/Utilities/CommandLineUtils.h>

#if (PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS) && defined(BUILDSYSTEM_HAS_ARCHIVE_TOOL))

PLASMA_CREATE_SIMPLE_TEST(IO, Archive)
{
  plStringBuilder sOutputFolder = plTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFolder.AppendPath("ArchiveTest");
  sOutputFolder.MakeCleanPath();

  // make sure it is empty
  plOSFile::DeleteFolder(sOutputFolder).IgnoreResult();
  plOSFile::CreateDirectoryStructure(sOutputFolder).IgnoreResult();

  if (!PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder, "Clear", "output", plFileSystem::AllowWrites).Succeeded()))
    return;

  const char* szTestData = "TestData";
  const char* szUnpackedData = "Unpacked";

  // write a couple of files for packaging
  const char* szFileList[] = {
    "File1.txt",
    "FolderA/File2.jpg", // should get stored uncompressed
    "FolderB/File3.txt",
    "FolderA/FolderC/File4.zip", // should get stored uncompressed
    "FolderA/FolderD/File5.txt",
    "File6.txt",
  };

  const plUInt32 uiMinFileSize = 1024 * 128;


  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Generate Data")
  {
    plUInt64 uiValue = 0;

    plStringBuilder fileName;

    for (plUInt32 uiFileIdx = 0; uiFileIdx < PLASMA_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      fileName.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);

      plFileWriter file;
      if (!PLASMA_TEST_BOOL(file.Open(fileName).Succeeded()))
        return;

      for (plUInt32 i = 0; i < uiMinFileSize * uiFileIdx; ++i)
      {
        file << uiValue;
        ++uiValue;
      }
    }
  }

  const plStringBuilder sArchiveFolder(sOutputFolder, "/", szTestData);
  const plStringBuilder sUnpackFolder(sOutputFolder, "/", szUnpackedData);
  const plStringBuilder sArchiveFile(sOutputFolder, "/", szTestData, ".plArchive");

  plStringBuilder pathToArchiveTool = plCommandLineUtils::GetGlobalInstance()->GetParameter(0);
  pathToArchiveTool.PathParentDirectory();
  pathToArchiveTool.AppendPath("ArchiveTool.exe");

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create a Package")
  {

    plProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack(sArchiveFolder);

    plInt32 iReturnValue = 1;

    plProcess ArchiveToolProc;
    if (!PLASMA_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    PLASMA_TEST_INT(iReturnValue, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Unpack the Package")
  {

    plProcessOptions opt;
    opt.m_sProcess = pathToArchiveTool;
    opt.m_Arguments.PushBack("-unpack");
    opt.m_Arguments.PushBack(sArchiveFile);
    opt.m_Arguments.PushBack("-out");
    opt.m_Arguments.PushBack(sUnpackFolder);

    plInt32 iReturnValue = 1;

    plProcess ArchiveToolProc;
    if (!PLASMA_TEST_BOOL(ArchiveToolProc.Execute(opt, &iReturnValue).Succeeded()))
      return;

    PLASMA_TEST_INT(iReturnValue, 0);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Compare unpacked data")
  {
    plUInt64 uiValue = 0;

    plStringBuilder sFileSrc;
    plStringBuilder sFileDst;

    for (plUInt32 uiFileIdx = 0; uiFileIdx < PLASMA_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(sOutputFolder, "/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(sOutputFolder, "/", szUnpackedData, "/", szFileList[uiFileIdx]);

      PLASMA_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Mount as Data Dir")
  {
    if (!PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive", plFileSystem::ReadOnly) == PLASMA_SUCCESS))
      return;

    plStringBuilder sFileSrc;
    plStringBuilder sFileDst;

    // test opening multiple files in parallel and keeping them open
    plFileReader readers[PLASMA_ARRAY_SIZE(szFileList)];
    for (plUInt32 uiFileIdx = 0; uiFileIdx < PLASMA_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);
      PLASMA_TEST_BOOL(readers[uiFileIdx].Open(sFileDst).Succeeded());

      // advance the reader a bit
      PLASMA_TEST_INT(readers[uiFileIdx].SkipBytes(uiMinFileSize * uiFileIdx), uiMinFileSize * uiFileIdx);
    }

    for (plUInt32 uiFileIdx = 0; uiFileIdx < PLASMA_ARRAY_SIZE(szFileList); ++uiFileIdx)
    {
      sFileSrc.Set(":output/", szTestData, "/", szFileList[uiFileIdx]);
      sFileDst.Set(":archive/", szFileList[uiFileIdx]);

      PLASMA_TEST_FILES(sFileSrc, sFileDst, "Unpacked file should be identical");
    }

    // mount a second time
    if (!PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sArchiveFile, "Clear", "archive2", plFileSystem::ReadOnly) == PLASMA_SUCCESS))
      return;
  }

  plFileSystem::RemoveDataDirectoryGroup("Clear");
}

#endif
