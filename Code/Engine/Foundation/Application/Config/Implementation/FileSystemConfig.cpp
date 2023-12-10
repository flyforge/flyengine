#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plApplicationFileSystemConfig, plNoBase, 1, plRTTIDefaultAllocator<plApplicationFileSystemConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("DataDirs", m_DataDirs),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plApplicationFileSystemConfig_DataDirConfig, plNoBase, 1, plRTTIDefaultAllocator<plApplicationFileSystemConfig_DataDirConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RelativePath", m_sDataDirSpecialPath),
    PLASMA_MEMBER_PROPERTY("Writable", m_bWritable),
    PLASMA_MEMBER_PROPERTY("RootName", m_sRootName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plResult plApplicationFileSystemConfig::Save(plStringView sPath)
{
  plFileWriter file;
  if (file.Open(sPath).Failed())
    return PLASMA_FAILURE;

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  for (plUInt32 i = 0; i < m_DataDirs.GetCount(); ++i)
  {
    writer.BeginObject("DataDir");

    plOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sDataDirSpecialPath, "Path");
    plOpenDdlUtils::StoreString(writer, m_DataDirs[i].m_sRootName, "RootName");
    plOpenDdlUtils::StoreBool(writer, m_DataDirs[i].m_bWritable, "Writable");

    writer.EndObject();
  }

  return PLASMA_SUCCESS;
}

void plApplicationFileSystemConfig::Load(plStringView sPath)
{
  PLASMA_LOG_BLOCK("plApplicationFileSystemConfig::Load()");

  m_DataDirs.Clear();

#if PLASMA_ENABLED(PLASMA_MIGRATE_RUNTIMECONFIGS)
  plStringBuilder sOldLoc;
  if (sPath.FindSubString("RuntimeConfigs/"))
  {
    sOldLoc = sPath;
    sOldLoc.ReplaceLast("RuntimeConfigs/", "");
    sPath = plFileSystem::MigrateFileLocation(sOldLoc, sPath);
  }
#endif

  plFileReader file;
  if (file.Open(sPath).Failed())
  {
    plLog::Dev("File-system config file '{0}' does not exist.", sPath);
    return;
  }

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse file-system config file '{0}'", sPath);
    return;
  }

  const plOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pDirs = pTree->GetFirstChild(); pDirs != nullptr; pDirs = pDirs->GetSibling())
  {
    if (!pDirs->IsCustomType("DataDir"))
      continue;

    DataDirConfig cfg;
    cfg.m_bWritable = false;

    const plOpenDdlReaderElement* pPath = pDirs->FindChildOfType(plOpenDdlPrimitiveType::String, "Path");
    const plOpenDdlReaderElement* pRoot = pDirs->FindChildOfType(plOpenDdlPrimitiveType::String, "RootName");
    const plOpenDdlReaderElement* pWrite = pDirs->FindChildOfType(plOpenDdlPrimitiveType::Bool, "Writable");

    if (pPath)
      cfg.m_sDataDirSpecialPath = pPath->GetPrimitivesString()[0];
    if (pRoot)
      cfg.m_sRootName = pRoot->GetPrimitivesString()[0];
    if (pWrite)
      cfg.m_bWritable = pWrite->GetPrimitivesBool()[0];

    /// \todo Temp fix for backwards compatibility
    {
      if (cfg.m_sRootName == "project")
      {
        cfg.m_sDataDirSpecialPath = ">project/";
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":project/"))
      {
        plStringBuilder temp(">project/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 9);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (cfg.m_sDataDirSpecialPath.StartsWith_NoCase(":sdk/"))
      {
        plStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath.GetData() + 5);
        cfg.m_sDataDirSpecialPath = temp;
      }
      else if (!cfg.m_sDataDirSpecialPath.StartsWith_NoCase(">sdk/"))
      {
        plStringBuilder temp(">sdk/");
        temp.AppendPath(cfg.m_sDataDirSpecialPath);
        cfg.m_sDataDirSpecialPath = temp;
      }
    }

    m_DataDirs.PushBack(cfg);
  }
}

void plApplicationFileSystemConfig::Apply()
{
  PLASMA_LOG_BLOCK("plApplicationFileSystemConfig::Apply");

  // plStringBuilder s;

  // Make sure previous calls to Apply do not accumulate
  Clear();

  for (const auto& var : m_DataDirs)
  {
    // if (plFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Succeeded())
    {
      plFileSystem::AddDataDirectory(var.m_sDataDirSpecialPath, "AppFileSystemConfig", var.m_sRootName, (!var.m_sRootName.IsEmpty() && var.m_bWritable) ? plFileSystem::DataDirUsage::AllowWrites : plFileSystem::DataDirUsage::ReadOnly).IgnoreResult();
    }
  }
}


void plApplicationFileSystemConfig::Clear()
{
  plFileSystem::RemoveDataDirectoryGroup("AppFileSystemConfig");
}

plResult plApplicationFileSystemConfig::CreateDataDirStubFiles()
{
  PLASMA_LOG_BLOCK("plApplicationFileSystemConfig::CreateDataDirStubFiles");

  plStringBuilder s;
  plResult res = PLASMA_SUCCESS;

  for (const auto& var : m_DataDirs)
  {
    if (plFileSystem::ResolveSpecialDirectory(var.m_sDataDirSpecialPath, s).Failed())
    {
      plLog::Error("Failed to get special directory '{0}'", var.m_sDataDirSpecialPath);
      res = PLASMA_FAILURE;
      continue;
    }

    s.AppendPath("DataDir.plManifest");

    plOSFile file;
    if (file.Open(s, plFileOpenMode::Write).Failed())
    {
      plLog::Error("Failed to create stub file '{0}'", s);
      res = PLASMA_FAILURE;
    }
  }

  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_FileSystemConfig);
