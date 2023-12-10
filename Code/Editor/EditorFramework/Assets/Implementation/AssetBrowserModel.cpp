#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentGenerator.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

plQtAssetFilter::plQtAssetFilter(QObject* pParent)
  : QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// plQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct FileComparer
{
  FileComparer(plQtAssetBrowserModel* pModel, const plHashTable<plUuid, plSubAsset>& allAssets)
    : m_pModel(pModel)
    , m_AllAssets(allAssets)
  {
    m_bSortByRecentlyUsed = m_pModel->m_pFilter->GetSortByRecentUse();
  }

  bool Less(const plQtAssetBrowserModel::VisibleEntry& a, const plQtAssetBrowserModel::VisibleEntry& b) const
  {
    if (a.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory) != b.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory))
      return a.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory);

    if (a.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory))
    {
      return plCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
    }

    const plSubAsset* pInfoA = nullptr;
    m_AllAssets.TryGetValue(a.m_Guid, pInfoA);

    const plSubAsset* pInfoB = nullptr;
    m_AllAssets.TryGetValue(b.m_Guid, pInfoB);

    plStringView sSortA;
    plStringView sSortB;
    if (pInfoA && !pInfoA->m_bMainAsset)
    {
      sSortA = pInfoA->GetName();
    }
    else
    {
      sSortA = a.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }
    if (pInfoB && !pInfoB->m_bMainAsset)
    {
      sSortB = pInfoB->GetName();
    }
    else
    {
      sSortB = b.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
    }

    if (m_bSortByRecentlyUsed)
    {
      if (pInfoA && pInfoB)
      {
        if (pInfoA->m_LastAccess != pInfoB->m_LastAccess)
        {
          return pInfoA->m_LastAccess > pInfoB->m_LastAccess;
        }
      }
      else if (pInfoA && pInfoA->m_LastAccess.IsPositive())
      {
        return true;
      }
      else if (pInfoB && pInfoB->m_LastAccess.IsPositive())
      {
        return false;
      }

      // in all other cases, fall through and do the file name comparison
    }

    plInt32 iValue = sSortA.Compare_NoCase(sSortB);
    if (iValue == 0)
    {
      if (!pInfoA && !pInfoB)
        return plCompareDataDirPath::Less(a.m_sAbsFilePath, b.m_sAbsFilePath);
      else if (pInfoA && pInfoB)
        return pInfoA->m_Data.m_Guid < pInfoB->m_Data.m_Guid;
      else
        return pInfoA == nullptr;
    }
    return iValue < 0;
  }

  PLASMA_ALWAYS_INLINE bool operator()(const plQtAssetBrowserModel::VisibleEntry& a, const plQtAssetBrowserModel::VisibleEntry& b) const
  {
    return Less(a, b);
  }

  plQtAssetBrowserModel* m_pModel = nullptr;
  const plHashTable<plUuid, plSubAsset>& m_AllAssets;
  bool m_bSortByRecentlyUsed = false;
};

plQtAssetBrowserModel::plQtAssetBrowserModel(QObject* pParent, plQtAssetFilter* pFilter)
  : QAbstractItemModel(pParent)
  , m_pFilter(pFilter)
{
  PLASMA_ASSERT_DEBUG(pFilter != nullptr, "plQtAssetBrowserModel requires a valid filter.");
  connect(pFilter, &plQtAssetFilter::FilterChanged, this, [this]()
    { resetModel(); });

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  PLASMA_VERIFY(connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageLoaded, this, &plQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
    "signal/slot connection failed");
  PLASMA_VERIFY(connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageInvalidated, this, &plQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
    "signal/slot connection failed");

  plFileSystemModel::GetSingleton()->m_FileChangedEvents.AddEventHandler(plMakeDelegate(&plQtAssetBrowserModel::FileSystemFileEventHandler, this));
  plFileSystemModel::GetSingleton()->m_FolderChangedEvents.AddEventHandler(plMakeDelegate(&plQtAssetBrowserModel::FileSystemFolderEventHandler, this));
}

plQtAssetBrowserModel::~plQtAssetBrowserModel()
{
  plFileSystemModel::GetSingleton()->m_FileChangedEvents.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserModel::FileSystemFileEventHandler, this));
  plFileSystemModel::GetSingleton()->m_FolderChangedEvents.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserModel::FileSystemFolderEventHandler, this));
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void plQtAssetBrowserModel::AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetUpdated:
    {
      VisibleEntry ve;
      ve.m_Guid = e.m_AssetGuid;
      ve.m_sAbsFilePath = e.m_pInfo->m_pAssetInfo->m_Path;

      HandleEntry(ve, AssetOp::Updated);
      break;
    }
    case plAssetCuratorEvent::Type::AssetListReset:
    {
      m_ImportExtensions.Clear();
      plAssetDocumentGenerator::GetSupportsFileTypes(m_ImportExtensions);
      break;
    }
    default:
      break;
  }
}


plInt32 plQtAssetBrowserModel::FindAssetIndex(const plUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (plUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}


plInt32 plQtAssetBrowserModel::FindIndex(plStringView sAbsPath) const
{
  for (plUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_sAbsFilePath.GetAbsolutePath() == sAbsPath)
    {
      return i;
    }
  }
  return -1;
}

void plQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  m_EntriesToDisplay.Clear();
  m_DisplayedEntries.Clear();

  auto allFolders = plFileSystemModel::GetSingleton()->GetFolders();

  for (const auto& folder : *allFolders)
  {
    if (m_pFilter->IsAssetFiltered(folder.Key().GetDataDirParentRelativePath(), true, nullptr))
      continue;

    auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
    entry.m_Flags = folder.Key().GetDataDirRelativePath().IsEmpty() ? plAssetBrowserItemFlags::DataDirectory : plAssetBrowserItemFlags::Folder;
    entry.m_sAbsFilePath = folder.Key();
  }

  auto allFiles = plFileSystemModel::GetSingleton()->GetFiles();

  for (const auto& file : *allFiles)
  {
    if (file.Value().m_DocumentID.IsValid())
    {
      auto mainAsset = plAssetCurator::GetSingleton()->GetSubAsset(file.Value().m_DocumentID);

      if (!mainAsset)
        continue;

      if (!m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*mainAsset)))
      {
        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = file.Value().m_DocumentID;
        entry.m_Flags = plAssetBrowserItemFlags::File | plAssetBrowserItemFlags::Asset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }

      for (const auto& subAssetGuid : mainAsset->m_pAssetInfo->m_SubAssets)
      {
        auto subAsset = plAssetCurator::GetSingleton()->GetSubAsset(subAssetGuid);

        if (subAsset && m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, &(*subAsset)))
          continue;

        auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
        entry.m_sAbsFilePath = file.Key();
        entry.m_Guid = subAssetGuid;
        entry.m_Flags |= plAssetBrowserItemFlags::SubAsset;
        m_DisplayedEntries.Insert(entry.m_Guid);
      }
    }
    else
    {
      if (m_pFilter->IsAssetFiltered(file.Key().GetDataDirParentRelativePath(), false, nullptr))
        continue;

      auto& entry = m_EntriesToDisplay.ExpandAndGetRef();
      entry.m_sAbsFilePath = file.Key();
      entry.m_Flags = plAssetBrowserItemFlags::File;
    }
  }

  plAssetCurator::plLockedSubAssetTable AllAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();
  const plHashTable<plUuid, plSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  FileComparer cmp(this, AllAssets);
  m_EntriesToDisplay.Sort(cmp);

  endResetModel();
}

void plQtAssetBrowserModel::HandleEntry(const VisibleEntry& entry, AssetOp op)
{
  auto subAsset = plAssetCurator::GetSingleton()->GetSubAsset(entry.m_Guid);

  if (m_pFilter->IsAssetFiltered(entry.m_sAbsFilePath.GetDataDirParentRelativePath(), entry.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory), subAsset.Borrow()))
  {
    if (!m_DisplayedEntries.Contains(entry.m_Guid))
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  plAssetCurator::plLockedSubAssetTable AllAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();
  const plHashTable<plUuid, plSubAsset>& AllAssets = *AllAssetsLocked.Borrow();

  FileComparer cmp(this, AllAssets);
  VisibleEntry* pLB = std::lower_bound(begin(m_EntriesToDisplay), end(m_EntriesToDisplay), entry, cmp);
  plUInt32 uiInsertIndex = pLB - m_EntriesToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model on top of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_EntriesToDisplay.GetCount())
  {
    for (plUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); i++)
    {
      VisibleEntry& displayEntry = m_EntriesToDisplay[i];
      if (!cmp.Less(displayEntry, entry) && !cmp.Less(entry, displayEntry))
      {
        uiInsertIndex = i;
        pLB = &displayEntry;
        break;
      }
    }
  }

  if (op == AssetOp::Add)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_EntriesToDisplay.Insert(entry, uiInsertIndex);
    if (entry.m_Guid.IsValid())
      m_DisplayedEntries.Insert(entry.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_EntriesToDisplay.RemoveAtAndCopy(uiInsertIndex);
      if (entry.m_Guid.IsValid())
        m_DisplayedEntries.Remove(entry.m_Guid);
      endRemoveRows();
    }
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_EntriesToDisplay.GetCount() && !cmp.Less(*pLB, entry) && !cmp.Less(entry, *pLB))
    {
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      plInt32 oldIndex = FindAssetIndex(entry.m_Guid);
      if (oldIndex != -1)
      {
        // Name has changed, remove old entry
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_EntriesToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(entry.m_Guid);
        endRemoveRows();
      }
      HandleEntry(entry, AssetOp::Add);
    }
  }
}


////////////////////////////////////////////////////////////////////////
// plQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void plQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant userData1, QVariant userData2)
{
  const plUuid guid(userData1.toULongLong(), userData2.toULongLong());

  for (plUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void plQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, plUInt32 uiImageID)
{
  for (plUInt32 i = 0; i < m_EntriesToDisplay.GetCount(); ++i)
  {
    if (m_EntriesToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void plQtAssetBrowserModel::OnFileSystemUpdate()
{
  plDynamicArray<FsEvent> events;

  {
    PLASMA_LOCK(m_Mutex);
    events.Swap(m_QueuedFileSystemEvents);
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type == plFileChangedEvent::Type::ModelReset)
    {
      resetModel();
      return;
    }
  }

  for (const auto& e : events)
  {
    if (e.m_FileEvent.m_Type != plFileChangedEvent::Type::None)
      HandleFile(e.m_FileEvent);
    else
      HandleFolder(e.m_FolderEvent);
  }
}

QVariant plQtAssetBrowserModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_EntriesToDisplay.GetCount())
    return QVariant();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  // Common properties shared among all item types.
  switch (iRole)
  {
    case plQtAssetBrowserModel::UserRoles::ItemFlags:
      return (int)entry.m_Flags.GetValue();
    case plQtAssetBrowserModel::UserRoles::Importable:
    {
      if (entry.m_Flags.IsSet(plAssetBrowserItemFlags::File) && !entry.m_Flags.IsSet(plAssetBrowserItemFlags::Asset))
      {
        plStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        return bImportable;
      }
      return false;
    }
    case plQtAssetBrowserModel::UserRoles::RelativePath:
      return plMakeQString(entry.m_sAbsFilePath.GetDataDirParentRelativePath());
    case plQtAssetBrowserModel::UserRoles::AbsolutePath:
      return plMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
  }

  if (entry.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::DataDirectory))
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        plStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();

        return plMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        plStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
        return plMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return plMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case plQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        return plQtUiServices::GetCachedIconResource(entry.m_Flags.IsSet(plAssetBrowserItemFlags::DataDirectory) ? ":/EditorFramework/Icons/DataDirectory.svg" : ":/EditorFramework/Icons/Folder.svg");
      }
    }
  }
  else if (!entry.m_Guid.IsValid()) // Normal file
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        return plMakeQString(entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension());
      }
      break;

      case Qt::EditRole:
      {
        plStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileNameAndExtension();
        return plMakeQString(sFilename);
      }

      case Qt::ToolTipRole:
      {
        return plMakeQString(entry.m_sAbsFilePath.GetAbsolutePath());
      }
      break;

      case plQtAssetBrowserModel::UserRoles::AssetIcon:
      {
        plStringBuilder sExt = entry.m_sAbsFilePath.GetAbsolutePath().GetFileExtension();
        sExt.ToLower();
        const bool bImportable = m_ImportExtensions.Contains(sExt);
        const bool bIsReferenced = plAssetCurator::GetSingleton()->IsReferenced(entry.m_sAbsFilePath.GetAbsolutePath());
        if (bImportable)
        {
          return plQtUiServices::GetCachedIconResource(bIsReferenced ? ":/EditorFramework/Icons/ImportedFile.svg" : ":/EditorFramework/Icons/ImportableFile.svg");
        }
        return plQtUiServices::GetCachedIconResource(":/GuiFoundation/Icons/Document.svg");
      }

      case Qt::DecorationRole:
      {
        QFileInfo fi(plMakeQString(entry.m_sAbsFilePath));
        return m_IconProvider.icon(fi);
      }
    }
  }
  else if (entry.m_Guid.IsValid()) // Asset or sub-asset
  {
    const plUuid AssetGuid = entry.m_Guid;
    const plAssetCurator::plLockedSubAsset pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

    // this can happen when a file was just changed on disk, e.g. got deleted
    if (pSubAsset == nullptr)
      return QVariant();

    switch (iRole)
    {
      case Qt::DisplayRole:
      {
        plStringBuilder sFilename = pSubAsset->GetName();
        return plMakeQString(sFilename);
      }
      break;

      case Qt::EditRole:
      {
        if (entry.m_Flags.IsSet(plAssetBrowserItemFlags::Asset))
        {
          // Don't allow changing extensions of assets
          plStringView sFilename = entry.m_sAbsFilePath.GetAbsolutePath().GetFileName();
          return plMakeQString(sFilename);
        }
      }
      break;

      case Qt::ToolTipRole:
      {
        plStringBuilder sToolTip = pSubAsset->GetName();
        sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());
        sToolTip.Append("\nTransform State: ");
        switch (pSubAsset->m_pAssetInfo->m_TransformState)
        {
          case plAssetInfo::Unknown:
            sToolTip.Append("Unknown");
            break;
          case plAssetInfo::UpToDate:
            sToolTip.Append("Up To Date");
            break;
          case plAssetInfo::NeedsTransform:
            sToolTip.Append("Needs Transform");
            break;
          case plAssetInfo::NeedsThumbnail:
            sToolTip.Append("Needs Thumbnail");
            break;
          case plAssetInfo::TransformError:
            sToolTip.Append("Transform Error");
            break;
          case plAssetInfo::MissingTransformDependency:
            sToolTip.Append("Missing Transform Dependency");
            break;
          case plAssetInfo::MissingThumbnailDependency:
            sToolTip.Append("Missing Thumbnail Dependency");
            break;
          case plAssetInfo::CircularDependency:
            sToolTip.Append("Circular Dependency");
            break;
          default:
            break;
        }

        return QString::fromUtf8(sToolTip, sToolTip.GetElementCount());
      }
      case Qt::DecorationRole:
      {
        if (m_bIconMode)
        {
          plString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_Path, pSubAsset->m_Data.m_sName);

          plUInt64 uiUserData1, uiUserData2;
          AssetGuid.GetValues(uiUserData1, uiUserData2);

          const QPixmap* pThumbnailPixmap = plQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName,
            sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &entry.m_uiThumbnailID);

          return *pThumbnailPixmap;
        }
        else
        {
          return plQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, plColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, plColorScheme::CategoryColorUsage::OverlayIcon));
        }
      }
      break;

      case UserRoles::SubAssetGuid:
        return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
      case UserRoles::AssetGuid:
        return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
      case UserRoles::AssetIcon:
        return plQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, plColorScheme::GetCategoryColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sAssetCategory, plColorScheme::CategoryColorUsage::OverlayIcon));
      case UserRoles::TransformState:
        return (int)pSubAsset->m_pAssetInfo->m_TransformState;
    }
  }
  else
  {
    PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
  return QVariant();
}

bool plQtAssetBrowserModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  if (!index.isValid())
    return false;

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_EntriesToDisplay.GetCount())
    return false;

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];
  const bool bIsAsset = entry.m_Guid.IsValid();
  if (entry.m_Flags.IsAnySet(plAssetBrowserItemFlags::Folder | plAssetBrowserItemFlags::File))
  {
    const plString& sAbsPath = entry.m_sAbsFilePath.GetAbsolutePath();
    emit editingFinished(plMakeQString(sAbsPath), value.toString(), bIsAsset);

    return true;
  }

  return false;
}

Qt::ItemFlags plQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_EntriesToDisplay.GetCount())
    return Qt::ItemFlags();

  const VisibleEntry& entry = m_EntriesToDisplay[iRow];

  Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (entry.m_Flags.IsAnySet(plAssetBrowserItemFlags::File | plAssetBrowserItemFlags::Folder))
  {
    flags |= Qt::ItemIsDragEnabled | Qt::ItemIsEditable;
  }

  return flags;
}

QVariant plQtAssetBrowserModel::headerData(int iSection, Qt::Orientation orientation, int iRole) const
{
  if (orientation == Qt::Horizontal && iRole == Qt::DisplayRole)
  {
    switch (iSection)
    {
      case 0:
        return QString("Files");
    }
  }
  return QVariant();
}

QModelIndex plQtAssetBrowserModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  if (parent.isValid() || iColumn != 0)
    return QModelIndex();

  return createIndex(iRow, iColumn);
}

QModelIndex plQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int plQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_EntriesToDisplay.GetCount();
}

int plQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList plQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/plEditor.AssetGuid";
  types << "application/plEditor.files";
  return types;
}

QMimeData* plQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QString sGuids;
  QList<QUrl> urls;
  plHybridArray<QString, 1> guids;
  plHybridArray<QString, 1> files;

  plStringBuilder tmp;

  for (plUInt32 i = 0; i < (plUInt32)indexes.size(); ++i)
  {
    QString sGuid(plConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<plUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();
    guids.PushBack(sGuid);
    if (i == 0)
      sGuids += sPath;
    else
      sGuids += "\n" + sPath;

    files.PushBack(sPath);
    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << guids;

  QByteArray encodedData2;
  QDataStream stream2(&encodedData2, QIODevice::WriteOnly);
  stream2 << files;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/plEditor.AssetGuid", encodedData);
  mimeData->setData("application/plEditor.files", encodedData2);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

Qt::DropActions plQtAssetBrowserModel::supportedDropActions() const
{
  return Qt::MoveAction | Qt::LinkAction;
}

void plQtAssetBrowserModel::FileSystemFileEventHandler(const plFileChangedEvent& e)
{
  bool bFire = false;

  {
    PLASMA_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FileEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void plQtAssetBrowserModel::FileSystemFolderEventHandler(const plFolderChangedEvent& e)
{
  bool bFire = false;

  {
    PLASMA_LOCK(m_Mutex);

    bFire = m_QueuedFileSystemEvents.IsEmpty();

    auto& res = m_QueuedFileSystemEvents.ExpandAndGetRef();
    res.m_FolderEvent = e;
  }

  if (bFire)
  {
    QMetaObject::invokeMethod(this, "OnFileSystemUpdate", Qt::ConnectionType::QueuedConnection);
  }
}

void plQtAssetBrowserModel::HandleFile(const plFileChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Guid = e.m_Status.m_DocumentID;
  ve.m_sAbsFilePath = e.m_Path;
  ve.m_Flags = plAssetBrowserItemFlags::File;
  if (ve.m_Guid.IsValid())
  {
    ve.m_Flags |= plAssetBrowserItemFlags::Asset;
  }

  switch (e.m_Type)
  {
    case plFileChangedEvent::Type::ModelReset:
      resetModel();
      return;

    case plFileChangedEvent::Type::FileAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case plFileChangedEvent::Type::DocumentLinked:
    {
      ve.m_Guid = plUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case plFileChangedEvent::Type::DocumentUnlinked:
    {
      ve.m_Guid = e.m_Status.m_DocumentID;
      HandleEntry(ve, AssetOp::Remove);
      ve.m_Guid = plUuid::MakeInvalid();
      HandleEntry(ve, AssetOp::Add);
      return;
    }
    case plFileChangedEvent::Type::FileRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}

void plQtAssetBrowserModel::HandleFolder(const plFolderChangedEvent& e)
{
  VisibleEntry ve;
  ve.m_Flags = plAssetBrowserItemFlags::Folder;
  ve.m_sAbsFilePath = e.m_Path;

  switch (e.m_Type)
  {
    case plFolderChangedEvent::Type::FolderAdded:
      HandleEntry(ve, AssetOp::Add);
      return;
    case plFolderChangedEvent::Type::FolderRemoved:
      HandleEntry(ve, AssetOp::Remove);
      return;
    default:
      return;
  }
}
