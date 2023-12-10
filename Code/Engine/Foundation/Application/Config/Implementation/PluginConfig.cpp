#include <Foundation/FoundationPCH.h>

#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plApplicationPluginConfig, plNoBase, 1, plRTTIDefaultAllocator<plApplicationPluginConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Plugins", m_Plugins),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plApplicationPluginConfig_PluginConfig, plNoBase, 1, plRTTIDefaultAllocator<plApplicationPluginConfig_PluginConfig>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("RelativePath", m_sAppDirRelativePath),
    PLASMA_MEMBER_PROPERTY("LoadCopy", m_bLoadCopy),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

bool plApplicationPluginConfig::PluginConfig::operator<(const PluginConfig& rhs) const
{
  return m_sAppDirRelativePath < rhs.m_sAppDirRelativePath;
}

bool plApplicationPluginConfig::AddPlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (plUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      return false;
    }
  }

  m_Plugins.PushBack(cfg);
  return true;
}

bool plApplicationPluginConfig::RemovePlugin(const PluginConfig& cfg0)
{
  PluginConfig cfg = cfg0;

  for (plUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    if (m_Plugins[i].m_sAppDirRelativePath == cfg.m_sAppDirRelativePath)
    {
      m_Plugins.RemoveAtAndSwap(i);
      return true;
    }
  }

  return false;
}

plApplicationPluginConfig::plApplicationPluginConfig() = default;

plResult plApplicationPluginConfig::Save(plStringView sPath) const
{
  m_Plugins.Sort();

  plDeferredFileWriter file;
  file.SetOutput(sPath, true);

  plOpenDdlWriter writer;
  writer.SetOutputStream(&file);
  writer.SetCompactMode(false);
  writer.SetPrimitiveTypeStringMode(plOpenDdlWriter::TypeStringMode::Compliant);

  for (plUInt32 i = 0; i < m_Plugins.GetCount(); ++i)
  {
    writer.BeginObject("Plugin");

    plOpenDdlUtils::StoreString(writer, m_Plugins[i].m_sAppDirRelativePath, "Path");
    plOpenDdlUtils::StoreBool(writer, m_Plugins[i].m_bLoadCopy, "LoadCopy");

    writer.EndObject();
  }

  return file.Close();
}

void plApplicationPluginConfig::Load(plStringView sPath)
{
  PLASMA_LOG_BLOCK("plApplicationPluginConfig::Load()");

  m_Plugins.Clear();

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
    plLog::Warning("Could not open plugins config file '{0}'", sPath);
    return;
  }

  plOpenDdlReader reader;
  if (reader.ParseDocument(file, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse plugins config file '{0}'", sPath);
    return;
  }

  const plOpenDdlReaderElement* pTree = reader.GetRootElement();

  for (const plOpenDdlReaderElement* pPlugin = pTree->GetFirstChild(); pPlugin != nullptr; pPlugin = pPlugin->GetSibling())
  {
    if (!pPlugin->IsCustomType("Plugin"))
      continue;

    PluginConfig cfg;

    const plOpenDdlReaderElement* pPath = pPlugin->FindChildOfType(plOpenDdlPrimitiveType::String, "Path");
    const plOpenDdlReaderElement* pCopy = pPlugin->FindChildOfType(plOpenDdlPrimitiveType::Bool, "LoadCopy");

    if (pPath)
    {
      cfg.m_sAppDirRelativePath = pPath->GetPrimitivesString()[0];
    }

    if (pCopy)
    {
      cfg.m_bLoadCopy = pCopy->GetPrimitivesBool()[0];
    }

    // this prevents duplicates
    AddPlugin(cfg);
  }
}

void plApplicationPluginConfig::Apply()
{
  PLASMA_LOG_BLOCK("plApplicationPluginConfig::Apply");

  for (const auto& var : m_Plugins)
  {
    plBitflags<plPluginLoadFlags> flags;
    flags.AddOrRemove(plPluginLoadFlags::LoadCopy, var.m_bLoadCopy);
    flags.AddOrRemove(plPluginLoadFlags::CustomDependency, false);

    plPlugin::LoadPlugin(var.m_sAppDirRelativePath, flags).IgnoreResult();
  }
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Application_Config_Implementation_PluginConfig);
