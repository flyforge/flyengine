#include <FoundationTest/FoundationTestPCH.h>

#if PLASMA_ENABLED(PLASMA_SUPPORTS_DIRECTORY_WATCHER)

#  include <Foundation/IO/DirectoryWatcher.h>
#  include <Foundation/IO/OSFile.h>
#  include <Foundation/Threading/ThreadUtils.h>

namespace DirectoryWatcherTestHelpers
{

  struct ExpectedEvent
  {
    ~ExpectedEvent(){}; // NOLINT: To make it non-pod

    const char* path;
    plDirectoryWatcherAction action;
    plDirectoryWatcherType type;

    bool operator==(const ExpectedEvent& other) const
    {
      return plStringView(path) == plStringView(other.path) && action == other.action && type == other.type;
    }
  };

  struct ExpectedEventStorage
  {
    plString path;
    plDirectoryWatcherAction action;
    plDirectoryWatcherType type;
  };

  void TickWatcher(plDirectoryWatcher& ref_watcher)
  {
    ref_watcher.EnumerateChanges([&](plStringView sPath, plDirectoryWatcherAction action, plDirectoryWatcherType type) {},
      plTime::Milliseconds(100));
  }
} // namespace DirectoryWatcherTestHelpers

PLASMA_CREATE_SIMPLE_TEST(IO, DirectoryWatcher)
{
  using namespace DirectoryWatcherTestHelpers;

  plStringBuilder tmp, tmp2;
  plStringBuilder sTestRootPath = plTestFramework::GetInstance()->GetAbsOutputPath();
  sTestRootPath.AppendPath("DirectoryWatcher/");

  auto CheckExpectedEvents = [&](plDirectoryWatcher& ref_watcher, plArrayPtr<ExpectedEvent> events) {
    plDynamicArray<ExpectedEventStorage> firedEvents;
    plUInt32 i = 0;
    ref_watcher.EnumerateChanges([&](plStringView sPath, plDirectoryWatcherAction action, plDirectoryWatcherType type) {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      if (i < events.GetCount())
      {
        PLASMA_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
        PLASMA_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
        PLASMA_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
      }
      i++;
    },
      plTime::Milliseconds(100));
    PLASMA_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsUnordered = [&](plDirectoryWatcher& ref_watcher, plArrayPtr<ExpectedEvent> events) {
    plDynamicArray<ExpectedEventStorage> firedEvents;
    plUInt32 i = 0;
    plDynamicArray<bool> eventFired;
    eventFired.SetCount(events.GetCount());
    ref_watcher.EnumerateChanges([&](plStringView sPath, plDirectoryWatcherAction action, plDirectoryWatcherType type) {
      tmp = sPath;
      tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
      firedEvents.PushBack({tmp, action, type});
      auto index = events.IndexOf({tmp, action, type});
      PLASMA_TEST_BOOL_MSG(index != plInvalidIndex, "Event %d (%s, %d, %d) not found in expected events list", i, tmp.GetData(), (int)action, (int)type);
      if (index != plInvalidIndex)
      {
        eventFired[index] = true;
      }
      i++;
      //
    },
      plTime::Milliseconds(100));
    for (auto& fired : eventFired)
    {
      PLASMA_TEST_BOOL(fired);
    }
    PLASMA_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CheckExpectedEventsMultiple = [&](plArrayPtr<plDirectoryWatcher*> watchers, plArrayPtr<ExpectedEvent> events) {
    plDynamicArray<ExpectedEventStorage> firedEvents;
    plUInt32 i = 0;
    plDirectoryWatcher::EnumerateChanges(
      watchers, [&](plStringView sPath, plDirectoryWatcherAction action, plDirectoryWatcherType type) {
        tmp = sPath;
        tmp.Shrink(sTestRootPath.GetCharacterCount(), 0);
        firedEvents.PushBack({tmp, action, type});
        if (i < events.GetCount())
        {
          PLASMA_TEST_BOOL_MSG(tmp == events[i].path, "Expected event at index %d path mismatch: '%s' vs '%s'", i, tmp.GetData(), events[i].path);
          PLASMA_TEST_BOOL_MSG(action == events[i].action, "Expected event at index %d action", i);
          PLASMA_TEST_BOOL_MSG(type == events[i].type, "Expected event at index %d type mismatch", i);
        }
        i++;
        //
      },
      plTime::Milliseconds(100));
    PLASMA_TEST_BOOL_MSG(firedEvents.GetCount() == events.GetCount(), "Directory watcher did not fire expected amount of events");
  };

  auto CreateFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    plOSFile file;
    PLASMA_TEST_BOOL(file.Open(tmp, plFileOpenMode::Write).Succeeded());
    PLASMA_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto ModifyFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);

    plOSFile file;
    PLASMA_TEST_BOOL(file.Open(tmp, plFileOpenMode::Append).Succeeded());
    PLASMA_TEST_BOOL(file.Write("Hello World", 11).Succeeded());
  };

  auto DeleteFile = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    PLASMA_TEST_BOOL(plOSFile::DeleteFile(tmp).Succeeded());
  };

  auto CreateDirectory = [&](const char* szRelPath) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(tmp).Succeeded());
  };

  auto Rename = [&](const char* szFrom, const char* szTo) {
    tmp = sTestRootPath;
    tmp.AppendPath(szFrom);

    tmp2 = sTestRootPath;
    tmp2.AppendPath(szTo);

    PLASMA_TEST_BOOL(plOSFile::MoveFileOrDirectory(tmp, tmp2).Succeeded());
  };

  auto DeleteDirectory = [&](const char* szRelPath, bool bTest = true) {
    tmp = sTestRootPath;
    tmp.AppendPath(szRelPath);
    tmp.MakeCleanPath();

    if (bTest)
    {
      PLASMA_TEST_BOOL(plOSFile::DeleteFolder(tmp).Succeeded());
    }
    else
    {
      plOSFile::DeleteFolder(tmp).IgnoreResult();
    }
  };

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple Create File")
  {
    plOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple delete file")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple modify file")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Writes).Succeeded());

    CreateFile("test.file");

    TickWatcher(watcher);

    ModifyFile("test.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("test.file");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple rename file")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Renames).Succeeded());

    CreateFile("test.file");
    Rename("test.file", "supertest.file");

    ExpectedEvent expectedEvents[] = {
      {"test.file", plDirectoryWatcherAction::RenamedOldName, plDirectoryWatcherType::File},
      {"supertest.file", plDirectoryWatcherAction::RenamedNewName, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteFile("supertest.file");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple create directory")
  {
    plOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
    PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(sTestRootPath).Succeeded());

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Creates).Succeeded());

    CreateDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple delete directory")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Deletes).Succeeded());

    DeleteDirectory("testDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Simple rename directory")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Renames).Succeeded());

    CreateDirectory("testDir");
    Rename("testDir", "supertestDir");

    ExpectedEvent expectedEvents[] = {
      {"testDir", plDirectoryWatcherAction::RenamedOldName, plDirectoryWatcherType::Directory},
      {"supertestDir", plDirectoryWatcherAction::RenamedNewName, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents);

    DeleteDirectory("supertestDir");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Subdirectory Create File")
  {
    tmp = sTestRootPath;
    tmp.AppendPath("subdir");
    PLASMA_TEST_BOOL(plOSFile::CreateDirectoryStructure(tmp).Succeeded());

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Subdirectory delete file")
  {

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Deletes | plDirectoryWatcher::Watch::Subdirectories).Succeeded());

    DeleteFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Subdirectory modify file")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(sTestRootPath, plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories).Succeeded());

    CreateFile("subdir/test.file");

    TickWatcher(watcher);

    ModifyFile("subdir/test.file");

    ExpectedEvent expectedEvents[] = {
      {"subdir/test.file", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GUI Create Folder & file")
  {
    DeleteDirectory("sub", false);
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("New Folder", "sub");

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
      {"sub/bla", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GUI Create Folder & file fast")
  {
    DeleteDirectory("sub", false);
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder");
    Rename("New Folder", "sub");

    ExpectedEvent expectedEvents1[] = {
      {"New Folder", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    CreateFile("sub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/bla", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/bla");
    DeleteFile("sub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/bla", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
      {"sub/bla", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GUI Create Folder & file fast subdir")
  {
    DeleteDirectory("sub", false);

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("New Folder/subsub");
    Rename("New Folder", "sub");

    TickWatcher(watcher);

    CreateFile("sub/subsub/bla");

    ExpectedEvent expectedEvents2[] = {
      {"sub/subsub/bla", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    ModifyFile("sub/subsub/bla");
    DeleteFile("sub/subsub/bla");

    ExpectedEvent expectedEvents3[] = {
      {"sub/subsub/bla", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
      {"sub/subsub/bla", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);

    DeleteDirectory("sub");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GUI Delete Folder")
  {
    DeleteDirectory("sub2", false);
    DeleteDirectory("../sub2", false);

    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/file1", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    Rename("sub2", "../sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory},
      {"sub2/file1", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"sub2", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory},
    };
    // Issue here: After moving sub2 out of view, it remains in m_pathToWd
    CheckExpectedEvents(watcher, expectedEvents2);

    Rename("../sub2", "sub2");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/file1", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Create, Delete, Create")
  {
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Subdirectories)
                   .Succeeded());

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"sub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/file1", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents1);

    DeleteDirectory("sub2");

    ExpectedEvent expectedEvents2[] = {
      {"sub2/file1", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory},
      {"sub2", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::Directory},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents2);

    CreateDirectory("sub2/subsub2");
    CreateFile("sub2/file1");
    CreateFile("sub2/subsub2/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"sub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/file1", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
      {"sub2/subsub2", plDirectoryWatcherAction::Added, plDirectoryWatcherType::Directory},
      {"sub2/subsub2/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsUnordered(watcher, expectedEvents3);

    DeleteDirectory("sub2");
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "GUI Create file & delete")
  {
    DeleteDirectory("sub", false);
    plDirectoryWatcher watcher;
    PLASMA_TEST_BOOL(watcher.OpenDirectory(
                          sTestRootPath,
                          plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                            plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents1);

    Rename("file2.txt", "datei2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"file2.txt", plDirectoryWatcherAction::RenamedOldName, plDirectoryWatcherType::File},
      {"datei2.txt", plDirectoryWatcherAction::RenamedNewName, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents2);

    DeleteFile("datei2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"datei2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEvents(watcher, expectedEvents3);
  }

  PLASMA_TEST_BLOCK(plTestBlock::Enabled, "Enumerate multiple")
  {
    DeleteDirectory("watch1", false);
    DeleteDirectory("watch2", false);
    DeleteDirectory("watch3", false);
    plDirectoryWatcher watchers[3];

    plDirectoryWatcher* pWatchers[] = {watchers + 0, watchers + 1, watchers + 2};

    CreateDirectory("watch1");
    CreateDirectory("watch2");
    CreateDirectory("watch3");

    plStringBuilder watchPath;

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch1");
    PLASMA_TEST_BOOL(watchers[0].OpenDirectory(
                              watchPath,
                              plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                                plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch2");
    PLASMA_TEST_BOOL(watchers[1].OpenDirectory(
                              watchPath,
                              plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                                plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    watchPath = sTestRootPath;
    watchPath.AppendPath("watch3");
    PLASMA_TEST_BOOL(watchers[2].OpenDirectory(
                              watchPath,
                              plDirectoryWatcher::Watch::Creates | plDirectoryWatcher::Watch::Deletes |
                                plDirectoryWatcher::Watch::Writes | plDirectoryWatcher::Watch::Renames)
                   .Succeeded());

    CreateFile("watch1/file2.txt");

    ExpectedEvent expectedEvents1[] = {
      {"watch1/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents1);

    CreateFile("watch2/file2.txt");

    ExpectedEvent expectedEvents2[] = {
      {"watch2/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents2);

    CreateFile("watch3/file2.txt");

    ExpectedEvent expectedEvents3[] = {
      {"watch3/file2.txt", plDirectoryWatcherAction::Added, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents3);

    ModifyFile("watch1/file2.txt");
    ModifyFile("watch2/file2.txt");

    ExpectedEvent expectedEvents4[] = {
      {"watch1/file2.txt", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
      {"watch2/file2.txt", plDirectoryWatcherAction::Modified, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents4);

    DeleteFile("watch1/file2.txt");
    DeleteFile("watch2/file2.txt");
    DeleteFile("watch3/file2.txt");

    ExpectedEvent expectedEvents5[] = {
      {"watch1/file2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"watch2/file2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
      {"watch3/file2.txt", plDirectoryWatcherAction::Removed, plDirectoryWatcherType::File},
    };
    CheckExpectedEventsMultiple(pWatchers, expectedEvents5);
  }

  plOSFile::DeleteFolder(sTestRootPath).IgnoreResult();
}

#endif
