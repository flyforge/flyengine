#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetTableWriter.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileSystem.h>

plResult plAssetTable::WriteAssetTable()
{
  PLASMA_PROFILE_SCOPE("WriteAssetTable");

  plStringBuilder sTemp;
  plString sResourcePath;

  {
    for (auto& man : plAssetDocumentManager::GetAllDocumentManagers())
    {
      if (!man->GetDynamicRTTI()->IsDerivedFrom<plAssetDocumentManager>())
        continue;

      plAssetDocumentManager* pManager = static_cast<plAssetDocumentManager*>(man);

      // allow to add fully custom entries
      pManager->AddEntriesToAssetTable(m_sDataDir, m_pProfile, plMakeDelegate(&plAssetTable::AddManagerResource, this));
    }
  }

  if (m_bReset)
  {
    m_GuidToPath.Clear();
    plAssetCurator::plLockedSubAssetTable allSubAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();

    for (auto it = allSubAssetsLocked->GetIterator(); it.IsValid(); ++it)
    {
      sTemp = it.Value().m_pAssetInfo->m_Path.GetAbsolutePath();

      // ignore all assets that are not located in this data directory
      if (!sTemp.IsPathBelowFolder(m_sDataDir))
        continue;

      Update(it.Value());
    }
    m_bReset = false;
  }

  // We don't write anything on a background process as the main editor process will have already written any dirty tables before sending an RPC request. We still want to engine process to reload any potential changes though and be able to check which resources to reload so the tables are kept up to date in memory.
  if (plQtEditorApp::GetSingleton()->IsBackgroundMode())
    return PLASMA_SUCCESS;

  plDeferredFileWriter file;
  file.SetOutput(m_sTargetFile);

  auto Write = [](const plString& sGuid, const plString& sPath, plDeferredFileWriter& ref_file) {
    ref_file.WriteBytes(sGuid.GetData(), sGuid.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes(";", 1).IgnoreResult();
    ref_file.WriteBytes(sPath.GetData(), sPath.GetElementCount()).IgnoreResult();
    ref_file.WriteBytes("\n", 1).IgnoreResult();
  };

  for (auto it = m_GuidToManagerResource.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value().m_sPath, file);
  }

  for (auto it = m_GuidToPath.GetIterator(); it.IsValid(); ++it)
  {
    Write(it.Key(), it.Value(), file);
  }

  if (file.Close().Failed())
  {
    plLog::Error("Failed to open asset lookup table file '{0}'", m_sTargetFile);
    return PLASMA_FAILURE;
  }

  m_bDirty = false;
  return PLASMA_SUCCESS;
}

void plAssetTable::Remove(const plSubAsset& subAsset)
{
  plStringBuilder sTemp;
  plConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);
  m_GuidToPath.Remove(sTemp);
  m_bDirty = true;
}

void plAssetTable::Update(const plSubAsset& subAsset)
{
  plStringBuilder sTemp;
  plAssetDocumentManager* pManager = subAsset.m_pAssetInfo->GetManager();
  plString sEntry = pManager->GetAssetTableEntry(&subAsset, m_sDataDir, m_pProfile);

  // It is valid to write no asset table entry, if no redirection is required. This is used by decal assets for instance.
  if (!sEntry.IsEmpty())
  {
    plConversionUtils::ToString(subAsset.m_Data.m_Guid, sTemp);

    m_GuidToPath[sTemp] = sEntry;
  }
  m_bDirty = true;
}

void plAssetTable::AddManagerResource(plStringView sGuid, plStringView sPath, plStringView sType)
{
  m_GuidToManagerResource[sGuid] = ManagerResource{sPath, sType};
}

plAssetTableWriter::plAssetTableWriter(const plApplicationFileSystemConfig& fileSystemConfig)
{
  m_FileSystemConfig = fileSystemConfig;
  m_DataDirToAssetTables.SetCount(m_FileSystemConfig.m_DataDirs.GetCount());

  plStringBuilder sDataDirPath;

  m_DataDirRoots.Reserve(m_FileSystemConfig.m_DataDirs.GetCount());
  for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
  {
    if (plFileSystem::ResolveSpecialDirectory(m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath, sDataDirPath).Failed())
    {
      plLog::Error("Failed to resolve data directory named '{}' at '{}'", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
      m_DataDirRoots.PushBack({});
    }
    else
    {
      m_DataDirRoots.PushBack(sDataDirPath);
    }
  }

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plAssetTableWriter::AssetCuratorEvents, this));
}

plAssetTableWriter::~plAssetTableWriter()
{
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plAssetTableWriter::AssetCuratorEvents, this));
}

void plAssetTableWriter::MainThreadTick()
{
  // We must flush any pending table changes before triggering resource reloads.
  // If no resource reload is scheduled, we can just wait for the timer to run out to flush the changes.
  //
  if (m_bTablesDirty && (plTime::Now() > m_NextTableFlush || m_bNeedToReloadResources))
  {
    m_bTablesDirty = false;
    if (WriteAssetTables(plAssetCurator::GetSingleton()->GetActiveAssetProfile(), false).Failed())
    {
      plLog::Error("Failed to write asset tables");
    }
  }

  if (m_bNeedToReloadResources)
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = plAssetCurator::GetSingleton()->GetKnownSubAssets();
    PLASMA_LOCK(m_AssetTableMutex);

    bool bReloadManagerResources = false;
    const plPlatformProfile* pCurrentProfile = plAssetCurator::GetSingleton()->GetActiveAssetProfile();
    for (const ReloadResource& reload : m_ReloadResources)
    {
      if (plAssetTable* pTable = GetAssetTable(reload.m_uiDataDirIndex, pCurrentProfile))
      {
        if (pTable->m_GuidToPath.Contains(reload.m_sResource))
        {
          plReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = reload.m_sResource;
          msg2.m_sResourceType = reload.m_sType;
          plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
        else
        {
          // If an asset is not represented by a resource in the table we assume it is represented by a manager resource.
          // Currently we don't know how these relate, e.g. we don't know all "Decal" assets are represented by the "{ ProjectDecalAtlas }" resource. Therefore, we just reload all manager resources.
          bReloadManagerResources = true;
        }
      }
    }
    m_ReloadResources.Clear();

    if (bReloadManagerResources)
    {
      for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        plAssetTable* pTable = GetAssetTable(i, pCurrentProfile);
        for (auto it : pTable->m_GuidToManagerResource)
        {
          plReloadResourceMsgToEngine msg2;
          msg2.m_sResourceID = it.Key();
          msg2.m_sResourceType = it.Value().m_sType;
          plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg2);
        }
      }
    }

    plSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadResources";
    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
    m_bNeedToReloadResources = false;
  }
}

void plAssetTableWriter::NeedsReloadResource(const plUuid& assetGuid)
{
  plAssetCurator::plLockedSubAsset asset = plAssetCurator::GetSingleton()->GetSubAsset(assetGuid);
  if (asset.isValid())
  {
    PLASMA_LOCK(m_AssetTableMutex);
    m_bNeedToReloadResources = true;
    plString sDocType = asset->m_Data.m_sSubAssetsDocumentTypeName.GetString();
    plStringBuilder sGuid;
    plConversionUtils::ToString(assetGuid, sGuid);
    const plUInt32 uiDataDirIndex = FindDataDir(*asset);
    m_ReloadResources.PushBack({uiDataDirIndex, sGuid, sDocType});
  }
}

plResult plAssetTableWriter::WriteAssetTables(const plPlatformProfile* pAssetProfile, bool bForce)
{
  CURATOR_PROFILE("WriteAssetTables");
  PLASMA_LOG_BLOCK("plAssetCurator::WriteAssetTables");
  PLASMA_ASSERT_DEV(pAssetProfile != nullptr, "WriteAssetTables: pAssetProfile must be set.");

  plResult res = PLASMA_SUCCESS;
  bool bAnyChanged = false;
  {
    // We need to lock the curator first because that lock is hold when AssetCuratorEvents are called.
    auto lock = plAssetCurator::GetSingleton()->GetKnownSubAssets();
    PLASMA_LOCK(m_AssetTableMutex);

    plStringBuilder sd;

    for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
    {
      plAssetTable* table = GetAssetTable(i, pAssetProfile);
      if (!table)
      {
        plLog::Error("WriteAssetTables: The data dir '{}' with path '{}' could not be resolved", m_FileSystemConfig.m_DataDirs[i].m_sRootName, m_FileSystemConfig.m_DataDirs[i].m_sDataDirSpecialPath);
        res = PLASMA_FAILURE;
        continue;
      }

      bAnyChanged |= (table->m_bReset || table->m_bDirty);
      if (!bForce && !table->m_bDirty && !table->m_bReset)
        continue;

      if (table->WriteAssetTable().Failed())
        res = PLASMA_FAILURE;
    }
  }

  if (bAnyChanged && pAssetProfile == plAssetCurator::GetSingleton()->GetActiveAssetProfile())
  {
    plSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ReloadAssetLUT";
    msg.m_sPayload = pAssetProfile->GetConfigName();
    plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }

  m_NextTableFlush = plTime::Now() + plTime::MakeFromSeconds(1.5);
  return res;
}

void plAssetTableWriter::AssetCuratorEvents(const plAssetCuratorEvent& e)
{
  PLASMA_LOCK(m_AssetTableMutex);

  const plPlatformProfile* pProfile = plAssetCurator::GetSingleton()->GetActiveAssetProfile();
  switch (e.m_Type)
  {
    //#TODO Are asset table entries static or do they change with the asset?
    /*case plAssetCuratorEvent::Type::AssetUpdated:
      if (e.m_pInfo->m_pAssetInfo->m_TransformState == plAssetInfo::TransformState::Unknown)
        return;
      [[fallthrough]];*/
    case plAssetCuratorEvent::Type::AssetAdded:
    case plAssetCuratorEvent::Type::AssetMoved:
    {
      plUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (plAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Update(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case plAssetCuratorEvent::Type::AssetRemoved:
    {
      plUInt32 uiDataDirIndex = FindDataDir(*e.m_pInfo);
      if (plAssetTable* pTable = GetAssetTable(uiDataDirIndex, pProfile))
      {
        pTable->Remove(*e.m_pInfo);
        m_bTablesDirty = true;
      }
    }
    break;
    case plAssetCuratorEvent::Type::AssetListReset:
      for (plUInt32 i = 0; i < m_FileSystemConfig.m_DataDirs.GetCount(); ++i)
      {
        for (auto it : m_DataDirToAssetTables[i])
        {
          it.Value()->m_bReset = true;
        }
      }
      m_bTablesDirty = true;
      break;
    case plAssetCuratorEvent::Type::ActivePlatformChanged:
      if (WriteAssetTables(pProfile, false).Failed())
      {
        plLog::Error("Failed to write asset tables");
      }
      break;
    default:
      break;
  }
}

plAssetTable* plAssetTableWriter::GetAssetTable(plUInt32 uiDataDirIndex, const plPlatformProfile* pAssetProfile)
{
  auto it = m_DataDirToAssetTables[uiDataDirIndex].Find(pAssetProfile);
  if (!it.IsValid())
  {
    if (m_DataDirRoots[uiDataDirIndex].IsEmpty())
      return nullptr;

    plUniquePtr<plAssetTable> table = PLASMA_DEFAULT_NEW(plAssetTable);
    table->m_pProfile = pAssetProfile;
    table->m_sDataDir = m_DataDirRoots[uiDataDirIndex];

    plStringBuilder sFinalPath(m_DataDirRoots[uiDataDirIndex], "/AssetCache/", pAssetProfile->GetConfigName(), ".plAidlt");
    sFinalPath.MakeCleanPath();
    table->m_sTargetFile = sFinalPath;

    it = m_DataDirToAssetTables[uiDataDirIndex].Insert(pAssetProfile, std::move(table));
  }
  return it.Value().Borrow();
}

plUInt32 plAssetTableWriter::FindDataDir(const plSubAsset& asset)
{
  return asset.m_pAssetInfo->m_Path.GetDataDirIndex();
}
