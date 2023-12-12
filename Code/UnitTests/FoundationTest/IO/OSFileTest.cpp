#include <FoundationTest/FoundationTestPCH.h>

#include <Foundation/IO/OSFile.h>

PLASMA_CREATE_SIMPLE_TEST(IO, OSFile)
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

  const plUInt32 uiTextLen = sFileContent.GetElementCount();

  plStringBuilder sOutputFile = plTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile.MakeCleanPath();
  sOutputFile.AppendPath("IO", "SubFolder");
  sOutputFile.AppendPath("OSFile_TestFile.txt");

  plStringBuilder sOutputFile2 = plTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile2.MakeCleanPath();
  sOutputFile2.AppendPath("IO", "SubFolder2");
  sOutputFile2.AppendPath("OSFile_TestFileCopy.txt");

  plStringBuilder sOutputFile3 = plTestFramework::GetInstance()->GetAbsOutputPath();
  sOutputFile3.MakeCleanPath();
  sOutputFile3.AppendPath("IO", "SubFolder2", "SubSubFolder");
  sOutputFile3.AppendPath("RandomFile.txt");

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Write File")
  {
    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile.GetData(), plFileOpenMode::Write) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(f.IsOpen());
    PLASMA_TEST_INT(f.GetFilePosition(), 0);
    PLASMA_TEST_INT(f.GetFileSize(), 0);

    for (plUInt32 i = 0; i < uiTextLen; ++i)
    {
      PLASMA_TEST_BOOL(f.Write(&sFileContent.GetData()[i], 1) == PLASMA_SUCCESS);
      PLASMA_TEST_INT(f.GetFilePosition(), i + 1);
      PLASMA_TEST_INT(f.GetFileSize(), i + 1);
    }

    PLASMA_TEST_INT(f.GetFilePosition(), uiTextLen);
    f.SetFilePosition(5, plFileSeekMode::FromStart);
    PLASMA_TEST_INT(f.GetFileSize(), uiTextLen);

    PLASMA_TEST_INT(f.GetFilePosition(), 5);
    // f.Close(); // The file should be closed automatically
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Append File")
  {
    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile.GetData(), plFileOpenMode::Append) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(f.IsOpen());
    PLASMA_TEST_INT(f.GetFilePosition(), uiTextLen);
    PLASMA_TEST_BOOL(f.Write(sFileContent.GetData(), uiTextLen) == PLASMA_SUCCESS);
    PLASMA_TEST_INT(f.GetFilePosition(), uiTextLen * 2);
    f.Close();
    PLASMA_TEST_BOOL(!f.IsOpen());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Read File")
  {
    const plUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile.GetData(), plFileOpenMode::Read) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(f.IsOpen());
    PLASMA_TEST_INT(f.GetFilePosition(), 0);

    PLASMA_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);
    PLASMA_TEST_INT(f.GetFilePosition(), uiTextLen * 2);

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Copy File")
  {
    plOSFile::CopyFile(sOutputFile.GetData(), sOutputFile2.GetData()).IgnoreResult();

    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile2.GetData(), plFileOpenMode::Read) == PLASMA_SUCCESS);

    const plUInt32 FS_MAX_PATH = 1024;
    char szTemp[FS_MAX_PATH];

    PLASMA_TEST_INT(f.Read(szTemp, FS_MAX_PATH), uiTextLen * 2);

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(szTemp, sFileContent.GetData(), uiTextLen));
    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(&szTemp[uiTextLen], sFileContent.GetData(), uiTextLen));

    f.Close();
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ReadAll")
  {
    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile, plFileOpenMode::Read) == PLASMA_SUCCESS);

    plDynamicArray<plUInt8> fileContent;
    const plUInt64 bytes = f.ReadAll(fileContent);

    PLASMA_TEST_INT(bytes, uiTextLen * 2);

    PLASMA_TEST_BOOL(plMemoryUtils::IsEqual(fileContent.GetData(), (const plUInt8*)sFileContent.GetData(), uiTextLen));

    f.Close();
  }

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS)
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "File Stats")
  {
    plFileStats s;

    plStringBuilder dir = sOutputFile2.GetFileDirectory();

    PLASMA_TEST_BOOL(plOSFile::GetFileStats(sOutputFile2.GetData(), s) == PLASMA_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(plSIUnitOfTime::Microsecond));

    PLASMA_TEST_BOOL(plOSFile::GetFileStats(dir.GetData(), s) == PLASMA_SUCCESS);
    // printf("%s Name: '%s' (%lli Bytes), Modified Time: %lli\n", s.m_bIsDirectory ? "Directory" : "File", s.m_sFileName.GetData(),
    // s.m_uiFileSize, s.m_LastModificationTime.GetInt64(plSIUnitOfTime::Microsecond));
  }

#  if (PLASMA_ENABLED(PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS) && PLASMA_ENABLED(PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS))
  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetFileCasing")
  {
    plStringBuilder dir = sOutputFile2;
    dir.ToLower();

#    if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS)
    // On Windows the drive letter will always be turned upper case by plOSFile::GetFileCasing()
    // ensure that our input data ('ground truth') also uses an upper case drive letter
    auto driveLetterIterator = sOutputFile2.GetIteratorFront();
    const plUInt32 uiDriveLetter = plStringUtils::ToUpperChar(driveLetterIterator.GetCharacter());
    sOutputFile2.ChangeCharacter(driveLetterIterator, uiDriveLetter);
#    endif

    plStringBuilder sCorrected;
    PLASMA_TEST_BOOL(plOSFile::GetFileCasing(dir.GetData(), sCorrected) == PLASMA_SUCCESS);

    // On Windows the drive letter will always be made to upper case
    PLASMA_TEST_STRING(sCorrected.GetData(), sOutputFile2.GetData());
  }
#  endif // PLASMA_SUPPORTS_CASE_INSENSITIVE_PATHS && PLASMA_SUPPORTS_UNRESTRICTED_FILE_ACCESS

#endif // PLASMA_SUPPORTS_FILE_STATS

#if PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS)

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "File Iterator")
  {
    // It is not really possible to test this stuff (with a guaranteed result), as long as we do not have
    // a test data folder with deterministic content
    // Therefore I tested it manually, and leave the code in, such that it is at least a 'does it compile and link' test.

    plStringBuilder sOutputFolder = plOSFile::GetApplicationDirectory();
    sOutputFolder.AppendPath("*");

    plStringBuilder sFullPath;

    plUInt32 uiFolders = 0;
    plUInt32 uiFiles = 0;

    bool bSkipFolder = true;

    plFileSystemIterator it;
    for (it.StartSearch(sOutputFolder.GetData(), plFileSystemIteratorFlags::ReportFilesAndFoldersRecursive); it.IsValid();)
    {
      sFullPath = it.GetCurrentPath();
      sFullPath.AppendPath(it.GetStats().m_sName.GetData());

      it.GetStats();
      it.GetCurrentPath();

      if (it.GetStats().m_bIsDirectory)
      {
        ++uiFolders;
        bSkipFolder = !bSkipFolder;

        if (bSkipFolder)
        {
          it.SkipFolder(); // replaces the 'Next' call
          continue;
        }
      }
      else
      {
        ++uiFiles;
      }

      it.Next();
    }

// The binary folder will only have subdirectories on windows desktop
#  if PLASMA_ENABLED(PLASMA_PLATFORM_WINDOWS_DESKTOP)
    PLASMA_TEST_BOOL(uiFolders > 0);
#  endif
    PLASMA_TEST_BOOL(uiFiles > 0);
  }

#endif

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Delete File")
  {
    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile.GetData()) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile.GetData()) == PLASMA_SUCCESS); // second time should still 'succeed'

    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile2.GetData()) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile2.GetData()) == PLASMA_SUCCESS); // second time should still 'succeed'

    plOSFile f;
    PLASMA_TEST_BOOL(f.Open(sOutputFile.GetData(), plFileOpenMode::Read) == PLASMA_FAILURE);  // file should not exist anymore
    PLASMA_TEST_BOOL(f.Open(sOutputFile2.GetData(), plFileOpenMode::Read) == PLASMA_FAILURE); // file should not exist anymore
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetCurrentWorkingDirectory")
  {
    plStringBuilder cwd = plOSFile::GetCurrentWorkingDirectory();

    PLASMA_TEST_BOOL(!cwd.IsEmpty());
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "MakePathAbsoluteWithCWD")
  {
    plStringBuilder cwd = plOSFile::GetCurrentWorkingDirectory();
    plStringBuilder path = plOSFile::MakePathAbsoluteWithCWD("sub/folder");

    PLASMA_TEST_BOOL(path.StartsWith(cwd));
    PLASMA_TEST_BOOL(path.EndsWith("/sub/folder"));
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExistsFile")
  {
    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile.GetData()) == false);
    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      plOSFile f;
      PLASMA_TEST_BOOL(f.Open(sOutputFile.GetData(), plFileOpenMode::Write) == PLASMA_SUCCESS);
    }

    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile.GetData()) == true);
    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile2.GetData()) == false);

    {
      plOSFile f;
      PLASMA_TEST_BOOL(f.Open(sOutputFile2.GetData(), plFileOpenMode::Write) == PLASMA_SUCCESS);
    }

    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile.GetData()) == true);
    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile2.GetData()) == true);

    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile.GetData()) == PLASMA_SUCCESS);
    PLASMA_TEST_BOOL(plOSFile::DeleteFile(sOutputFile2.GetData()) == PLASMA_SUCCESS);

    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile.GetData()) == false);
    PLASMA_TEST_BOOL(plOSFile::ExistsFile(sOutputFile2.GetData()) == false);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "ExistsDirectory")
  {
    // files are not folders
    PLASMA_TEST_BOOL(plOSFile::ExistsDirectory(sOutputFile.GetData()) == false);
    PLASMA_TEST_BOOL(plOSFile::ExistsDirectory(sOutputFile2.GetData()) == false);

    plStringBuilder sOutputFolder = plTestFramework::GetInstance()->GetAbsOutputPath();
    PLASMA_TEST_BOOL(plOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("IO");
    PLASMA_TEST_BOOL(plOSFile::ExistsDirectory(sOutputFolder) == true);

    sOutputFile.AppendPath("SubFolder");
    PLASMA_TEST_BOOL(plOSFile::ExistsDirectory(sOutputFolder) == true);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GetApplicationDirectory")
  {
    plStringView sAppDir = plOSFile::GetApplicationDirectory();
    PLASMA_TEST_BOOL(!sAppDir.IsEmpty());
  }

#if (PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_ITERATORS) && PLASMA_ENABLED(PLASMA_SUPPORTS_FILE_STATS))

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "DeleteFolder")
  {
    {
      plOSFile f;
      PLASMA_TEST_BOOL(f.Open(sOutputFile3.GetData(), plFileOpenMode::Write) == PLASMA_SUCCESS);
    }

    plStringBuilder SubFolder2 = plTestFramework::GetInstance()->GetAbsOutputPath();
    SubFolder2.MakeCleanPath();
    SubFolder2.AppendPath("IO", "SubFolder2");

    PLASMA_TEST_BOOL(plOSFile::DeleteFolder(SubFolder2).Succeeded());
    PLASMA_TEST_BOOL(!plOSFile::ExistsDirectory(SubFolder2));
  }

#endif
}
