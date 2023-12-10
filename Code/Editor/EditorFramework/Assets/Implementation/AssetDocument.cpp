#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Texture/Image/ImageConversion.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAssetDocument, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plAssetDocument::plAssetDocument(plStringView sDocumentPath, plDocumentObjectManager* pObjectManager, plAssetDocEngineConnection engineConnectionType)
  : plDocument(sDocumentPath, pObjectManager)
{
  m_EngineConnectionType = engineConnectionType;
  m_EngineStatus = (m_EngineConnectionType != plAssetDocEngineConnection::None) ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_pEngineConnection = nullptr;
  m_uiCommonAssetStateFlags = plCommonAssetUiState::Grid | plCommonAssetUiState::Loop | plCommonAssetUiState::Visualizers;

  if (m_EngineConnectionType != plAssetDocEngineConnection::None)
  {
    plEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(plMakeDelegate(&plAssetDocument::EngineConnectionEventHandler, this));
  }
}

plAssetDocument::~plAssetDocument()
{
  m_pMirror->DeInit();

  if (m_EngineConnectionType != plAssetDocEngineConnection::None)
  {
    plEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(plMakeDelegate(&plAssetDocument::EngineConnectionEventHandler, this));

    if (m_pEngineConnection)
    {
      plEditorEngineProcessConnection::GetSingleton()->DestroyEngineConnection(this);
    }
  }
}

void plAssetDocument::SetCommonAssetUiState(plCommonAssetUiState::Enum state, double value)
{
  if (value == 0)
  {
    m_uiCommonAssetStateFlags &= ~((plUInt32)state);
  }
  else
  {
    m_uiCommonAssetStateFlags |= (plUInt32)state;
  }

  plCommonAssetUiState e;
  e.m_State = state;
  e.m_fValue = value;

  m_CommonAssetUiChangeEvent.Broadcast(e);
}

double plAssetDocument::GetCommonAssetUiState(plCommonAssetUiState::Enum state) const
{
  return (m_uiCommonAssetStateFlags & (plUInt32)state) != 0 ? 1.0f : 0.0f;
}

plAssetDocumentManager* plAssetDocument::GetAssetDocumentManager() const
{
  return static_cast<plAssetDocumentManager*>(GetDocumentManager());
}

const plAssetDocumentInfo* plAssetDocument::GetAssetDocumentInfo() const
{
  return static_cast<plAssetDocumentInfo*>(m_pDocumentInfo);
}

plBitflags<plAssetDocumentFlags> plAssetDocument::GetAssetFlags() const
{
  return GetAssetDocumentTypeDescriptor()->m_AssetDocumentFlags;
}

plDocumentInfo* plAssetDocument::CreateDocumentInfo()
{
  return PLASMA_DEFAULT_NEW(plAssetDocumentInfo);
}

plTaskGroupID plAssetDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  plAssetDocumentInfo* pInfo = static_cast<plAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_TransformDependencies.Clear();
  pInfo->m_ThumbnailDependencies.Clear();
  pInfo->m_PackageDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetsDocumentTypeName.Assign(GetDocumentTypeName());
  pInfo->ClearMetaData();
  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_TransformDependencies.Remove(plString());
  pInfo->m_ThumbnailDependencies.Remove(plString());
  pInfo->m_PackageDependencies.Remove(plString());

  return plDocument::InternalSaveDocument(callback);
}

void plAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();
  plAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());
  plAssetCurator::GetSingleton()->MainThreadTick(false);

  if (flags.IsAnySet(plAssetDocumentFlags::AutoTransformOnSave))
  {
    // If we request an engine connection but the mirror is not set up yet we are still
    // creating the document and TransformAsset will most likely fail.
    if (m_EngineConnectionType == plAssetDocEngineConnection::None || m_pEngineConnection)
    {
      plUuid docGuid = GetGuid();

      plSharedPtr<plDelegateTask<void>> pTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "TransformAfterSaveDocument", plTaskNesting::Never, [docGuid]() {
        plDocument* pDoc = plDocumentManager::GetDocumentByGuid(docGuid);
        if (pDoc == nullptr)
          return;

        /// \todo Should only be done for platform agnostic assets
        plTransformStatus ret = plAssetCurator::GetSingleton()->TransformAsset(docGuid, plTransformFlags::TriggeredManually);

        if (ret.Failed())
        {
          plLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, pDoc->GetDocumentPath());
        }
        else
        {
          plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
        }
        //
      });

      pTask->ConfigureTask("TransformAfterSaveDocument", plTaskNesting::Maybe);
      plTaskSystem::StartSingleTask(pTask, plTaskPriority::ThisFrameMainThread);
    }
  }
}

void plAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = PLASMA_DEFAULT_NEW(plIPCObjectMirrorEditor);
}

void plAssetDocument::InitializeAfterLoadingAndSaving()
{
  if (m_EngineConnectionType != plAssetDocEngineConnection::None)
  {
    m_pEngineConnection = plEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus = EngineStatus::Initializing;

    if (m_EngineConnectionType == plAssetDocEngineConnection::FullObjectMirroring)
    {
      m_pMirror->SetIPC(m_pEngineConnection);
      m_pMirror->InitSender(GetObjectManager());
    }
  }
}

void plAssetDocument::AddPrefabDependencies(const plDocumentObject* pObject, plAssetDocumentInfo* pInfo) const
{
  {
    const plDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      plStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(plConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }


  const plHybridArray<plDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;
    AddPrefabDependencies(pChild, pInfo);
  }
}


void plAssetDocument::AddReferences(const plDocumentObject* pObject, plAssetDocumentInfo* pInfo, bool bInsidePrefab) const
{
  {
    const plDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      plStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(plConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
      pInfo->m_ThumbnailDependencies.Insert(plConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }

  const plRTTI* pType = pObject->GetTypeAccessor().GetType();
  plHybridArray<const plAbstractProperty*, 32> Properties;
  pType->GetAllProperties(Properties);
  for (auto pProp : Properties)
  {
    if (pProp->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;

    plBitflags<plDependencyFlags> depFlags;

    if (auto pAttr = pProp->GetAttributeByType<plAssetBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    if (auto pAttr = pProp->GetAttributeByType<plFileBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    // add all strings that are marked as asset references or file references
    if (depFlags != 0)
    {
      switch (pProp->GetCategory())
      {
        case plPropertyCategory::Member:
        {
          if (pProp->GetFlags().IsSet(plPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == plVariantType::String)
          {
            if (bInsidePrefab)
            {
              plHybridArray<plPropertySelection, 1> selection;
              selection.PushBack({pObject, plVariant()});
              plDefaultObjectState defaultState(GetObjectAccessor(), selection.GetArrayPtr());
              if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultValue(pProp))
                continue;
            }

            const plVariant& value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

            if (value.IsA<plString>())
            {
              if (depFlags.IsSet(plDependencyFlags::Transform))
                pInfo->m_TransformDependencies.Insert(value.Get<plString>());

              if (depFlags.IsSet(plDependencyFlags::Thumbnail))
                pInfo->m_ThumbnailDependencies.Insert(value.Get<plString>());

              if (depFlags.IsSet(plDependencyFlags::Package))
                pInfo->m_PackageDependencies.Insert(value.Get<plString>());
            }
          }
        }
        break;

        case plPropertyCategory::Array:
        case plPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(plPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == plVariantType::String)
          {
            const plInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

            if (bInsidePrefab)
            {
              plHybridArray<plPropertySelection, 1> selection;
              selection.PushBack({pObject, plVariant()});
              plDefaultContainerState defaultState(GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (plInt32 i = 0; i < iCount; ++i)
              {
                plVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(i))
                {
                  continue;
                }
                if (depFlags.IsSet(plDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<plString>());
              }
            }
            else
            {
              for (plInt32 i = 0; i < iCount; ++i)
              {
                plVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);

                if (depFlags.IsSet(plDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<plString>());
              }
            }
          }
        }
        break;

        case plPropertyCategory::Map:
          // #TODO Search for exposed params that reference assets.
          if (pProp->GetFlags().IsSet(plPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == plVariantType::String)
          {
            plVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());
            const plVariantDictionary& varDict = value.Get<plVariantDictionary>();
            if (bInsidePrefab)
            {
              plHybridArray<plPropertySelection, 1> selection;
              selection.PushBack({pObject, plVariant()});
              plDefaultContainerState defaultState(GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (auto it : varDict)
              {
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(it.Key()))
                {
                  continue;
                }

                if (depFlags.IsSet(plDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<plString>());
              }
            }
            else
            {
              for (auto it : varDict)
              {
                if (depFlags.IsSet(plDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<plString>());

                if (depFlags.IsSet(plDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<plString>());
              }
            }
          }
          break;

        default:
          break;
      }
    }
  }

  const plHybridArray<plDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;

    AddReferences(pChild, pInfo, bInsidePrefab);
  }
}

void plAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  const plDocumentObject* pRoot = GetObjectManager()->GetRootObject();

  AddPrefabDependencies(pRoot, pInfo);
  AddReferences(pRoot, pInfo, false);
}

void plAssetDocument::EngineConnectionEventHandler(const plEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == plEditorEngineProcessConnection::Event::Type::ProcessCrashed)
  {
    m_EngineStatus = EngineStatus::Disconnected;
  }
  else if (e.m_Type == plEditorEngineProcessConnection::Event::Type::ProcessStarted)
  {
    m_EngineStatus = EngineStatus::Initializing;
  }
}

plUInt64 plAssetDocument::GetDocumentHash() const
{
  plUInt64 uiHash = plHashingUtils::xxHash64(&m_pDocumentInfo->m_DocumentID, sizeof(plUuid));
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }

  // Gather used types, sort by name to make it stable and hash their data
  plSet<const plRTTI*> types;
  plToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
  plDynamicArray<const plRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const plRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }

  typesSorted.Sort([](const plRTTI* a, const plRTTI* b) { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  for (const plRTTI* pType : typesSorted)
  {
    uiHash = plHashingUtils::xxHash64(pType->GetTypeName().GetStartPointer(), pType->GetTypeName().GetElementCount(), uiHash);
    const plUInt32 uiType = pType->GetTypeVersion();
    uiHash = plHashingUtils::xxHash64(&uiType, sizeof(uiType), uiHash);
  }
  return uiHash;
}

void plAssetDocument::GetChildHash(const plDocumentObject* pObject, plUInt64& uiHash) const
{
  pObject->ComputeObjectHash(uiHash);

  for (auto pChild : pObject->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }
}

plTransformStatus plAssetDocument::DoTransformAsset(const plPlatformProfile* pAssetProfile0 /*= nullptr*/, plBitflags<plTransformFlags> transformFlags)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(plAssetDocumentFlags::DisableTransform))
    return plStatus("Asset transform has been disabled on this asset");

  const plPlatformProfile* pAssetProfile = plAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);

  plUInt64 uiHash = 0;
  plUInt64 uiThumbHash = 0;
  plAssetInfo::TransformState state = plAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), pAssetProfile, GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash);
  if (state == plAssetInfo::TransformState::UpToDate && !transformFlags.IsSet(plTransformFlags::ForceTransform))
    return plStatus(PLASMA_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return plStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    plAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, pAssetProfile, &AssetHeader, transformFlags](const char* szOutputTag) -> plTransformStatus
    {
      const plString sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), szOutputTag, pAssetProfile);
      plTransformStatus ret = InternalTransformAsset(sTargetFile, szOutputTag, pAssetProfile, AssetHeader, transformFlags);

      // if writing failed, make sure the output file does not exist
      if (ret.Failed())
      {
        plFileSystem::DeleteFile(sTargetFile);
      }
      plAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
      return ret;
    };

    plTransformStatus res;
    for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
    {
      res = GenerateOutput(it.Key());
      if (res.Failed())
        return res;
    }

    res = GenerateOutput("");
    if (res.Failed())
      return res;

    plAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

plTransformStatus plAssetDocument::TransformAsset(plBitflags<plTransformFlags> transformFlags, const plPlatformProfile* pAssetProfile)
{
  PLASMA_PROFILE_SCOPE("TransformAsset");

  if (!transformFlags.IsSet(plTransformFlags::ForceTransform))
  {
    PLASMA_SUCCEED_OR_RETURN(SaveDocument().m_Result);

    const auto assetFlags = GetAssetFlags();

    if (assetFlags.IsSet(plAssetDocumentFlags::DisableTransform) || (assetFlags.IsSet(plAssetDocumentFlags::OnlyTransformManually) && !transformFlags.IsSet(plTransformFlags::TriggeredManually)))
    {
      return plStatus(PLASMA_SUCCESS, "Transform is disabled for this asset");
    }
  }

  const plTransformStatus res = DoTransformAsset(pAssetProfile, transformFlags);

  if (transformFlags.IsSet(plTransformFlags::TriggeredManually))
  {
    SaveDocument().LogFailure();
    plAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
  }

  return res;
}

plTransformStatus plAssetDocument::CreateThumbnail()
{
  plUInt64 uiHash = 0;
  plUInt64 uiThumbHash = 0;

  plAssetInfo::TransformState state = plAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), plAssetCurator::GetSingleton()->GetActiveAssetProfile(), GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash);

  if (state == plAssetInfo::TransformState::UpToDate)
    return plStatus(PLASMA_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return plStatus("Computing the hash for this asset or any dependency failed");

  if (state == plAssetInfo::NeedsThumbnail)
  {
    ThumbnailInfo ThumbnailInfo;
    ThumbnailInfo.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    plTransformStatus res = InternalCreateThumbnail(ThumbnailInfo);

    InvalidateAssetThumbnail();
    plAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
  return plTransformStatus(plFmt("Asset state is {}", state));
}

plTransformStatus plAssetDocument::InternalTransformAsset(const char* szTargetFile, plStringView sOutputTag, const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  plDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  if (AssetHeader.Write(file) == PLASMA_FAILURE)
  {
    file.Discard();
    return plTransformStatus("Failed to write asset header");
  }

  plTransformStatus res = InternalTransformAsset(file, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  if (res.m_Result != plTransformResult::Success)
  {
    // We do not want to overwrite the old output file if we failed to transform the asset.
    file.Discard();
    return res;
  }


  if (file.Close().Failed())
  {
    plLog::Error("Could not open file for writing: '{0}'", szTargetFile);
    return plStatus("Opening the asset output file failed");
  }

  return plStatus(PLASMA_SUCCESS);
}

plString plAssetDocument::GetThumbnailFilePath(plStringView sSubAssetName /*= plStringView()*/) const
{
  return GetAssetDocumentManager()->GenerateResourceThumbnailPath(GetDocumentPath(), sSubAssetName);
}

void plAssetDocument::InvalidateAssetThumbnail(plStringView sSubAssetName /*= plStringView()*/) const
{
  const plString sResourceFile = GetThumbnailFilePath(sSubAssetName);
  plAssetCurator::GetSingleton()->NotifyOfFileChange(sResourceFile);
  plQtImageCache::GetSingleton()->InvalidateCache(sResourceFile);
}

plStatus plAssetDocument::SaveThumbnail(const plImage& img, const ThumbnailInfo& thumbnailInfo) const
{
  plImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (plImageConversion::Convert(img, converted, plImageFormat::R8G8B8A8_UNORM).Failed())
  {
    const plStringBuilder sResourceFile = GetThumbnailFilePath();

    plLog::Error("Could not convert asset thumbnail to target format: '{0}'", sResourceFile);
    return plStatus(plFmt("Could not convert asset thumbnail to target format: '{0}'", sResourceFile));
  }

  QImage qimg(converted.GetPixelPointer<plUInt8>(), converted.GetWidth(), converted.GetHeight(), QImage::Format_RGBA8888);

  return SaveThumbnail(qimg, thumbnailInfo);
}

plStatus plAssetDocument::SaveThumbnail(const QImage& qimg0, const ThumbnailInfo& thumbnailInfo) const
{
  const plStringBuilder sResourceFile = GetThumbnailFilePath();
  PLASMA_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  QImage qimg = qimg0;

  if (qimg.width() == qimg.height())
  {
    // if necessary scale the image to the proper size
    if (qimg.width() != plThumbnailSize)
      qimg = qimg.scaled(plThumbnailSize, plThumbnailSize, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to plThumbnailSize
    if (qimg.width() > qimg.height())
      qimg = qimg.scaledToWidth(plThumbnailSize, Qt::TransformationMode::SmoothTransformation);
    else
      qimg = qimg.scaledToHeight(plThumbnailSize, Qt::TransformationMode::SmoothTransformation);

    // create a black canvas
    QImage img2(plThumbnailSize, plThumbnailSize, QImage::Format_RGBA8888);
    img2.fill(Qt::GlobalColor::black);

    QPoint destPos = QPoint((plThumbnailSize - qimg.width()) / 2, (plThumbnailSize - qimg.height()) / 2);

    // paint the smaller image such that it ends up centered
    QPainter painter(&img2);
    painter.drawImage(destPos, qimg);
    painter.end();

    qimg = img2;
  }

  // make sure the directory exists, Qt will not create sub-folders
  const plStringBuilder sDir = sResourceFile.GetFileDirectory();
  PLASMA_SUCCEED_OR_RETURN(plOSFile::CreateDirectoryStructure(sDir));

  // save to JPEG
  if (!qimg.save(QString::fromUtf8(sResourceFile.GetData()), nullptr, 90))
  {
    plLog::Error("Could not save asset thumbnail: '{0}'", sResourceFile);
    return plStatus(plFmt("Could not save asset thumbnail: '{0}'", sResourceFile));
  }

  AppendThumbnailInfo(sResourceFile, thumbnailInfo);
  InvalidateAssetThumbnail();

  return plStatus(PLASMA_SUCCESS);
}

void plAssetDocument::AppendThumbnailInfo(plStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const
{
  plContiguousMemoryStreamStorage storage;
  {
    plFileReader reader;
    if (reader.Open(sThumbnailFile).Failed())
    {
      return;
    }
    storage.ReadAll(reader);
  }

  plDeferredFileWriter writer;
  writer.SetOutput(sThumbnailFile);
  writer.WriteBytes(storage.GetData(), storage.GetStorageSize64()).IgnoreResult();

  thumbnailInfo.Serialize(writer).IgnoreResult();

  if (writer.Close().Failed())
  {
    plLog::Error("Could not open file for writing: '{0}'", sThumbnailFile);
  }
}

plStatus plAssetDocument::RemoteExport(const plAssetFileHeader& header, const char* szOutputTarget) const
{
  plProgressRange range("Exporting Asset", 2, false);

  plLog::Info("Exporting {0} to \"{1}\"", GetDocumentTypeName(), szOutputTarget);

  if (GetEngineStatus() == plAssetDocument::EngineStatus::Disconnected)
  {
    return plStatus(plFmt("Exporting {0} to \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), szOutputTarget));
  }
  else if (GetEngineStatus() == plAssetDocument::EngineStatus::Initializing)
  {
    if (plEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), plDocumentOpenResponseMsgToEditor::GetStaticRTTI(), plTime::MakeFromSeconds(10)).Failed())
    {
      return plStatus(plFmt("Exporting {0} to \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), szOutputTarget));
    }
    PLASMA_ASSERT_DEV(GetEngineStatus() == plAssetDocument::EngineStatus::Loaded, "After receiving plDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  range.BeginNextStep(szOutputTarget);

  plExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  plStatus status(PLASMA_FAILURE);
  plProcessCommunicationChannel::WaitForMessageCallback callback = [&status](plProcessMessage* pMsg) -> bool
  {
    plExportDocumentMsgToEditor* pMsg2 = plDynamicCast<plExportDocumentMsgToEditor*>(pMsg);
    status = plStatus(pMsg2->m_bOutputSuccess ? PLASMA_SUCCESS : PLASMA_FAILURE, pMsg2->m_sFailureMsg);
    return true;
  };

  if (plEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), plExportDocumentMsgToEditor::GetStaticRTTI(), plTime::MakeFromSeconds(60), &callback).Failed())
  {
    return plStatus(plFmt("Remote exporting {0} to \"{1}\" timed out.", GetDocumentTypeName(), msg.m_sOutputFile));
  }
  else
  {
    if (status.Failed())
    {
      return status;
    }

    plLog::Success("{0} \"{1}\" has been exported.", GetDocumentTypeName(), msg.m_sOutputFile);

    ShowDocumentStatus(plFmt("{0} exported successfully", GetDocumentTypeName()));

    return plStatus(PLASMA_SUCCESS);
  }
}

plTransformStatus plAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo)
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
  return plStatus("Not implemented");
}

plStatus plAssetDocument::RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, plArrayPtr<plStringView> viewExclusionTags) const
{
  plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  plLog::Info("Create {0} thumbnail for \"{1}\"", GetDocumentTypeName(), GetDocumentPath());

  if (GetEngineStatus() == plAssetDocument::EngineStatus::Disconnected)
  {
    return plStatus(plFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == plAssetDocument::EngineStatus::Initializing)
  {
    if (plEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), plDocumentOpenResponseMsgToEditor::GetStaticRTTI(), plTime::MakeFromSeconds(10)).Failed())
    {
      return plStatus(plFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), GetDocumentPath()));
    }
    PLASMA_ASSERT_DEV(GetEngineStatus() == plAssetDocument::EngineStatus::Loaded, "After receiving plDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  SyncObjectsToEngine();
  plCreateThumbnailMsgToEngine msg;
  msg.m_uiWidth = plThumbnailSize;
  msg.m_uiHeight = plThumbnailSize;
  for (const plStringView& tag : viewExclusionTags)
  {
    msg.m_ViewExcludeTags.PushBack(tag);
  }
  GetEditorEngineConnection()->SendMessage(&msg);

  plDataBuffer data;
  plProcessCommunicationChannel::WaitForMessageCallback callback = [&data](plProcessMessage* pMsg) -> bool
  {
    plCreateThumbnailMsgToEditor* pThumbnailMsg = plDynamicCast<plCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (plEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), plCreateThumbnailMsgToEditor::GetStaticRTTI(), plTime::MakeFromSeconds(60), &callback).Failed())
  {
    return plStatus(plFmt("Create {0} thumbnail for \"{1}\" failed timed out.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return plStatus(plFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", GetDocumentTypeName()));
    }

    plImageHeader imgHeader;
    imgHeader.SetImageFormat(plImageFormat::R8G8B8A8_UNORM);
    imgHeader.SetWidth(msg.m_uiWidth);
    imgHeader.SetHeight(msg.m_uiHeight);

    plImage image;
    image.ResetAndAlloc(imgHeader);
    PLASMA_ASSERT_DEV(data.GetCount() == imgHeader.ComputeDataSize(), "Thumbnail plImage has different size than data buffer!");
    plMemoryUtils::Copy(image.GetPixelPointer<plUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, thumbnailInfo).LogFailure();

    plLog::Success("{0} thumbnail for \"{1}\" has been exported.", GetDocumentTypeName(), GetDocumentPath());

    ShowDocumentStatus(plFmt("{0} thumbnail created successfully", GetDocumentTypeName()));

    return plStatus(PLASMA_SUCCESS);
  }
}

plUInt16 plAssetDocument::GetAssetTypeVersion() const
{
  return (plUInt16)GetDynamicRTTI()->GetTypeVersion();
}

bool plAssetDocument::SendMessageToEngine(plEditorEngineDocumentMsg* pMessage /*= false*/) const
{
  return GetEditorEngineConnection()->SendMessage(pMessage);
}

void plAssetDocument::HandleEngineMessage(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plDocumentOpenResponseMsgToEditor>())
  {
    if (m_EngineConnectionType == plAssetDocEngineConnection::FullObjectMirroring)
    {
      // make sure the engine clears the document first
      plDocumentClearMsgToEngine msgClear;
      msgClear.m_DocumentGuid = GetGuid();
      SendMessageToEngine(&msgClear);

      m_pMirror->SendDocument();
    }
    m_EngineStatus = EngineStatus::Loaded;
    // make sure all sync objects are 'modified' so that they will get resent as well
    for (auto* pObject : m_SyncObjects)
    {
      pObject->SetModified();
    }
  }

  m_ProcessMessageEvent.Broadcast(pMsg);
}

void plAssetDocument::AddSyncObject(plEditorEngineSyncObject* pSync) const
{
  pSync->Configure(GetGuid(), [this](plEditorEngineSyncObject* pSync)
    { RemoveSyncObject(pSync); });

  m_SyncObjects.PushBack(pSync);
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void plAssetDocument::RemoveSyncObject(plEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveAndSwap(pSync);
}

plEditorEngineSyncObject* plAssetDocument::FindSyncObject(const plUuid& guid) const
{
  plEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

plEditorEngineSyncObject* plAssetDocument::FindSyncObject(const plRTTI* pType) const
{
  for (plEditorEngineSyncObject* pSync : m_SyncObjects)
  {
    if (pSync->GetDynamicRTTI() == pType)
    {
      return pSync;
    }
  }
  return nullptr;
}

void plAssetDocument::SyncObjectsToEngine() const
{
  // Tell the engine which sync objects have been removed recently
  {
    for (const auto& guid : m_DeletedObjects)
    {
      plEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = guid;
      SendMessageToEngine(&msg);
    }

    m_DeletedObjects.Clear();
  }

  for (auto* pObject : m_SyncObjects)
  {
    if (!pObject->GetModified())
      continue;

    plEditorEngineSyncObjectMsg msg;
    msg.m_ObjectGuid = pObject->m_SyncObjectGuid;
    msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

    plContiguousMemoryStreamStorage storage;
    plMemoryStreamWriter writer(&storage);
    plMemoryStreamReader reader(&storage);

    plReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = plArrayPtr<const plUInt8>(storage.GetData(), storage.GetStorageSize32());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void plAssetDocument::SendDocumentOpenMessage(bool bOpen)
{
  PLASMA_PROFILE_SCOPE("SendDocumentOpenMessage");

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  plAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  m_EngineStatus = EngineStatus::Initializing;

  plDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = GetCreateEngineMetaData();

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&m);
}

namespace
{
  static const char* szThumbnailInfoTag = "plThumb";
}

plResult plAssetDocument::ThumbnailInfo::Deserialize(plStreamReader& inout_reader)
{
  char tag[8] = {0};

  if (inout_reader.ReadBytes(tag, 7) != 7)
    return PLASMA_FAILURE;

  if (!plStringUtils::IsEqual(tag, szThumbnailInfoTag))
  {
    return PLASMA_FAILURE;
  }

  inout_reader >> m_uiHash;
  inout_reader >> m_uiVersion;
  inout_reader >> m_uiReserved;

  return PLASMA_SUCCESS;
}

plResult plAssetDocument::ThumbnailInfo::Serialize(plStreamWriter& inout_writer) const
{
  PLASMA_SUCCEED_OR_RETURN(inout_writer.WriteBytes(szThumbnailInfoTag, 7));

  inout_writer << m_uiHash;
  inout_writer << m_uiVersion;
  inout_writer << m_uiReserved;

  return PLASMA_SUCCESS;
}
