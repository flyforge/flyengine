#include <Utilities/UtilitiesPCH.h>

#include <Foundation/CodeUtils/Preprocessor.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/IO/FileSystem/FileSystem.h>
#include <Foundation/IO/OSFile.h>
#include <Utilities/Resources/ConfigFileResource.h>

static plConfigFileResourceLoader s_ConfigFileResourceLoader;

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(Utilties, ConfigFileResource)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plResourceManager::SetResourceTypeLoader<plConfigFileResource>(&s_ConfigFileResourceLoader);

    auto hFallback = plResourceManager::LoadResource<plConfigFileResource>("Empty.plConfig");
    plResourceManager::SetResourceTypeMissingFallback<plConfigFileResource>(hFallback);
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plResourceManager::SetResourceTypeMissingFallback<plConfigFileResource>(plConfigFileResourceHandle());
    plResourceManager::SetResourceTypeLoader<plConfigFileResource>(nullptr);
    plConfigFileResource::CleanupDynamicPluginReferences();
  }

  PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plConfigFileResource, 1, plRTTIDefaultAllocator<plConfigFileResource>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

PL_RESOURCE_IMPLEMENT_COMMON_CODE(plConfigFileResource);

plConfigFileResource::plConfigFileResource()
  : plResource(plResource::DoUpdate::OnAnyThread, 0)
{
}

plConfigFileResource::~plConfigFileResource() = default;

plInt32 plConfigFileResource::GetInt(plTempHashedString sName, plInt32 iFallback) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return iFallback;
}

plInt32 plConfigFileResource::GetInt(plTempHashedString sName) const
{
  auto it = m_IntData.Find(sName);
  if (it.IsValid())
    return it.Value();

  plLog::Error("{}: 'int' config variable (name hash = {}) doesn't exist.", this->GetResourceIdOrDescription(), sName.GetHash());
  return 0;
}

float plConfigFileResource::GetFloat(plTempHashedString sName, float fFallback) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return fFallback;
}

float plConfigFileResource::GetFloat(plTempHashedString sName) const
{
  auto it = m_FloatData.Find(sName);
  if (it.IsValid())
    return it.Value();

  plLog::Error("{}: 'float' config variable (name hash = {}) doesn't exist.", this->GetResourceIdOrDescription(), sName.GetHash());
  return 0;
}

bool plConfigFileResource::GetBool(plTempHashedString sName, bool bFallback) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return bFallback;
}

bool plConfigFileResource::GetBool(plTempHashedString sName) const
{
  auto it = m_BoolData.Find(sName);
  if (it.IsValid())
    return it.Value();

  plLog::Error("{}: 'float' config variable (name hash = {}) doesn't exist.", this->GetResourceIdOrDescription(), sName.GetHash());
  return false;
}

plStringView plConfigFileResource::GetString(plTempHashedString sName, plStringView sFallback) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  return sFallback;
}

plStringView plConfigFileResource::GetString(plTempHashedString sName) const
{
  auto it = m_StringData.Find(sName);
  if (it.IsValid())
    return it.Value();

  plLog::Error("{}: 'string' config variable '(name hash = {}) doesn't exist.", this->GetResourceIdOrDescription(), sName.GetHash());
  return "";
}

plResourceLoadDesc plConfigFileResource::UnloadData(Unload WhatToUnload)
{
  m_IntData.Clear();
  m_FloatData.Clear();
  m_StringData.Clear();
  m_BoolData.Clear();

  plResourceLoadDesc d;
  d.m_State = plResourceState::Unloaded;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  return d;
}

plResourceLoadDesc plConfigFileResource::UpdateContent(plStreamReader* Stream)
{
  plResourceLoadDesc d;
  d.m_uiQualityLevelsDiscardable = 0;
  d.m_uiQualityLevelsLoadable = 0;
  d.m_State = plResourceState::Loaded;

  if (Stream == nullptr)
  {
    d.m_State = plResourceState::LoadedResourceMissing;
    return d;
  }

  m_RequiredFiles.ReadDependencyFile(*Stream).IgnoreResult();
  Stream->ReadHashTable(m_IntData).IgnoreResult();
  Stream->ReadHashTable(m_FloatData).IgnoreResult();
  Stream->ReadHashTable(m_StringData).IgnoreResult();
  Stream->ReadHashTable(m_BoolData).IgnoreResult();

  return d;
}

void plConfigFileResource::UpdateMemoryUsage(MemoryUsage& out_NewMemoryUsage)
{
  out_NewMemoryUsage.m_uiMemoryCPU = m_IntData.GetHeapMemoryUsage() + m_FloatData.GetHeapMemoryUsage() + m_StringData.GetHeapMemoryUsage() + m_BoolData.GetHeapMemoryUsage();
  out_NewMemoryUsage.m_uiMemoryGPU = 0;
}

//////////////////////////////////////////////////////////////////////////

plResult plConfigFileResourceLoader::LoadedData::PrePropFileLocator(plStringView sCurAbsoluteFile, plStringView sIncludeFile, plPreprocessor::IncludeType incType, plStringBuilder& out_sAbsoluteFilePath)
{
  plResult res = plPreprocessor::DefaultFileLocator(sCurAbsoluteFile, sIncludeFile, incType, out_sAbsoluteFilePath);

  m_RequiredFiles.AddFileDependency(out_sAbsoluteFilePath);

  return res;
}

plResourceLoadData plConfigFileResourceLoader::OpenDataStream(const plResource* pResource)
{
  PL_PROFILE_SCOPE("ReadResourceFile");
  PL_LOG_BLOCK("Load Config Resource", pResource->GetResourceID());

  plStringBuilder sConfig;

  plMap<plString, plInt32> intData;
  plMap<plString, float> floatData;
  plMap<plString, plString> stringData;
  plMap<plString, bool> boolData;

  LoadedData* pData = PL_DEFAULT_NEW(LoadedData);
  pData->m_Reader.SetStorage(&pData->m_Storage);

  plPreprocessor preprop;

  // used to gather all the transitive file dependencies
  preprop.SetFileLocatorFunction(plMakeDelegate(&plConfigFileResourceLoader::LoadedData::PrePropFileLocator, pData));

  if (plStringUtils::IsEqual(pResource->GetResourceID(), "Empty.plConfig"))
  {
    // do nothing
  }
  else if (preprop.Process(pResource->GetResourceID(), sConfig, false, true, false).Succeeded())
  {
    sConfig.ReplaceAll("\r", "");
    sConfig.ReplaceAll("\n", ";");

    plHybridArray<plStringView, 32> lines;
    sConfig.Split(false, lines, ";");

    plStringBuilder key, value, line;

    for (plStringView tmp : lines)
    {
      line = tmp;
      line.Trim(" \t");

      if (line.IsEmpty())
        continue;

      const char* szAssign = line.FindSubString("=");

      if (szAssign == nullptr)
      {
        plLog::Error("Invalid line in config file: '{}'", tmp);
      }
      else
      {
        value = szAssign + 1;
        value.Trim(" ");

        line.SetSubString_FromTo(line.GetData(), szAssign);
        line.ReplaceAll("\t", " ");
        line.ReplaceAll("  ", " ");
        line.Trim(" ");

        const bool bOverride = line.TrimWordStart("override ");
        line.Trim(" ");

        if (line.StartsWith("int "))
        {
          key.SetSubString_FromTo(line.GetData() + 4, szAssign);
          key.Trim(" ");

          if (bOverride && !intData.Contains(key))
            plLog::Error("Config 'int' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && intData.Contains(key))
            plLog::Error("Config 'int' key '{}' is not marked override, but exist already. Use 'override int' instead.", key);

          plInt32 val;
          if (plConversionUtils::StringToInt(value, val).Succeeded())
          {
            intData[key] = val;
          }
          else
          {
            plLog::Error("Failed to parse 'int' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("float "))
        {
          key.SetSubString_FromTo(line.GetData() + 6, szAssign);
          key.Trim(" ");

          if (bOverride && !floatData.Contains(key))
            plLog::Error("Config 'float' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && floatData.Contains(key))
            plLog::Error("Config 'float' key '{}' is not marked override, but exist already. Use 'override float' instead.", key);

          double val;
          if (plConversionUtils::StringToFloat(value, val).Succeeded())
          {
            floatData[key] = (float)val;
          }
          else
          {
            plLog::Error("Failed to parse 'float' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("bool "))
        {
          key.SetSubString_FromTo(line.GetData() + 5, szAssign);
          key.Trim(" ");

          if (bOverride && !boolData.Contains(key))
            plLog::Error("Config 'bool' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && boolData.Contains(key))
            plLog::Error("Config 'bool' key '{}' is not marked override, but exist already. Use 'override bool' instead.", key);

          bool val;
          if (plConversionUtils::StringToBool(value, val).Succeeded())
          {
            boolData[key] = val;
          }
          else
          {
            plLog::Error("Failed to parse 'bool' in config file: '{}'", tmp);
          }
        }
        else if (line.StartsWith("string "))
        {
          key.SetSubString_FromTo(line.GetData() + 7, szAssign);
          key.Trim(" ");

          if (bOverride && !stringData.Contains(key))
            plLog::Error("Config 'string' key '{}' is marked override, but doesn't exist yet. Remove 'override' keyword.", key);
          if (!bOverride && stringData.Contains(key))
            plLog::Error("Config 'string' key '{}' is not marked override, but exist already. Use 'override string' instead.", key);

          if (!value.StartsWith("\"") || !value.EndsWith("\""))
          {
            plLog::Error("Failed to parse 'string' in config file: '{}'", tmp);
          }
          else
          {
            value.Shrink(1, 1);
            stringData[key] = value;
          }
        }
        else
        {
          plLog::Error("Invalid line in config file: '{}'", tmp);
        }
      }
    }
  }
  else
  {
    // empty stream
    return {};
  }

  plResourceLoadData res;
  res.m_pDataStream = &pData->m_Reader;
  res.m_pCustomLoaderData = pData;

#if PL_ENABLED(PL_SUPPORTS_FILE_STATS)
  plFileStats stat;
  if (plFileSystem::GetFileStats(pResource->GetResourceID(), stat).Succeeded())
  {
    res.m_sResourceDescription = stat.m_sName;
    res.m_LoadedFileModificationDate = stat.m_LastModificationTime;
  }
#endif

  plMemoryStreamWriter writer(&pData->m_Storage);

  pData->m_RequiredFiles.StoreCurrentTimeStamp();
  pData->m_RequiredFiles.WriteDependencyFile(writer).IgnoreResult();
  writer.WriteMap(intData).IgnoreResult();
  writer.WriteMap(floatData).IgnoreResult();
  writer.WriteMap(stringData).IgnoreResult();
  writer.WriteMap(boolData).IgnoreResult();

  return res;
}

void plConfigFileResourceLoader::CloseDataStream(const plResource* pResource, const plResourceLoadData& loaderData)
{
  LoadedData* pData = static_cast<LoadedData*>(loaderData.m_pCustomLoaderData);

  PL_DEFAULT_DELETE(pData);
}

bool plConfigFileResourceLoader::IsResourceOutdated(const plResource* pResource) const
{
  return static_cast<const plConfigFileResource*>(pResource)->m_RequiredFiles.HasAnyFileChanged();
}


PL_STATICLINK_FILE(Utilities, Utilities_Resources_ConfigFileResource);
