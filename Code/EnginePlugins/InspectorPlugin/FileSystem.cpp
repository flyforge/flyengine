#include <InspectorPlugin/InspectorPluginPCH.h>

#include <Foundation/Communication/Telemetry.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

static plInt32 s_iDataDirCounter = 0;
static plMap<plString, plInt32, plCompareHelper<plString>, plStaticAllocatorWrapper> s_KnownDataDirs;

static void FileSystemEventHandler(const plFileSystem::FileEvent& e)
{
  switch (e.m_EventType)
  {
    case plFileSystem::FileEventType::AddDataDirectorySucceeded:
    {
      bool bExisted = false;
      auto it = s_KnownDataDirs.FindOrAdd(e.m_sFileOrDirectory, &bExisted);

      if (!bExisted)
      {
        it.Value() = s_iDataDirCounter;
        ++s_iDataDirCounter;
      }

      plStringBuilder sName;
      sName.Format("IO/DataDirs/Dir{0}", plArgI(it.Value(), 2, true));

      plStats::SetStat(sName.GetData(), e.m_sFileOrDirectory);
    }
    break;

    case plFileSystem::FileEventType::RemoveDataDirectory:
    {
      auto it = s_KnownDataDirs.Find(e.m_sFileOrDirectory);

      if (!it.IsValid())
        break;

      plStringBuilder sName;
      sName.Format("IO/DataDirs/Dir{0}", plArgI(it.Value(), 2, true));

      plStats::RemoveStat(sName.GetData());
    }
    break;

    default:
      break;
  }
}

void AddFileSystemEventHandler()
{
  plFileSystem::RegisterEventHandler(FileSystemEventHandler);
}

void RemoveFileSystemEventHandler()
{
  plFileSystem::UnregisterEventHandler(FileSystemEventHandler);
}
