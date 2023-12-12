#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/FileSystem/DataDirTypeFolder.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_LONG_PATHS)
#  define LongPath                                                                                                                                   \
    "AVeryLongSubFolderPathNameThatShouldExceedThePathLengthLimitOnPlatformsLikeWindowsWhereOnly260CharactersAreAllowedOhNoesIStillNeedMoreThisIsNo" \
    "tLongEnoughAaaaaaaaaaaaaaahhhhStillTooShortAaaaaaaaaaaaaaaaaaaaaahImBoredNow"
#else
#  define LongPath "AShortPathBecaueThisPlatformDoesntSupportLongOnes"
#endif

PLASMA_CREATE_SIMPLE_TEST(IO, FileSystem)
{
  plStringBuilder sFileContent = "Lyrics to Taste The Cake:\n\
Turret: Who's there?\n\
Turret: Is anyone there?\n\
Turret: I see you.\n\
\n\
Chell rises from a stasis inside of a glass box\n\
She isn't greeted by faces,\n\
Only concrete and clocks.\n\
...";

  plStringBuilder szOutputFolder = plTestFramework::GetInstance()->GetAbsOutputPath();
  szOutputFolder.MakeCleanPath();

  plStringBuilder sOutputFolderResolved;
  plFileSystem::ResolveSpecialDirectory(szOutputFolder, sOutputFolderResolved).IgnoreResult();

  plStringBuilder sOutputFolder1 = szOutputFolder;
  sOutputFolder1.AppendPath("IO", "SubFolder");
  plStringBuilder sOutputFolder1Resolved;
  plFileSystem::ResolveSpecialDirectory(sOutputFolder1, sOutputFolder1Resolved).IgnoreResult();

  plStringBuilder sOutputFolder2 = szOutputFolder;
  sOutputFolder2.AppendPath("IO", "SubFolder2");
  plStringBuilder sOutputFolder2Resolved;
  plFileSystem::ResolveSpecialDirectory(sOutputFolder2, sOutputFolder2Resolved).IgnoreResult();

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Setup Data Dirs")
  {
    // adding the same factory three times would actually not make a difference
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);
    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);

    // plFileSystem::ClearAllDataDirectoryFactories();

    plFileSystem::RegisterDataDirectoryFactory(plDataDirectory::FolderType::Factory);

    // for absolute paths
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory("", "", ":", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(szOutputFolder, "Clear", "output", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

    plStringBuilder sTempFile = sOutputFolder1Resolved;
    sTempFile.AppendPath(LongPath);
    sTempFile.AppendPath("Temp.tmp");

    plFileWriter TempFile;
    PLASMA_TEST_BOOL(TempFile.Open(sTempFile) == PLASMA_SUCCESS);
    TempFile.Close();

    sTempFile = sOutputFolder2Resolved;
    sTempFile.AppendPath("Temp.tmp");

    PLASMA_TEST_BOOL(TempFile.Open(sTempFile) == PLASMA_SUCCESS);
    TempFile.Close();

    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder1, "Clear", "output1", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Clear") == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == PLASMA_SUCCESS);

    PLASMA_TEST_INT(plFileSystem::RemoveDataDirectoryGroup("Remove"), 3);

    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder1, "Remove") == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Remove") == PLASMA_SUCCESS);

    plFileSystem::ClearAllDataDirectories();

    PLASMA_TEST_INT(plFileSystem::RemoveDataDirectoryGroup("Remove"), 0);
    PLASMA_TEST_INT(plFileSystem::RemoveDataDirectoryGroup("Clear"), 0);

    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder1, "", "output1", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2) == PLASMA_SUCCESS);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Add / Remove Data Dirs")
  {
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory("", "xyz-rooted", "xyz", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(plFileSystem::FindDataDirectoryWithRoot("xyz") != nullptr);

    PLASMA_TEST_BOOL(plFileSystem::RemoveDataDirectory("xyz") == true);

    PLASMA_TEST_BOOL(plFileSystem::FindDataDirectoryWithRoot("xyz") == nullptr);

    PLASMA_TEST_BOOL(plFileSystem::RemoveDataDirectory("xyz") == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Write File")
  {
    plFileWriter FileOut;

    plStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    PLASMA_TEST_BOOL(FileOut.Open(":output1/FileSystemTest.txt") == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(FileOut.GetFilePathRelative(), "FileSystemTest.txt");
    PLASMA_TEST_STRING(FileOut.GetFilePathAbsolute(), sAbs);

    PLASMA_TEST_INT(FileOut.GetFileSize(), 0);

    PLASMA_TEST_BOOL(FileOut.WriteBytes(sFileContent.GetData(), sFileContent.GetElementCount()) == PLASMA_SUCCESS);

    FileOut.Flush().IgnoreResult();
    PLASMA_TEST_INT(FileOut.GetFileSize(), sFileContent.GetElementCount());

    FileOut.Close();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Read File")
  {
    plFileReader FileIn;

    plStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    PLASMA_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    PLASMA_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    PLASMA_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    PLASMA_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#if PLASMA_DISABLED(PLASMA_PLATFORM_WINDOWS_UWP)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Read File (Absolute Path)")
  {
    plFileReader FileIn;

    plStringBuilder sAbs = sOutputFolder1Resolved;
    sAbs.AppendPath("FileSystemTest.txt");

    PLASMA_TEST_BOOL(FileIn.Open(sAbs) == PLASMA_SUCCESS);

    PLASMA_TEST_STRING(FileIn.GetFilePathRelative(), "FileSystemTest.txt");
    PLASMA_TEST_STRING(FileIn.GetFilePathAbsolute(), sAbs);

    PLASMA_TEST_INT(FileIn.GetFileSize(), sFileContent.GetElementCount());

    char szTemp[1024 * 2];
    PLASMA_TEST_INT(FileIn.ReadBytes(szTemp, 1024 * 2), sFileContent.GetElementCount());

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), sFileContent.GetElementCount()));

    FileIn.Close();
  }

#endif

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Delete File / Exists File")
  {
    {
      PLASMA_TEST_BOOL(plFileSystem::ExistsFile(":output1/FileSystemTest.txt"));
      plFileSystem::DeleteFile(":output1/FileSystemTest.txt");
      PLASMA_TEST_BOOL(!plFileSystem::ExistsFile("FileSystemTest.txt"));

      plFileReader FileIn;
      PLASMA_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == PLASMA_FAILURE);
    }

    // very long path names
    {
      plStringBuilder sTempFile = ":output1";
      sTempFile.AppendPath(LongPath);
      sTempFile.AppendPath("Temp.tmp");

      plFileWriter TempFile;
      PLASMA_TEST_BOOL(TempFile.Open(sTempFile) == PLASMA_SUCCESS);
      TempFile.Close();

      PLASMA_TEST_BOOL(plFileSystem::ExistsFile(sTempFile));
      plFileSystem::DeleteFile(sTempFile);
      PLASMA_TEST_BOOL(!plFileSystem::ExistsFile(sTempFile));

      plFileReader FileIn;
      PLASMA_TEST_BOOL(FileIn.Open("FileSystemTest.txt") == PLASMA_FAILURE);
    }
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileStats")
  {
    const char* szPath = ":output1/" LongPath "/FileSystemTest.txt";

    // Create file
    {
      plFileWriter FileOut;
      plStringBuilder sAbs = sOutputFolder1Resolved;
      sAbs.AppendPath("FileSystemTest.txt");
      PLASMA_TEST_BOOL(FileOut.Open(szPath) == PLASMA_SUCCESS);
      FileOut.WriteBytes("Test", 4).IgnoreResult();
    }

    plFileStats stat;

    PLASMA_TEST_BOOL(plFileSystem::GetFileStats(szPath, stat).Succeeded());

    PLASMA_TEST_BOOL(!stat.m_bIsDirectory);
    PLASMA_TEST_STRING(stat.m_sName, "FileSystemTest.txt");
    PLASMA_TEST_INT(stat.m_uiFileSize, 4);

    plFileSystem::DeleteFile(szPath);
    PLASMA_TEST_BOOL(plFileSystem::GetFileStats(szPath, stat).Failed());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ResolvePath")
  {
    plStringBuilder sRel, sAbs;

    PLASMA_TEST_BOOL(plFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == PLASMA_SUCCESS);

    plStringBuilder sExpectedAbs = sOutputFolder1Resolved;
    sExpectedAbs.AppendPath("FileSystemTest2.txt");

    PLASMA_TEST_STRING(sAbs, sExpectedAbs);
    PLASMA_TEST_STRING(sRel, "FileSystemTest2.txt");

    // create a file in the second dir
    {
      PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "Remove", "output2", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

      {
        plFileWriter FileOut;
        PLASMA_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == PLASMA_SUCCESS);
      }

      PLASMA_TEST_INT(plFileSystem::RemoveDataDirectoryGroup("Remove"), 1);
    }

    // find the path to an existing file
    {
      PLASMA_TEST_BOOL(plFileSystem::ResolvePath("FileSystemTest2.txt", &sAbs, &sRel) == PLASMA_SUCCESS);

      sExpectedAbs = sOutputFolder2Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      PLASMA_TEST_STRING(sAbs, sExpectedAbs);
      PLASMA_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      PLASMA_TEST_BOOL(plFileSystem::ResolvePath(":output1/FileSystemTest2.txt", &sAbs, &sRel) == PLASMA_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("FileSystemTest2.txt");

      PLASMA_TEST_STRING(sAbs, sExpectedAbs);
      PLASMA_TEST_STRING(sRel, "FileSystemTest2.txt");
    }

    // find where we would write the file to (ignoring existing files)
    {
      PLASMA_TEST_BOOL(plFileSystem::ResolvePath(":output1/SubSub/FileSystemTest2.txt", &sAbs, &sRel) == PLASMA_SUCCESS);

      sExpectedAbs = sOutputFolder1Resolved;
      sExpectedAbs.AppendPath("SubSub/FileSystemTest2.txt");

      PLASMA_TEST_STRING(sAbs, sExpectedAbs);
      PLASMA_TEST_STRING(sRel, "SubSub/FileSystemTest2.txt");
    }

    plFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    plFileSystem::DeleteFile(":output2/FileSystemTest2.txt");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "FindFolderWithSubPath")
  {
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(szOutputFolder, "remove", "toplevel", plFileSystem::AllowWrites) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plFileSystem::AddDataDirectory(sOutputFolder2, "remove", "output2", plFileSystem::AllowWrites) == PLASMA_SUCCESS);

    plStringBuilder StartPath;
    plStringBuilder SubPath;
    plStringBuilder result, expected;

    // make sure this exists
    {
      plFileWriter FileOut;
      PLASMA_TEST_BOOL(FileOut.Open(":output2/FileSystemTest2.txt") == PLASMA_SUCCESS);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("DoesNotExist");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Failed());
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2");
      expected.Set(sOutputFolderResolved, "/IO/");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      PLASMA_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      PLASMA_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub");
      SubPath.Set("IO/SubFolder2");
      expected.Set(sOutputFolderResolved, "/");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      PLASMA_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(sOutputFolder1Resolved, "/SubSub", "/Irrelevant");
      SubPath.Set("SubFolder2/FileSystemTest2.txt");
      expected.Set(sOutputFolderResolved, "/IO/");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      PLASMA_TEST_STRING(result, expected);
    }

    {
      StartPath.Set(":toplevel/IO/SubFolder");
      SubPath.Set("IO/SubFolder2");
      expected.Set(":toplevel/");

      PLASMA_TEST_BOOL(plFileSystem::FindFolderWithSubPath(result, StartPath, SubPath).Succeeded());
      PLASMA_TEST_STRING(result, expected);
    }

    plFileSystem::DeleteFile(":output1/FileSystemTest2.txt");
    plFileSystem::DeleteFile(":output2/FileSystemTest2.txt");

    plFileSystem::RemoveDataDirectoryGroup("remove");
  }
}
