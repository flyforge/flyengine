#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetBrowserModel.moc.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

plQtAssetFilter::plQtAssetFilter(QObject* pParent)
  : QObject(pParent)
{
}

////////////////////////////////////////////////////////////////////////
// plQtAssetBrowserModel public functions
////////////////////////////////////////////////////////////////////////

struct AssetComparer
{
  AssetComparer(plQtAssetBrowserModel* model, const plHashTable<plUuid, plSubAsset>& allAssets)
    : m_Model(model)
    , m_AllAssets(allAssets)
  {
  }

  PLASMA_ALWAYS_INLINE bool Less(const plQtAssetBrowserModel::AssetEntry& a, const plQtAssetBrowserModel::AssetEntry& b) const
  {
    const plSubAsset* pInfoA = &m_AllAssets.Find(a.m_Guid).Value();
    const plSubAsset* pInfoB = &m_AllAssets.Find(b.m_Guid).Value();

    return m_Model->m_pFilter->Less(pInfoA, pInfoB);
  }

  PLASMA_ALWAYS_INLINE bool operator()(const plQtAssetBrowserModel::AssetEntry& a, const plQtAssetBrowserModel::AssetEntry& b) const
  {
    return Less(a, b);
  }

  plQtAssetBrowserModel* m_Model;
  const plHashTable<plUuid, plSubAsset>& m_AllAssets;
};

plQtAssetBrowserModel::plQtAssetBrowserModel(QObject* pParent, plQtAssetFilter* pFilter)
  : QAbstractItemModel(pParent)
  , m_pFilter(pFilter)
{
  PLASMA_ASSERT_DEBUG(pFilter != nullptr, "plQtAssetBrowserModel requires a valid filter.");
  connect(pFilter, &plQtAssetFilter::FilterChanged, this, [this]() { resetModel(); });

  plAssetCurator::GetSingleton()->m_Events.AddEventHandler(plMakeDelegate(&plQtAssetBrowserModel::AssetCuratorEventHandler, this));

  resetModel();
  SetIconMode(true);

  PLASMA_VERIFY(connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageLoaded, this, &plQtAssetBrowserModel::ThumbnailLoaded) != nullptr,
    "signal/slot connection failed");
  PLASMA_VERIFY(connect(plQtImageCache::GetSingleton(), &plQtImageCache::ImageInvalidated, this, &plQtAssetBrowserModel::ThumbnailInvalidated) != nullptr,
    "signal/slot connection failed");
}

plQtAssetBrowserModel::~plQtAssetBrowserModel()
{
  plAssetCurator::GetSingleton()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAssetBrowserModel::AssetCuratorEventHandler, this));
}

void plQtAssetBrowserModel::AssetCuratorEventHandler(const plAssetCuratorEvent& e)
{
  switch (e.m_Type)
  {
    case plAssetCuratorEvent::Type::AssetAdded:
      HandleAsset(e.m_pInfo, AssetOp::Add);
      break;
    case plAssetCuratorEvent::Type::AssetRemoved:
      HandleAsset(e.m_pInfo, AssetOp::Remove);
      break;
    case plAssetCuratorEvent::Type::AssetListReset:
      resetModel();
      break;
    case plAssetCuratorEvent::Type::AssetUpdated:
      HandleAsset(e.m_pInfo, AssetOp::Updated);
      break;
    default:
      break;
  }
}


plInt32 plQtAssetBrowserModel::FindAssetIndex(const plUuid& assetGuid) const
{
  if (!m_DisplayedEntries.Contains(assetGuid))
    return -1;

  for (plUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == assetGuid)
    {
      return i;
    }
  }

  return -1;
}

void plQtAssetBrowserModel::resetModel()
{
  beginResetModel();

  plAssetCurator::plLockedSubAssetTable AllAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();
  const plHashTable<plUuid, plSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  m_AssetsToDisplay.Clear();
  m_AssetsToDisplay.Reserve(AllAssets.GetCount());
  m_DisplayedEntries.Clear();

  AssetEntry ae;
  // last access > filename
  for (auto it = AllAssets.GetIterator(); it.IsValid(); ++it)
  {
    const plSubAsset* pSub = &it.Value();
    if (m_pFilter->IsAssetFiltered(pSub))
      continue;

    Init(ae, pSub);

    m_AssetsToDisplay.PushBack(ae);
    m_DisplayedEntries.Insert(ae.m_Guid);
  }

  AssetComparer cmp(this, AllAssets);
  m_AssetsToDisplay.Sort(cmp);

  endResetModel();
  PLASMA_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
}

void plQtAssetBrowserModel::HandleAsset(const plSubAsset* pInfo, AssetOp op)
{
  if (m_pFilter->IsAssetFiltered(pInfo))
  {
    // TODO: Due to file system watcher weirdness the m_sDataDirRelativePath can be empty at this point when renaming files
    // really rare haven't reproed it yet but that case crashes when getting the name so early out that.
    if (!m_DisplayedEntries.Contains(pInfo->m_Data.m_Guid) || pInfo->m_pAssetInfo->m_sDataDirParentRelativePath.IsEmpty())
    {
      return;
    }

    // Filtered but still exists, remove it.
    op = AssetOp::Remove;
  }

  plAssetCurator::plLockedSubAssetTable AllAssetsLocked = plAssetCurator::GetSingleton()->GetKnownSubAssets();
  const plHashTable<plUuid, plSubAsset>& AllAssets = *(AllAssetsLocked.operator->());

  AssetEntry ae;
  Init(ae, pInfo);

  AssetComparer cmp(this, AllAssets);
  AssetEntry* pLB = std::lower_bound(begin(m_AssetsToDisplay), end(m_AssetsToDisplay), ae, cmp);
  plUInt32 uiInsertIndex = pLB - m_AssetsToDisplay.GetData();
  // TODO: Due to sorting issues the above can fail (we need to add a sorting model ontop of this as we use mutable data (name) for sorting.
  if (uiInsertIndex >= m_AssetsToDisplay.GetCount())
  {
    for (plUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); i++)
    {
      AssetEntry& displayEntry = m_AssetsToDisplay[i];
      if (!cmp.Less(displayEntry, ae) && !cmp.Less(ae, displayEntry))
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
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
      return;

    beginInsertRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
    m_AssetsToDisplay.Insert(ae, uiInsertIndex);
    m_DisplayedEntries.Insert(pInfo->m_Data.m_Guid);
    endInsertRows();
  }
  else if (op == AssetOp::Remove)
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      beginRemoveRows(QModelIndex(), uiInsertIndex, uiInsertIndex);
      m_AssetsToDisplay.RemoveAtAndCopy(uiInsertIndex);
      m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
      endRemoveRows();
    }
  }
  else
  {
    // Equal?
    if (uiInsertIndex < m_AssetsToDisplay.GetCount() && !cmp.Less(*pLB, ae) && !cmp.Less(ae, *pLB))
    {
      QModelIndex idx = index(uiInsertIndex, 0);
      Q_EMIT dataChanged(idx, idx);
    }
    else
    {
      plInt32 oldIndex = FindAssetIndex(pInfo->m_Data.m_Guid);
      if (oldIndex != -1)
      {
        // Name has changed, remove old entry
        beginRemoveRows(QModelIndex(), oldIndex, oldIndex);
        m_AssetsToDisplay.RemoveAtAndCopy(oldIndex);
        m_DisplayedEntries.Remove(pInfo->m_Data.m_Guid);
        endRemoveRows();
      }
      HandleAsset(pInfo, AssetOp::Add);
    }
  }
  PLASMA_ASSERT_DEBUG(m_AssetsToDisplay.GetCount() == m_DisplayedEntries.GetCount(), "Implementation error: Set and sorted list diverged");
}


////////////////////////////////////////////////////////////////////////
// plQtAssetBrowserModel QAbstractItemModel functions
////////////////////////////////////////////////////////////////////////

void plQtAssetBrowserModel::ThumbnailLoaded(QString sPath, QModelIndex index, QVariant UserData1, QVariant UserData2)
{
  const plUuid guid(UserData1.toULongLong(), UserData2.toULongLong());

  for (plUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_Guid == guid)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

void plQtAssetBrowserModel::ThumbnailInvalidated(QString sPath, plUInt32 uiImageID)
{
  for (plUInt32 i = 0; i < m_AssetsToDisplay.GetCount(); ++i)
  {
    if (m_AssetsToDisplay[i].m_uiThumbnailID == uiImageID)
    {
      QModelIndex idx = createIndex(i, 0);
      Q_EMIT dataChanged(idx, idx);
      return;
    }
  }
}

QVariant plQtAssetBrowserModel::data(const QModelIndex& index, int role) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  const plInt32 iRow = index.row();
  if (iRow < 0 || iRow >= (plInt32)m_AssetsToDisplay.GetCount())
    return QVariant();

  const auto& asset = m_AssetsToDisplay[iRow];
  const plUuid AssetGuid = asset.m_Guid;
  const plAssetCurator::plLockedSubAsset pSubAsset = plAssetCurator::GetSingleton()->GetSubAsset(AssetGuid);

  // this can happen when a file was just changed on disk, e.g. got deleted
  if (pSubAsset == nullptr)
    return QVariant();

  switch (role)
  {
    case Qt::DisplayRole:
    {
      plStringBuilder sFilename = pSubAsset->GetName();
      return QString::fromUtf8(sFilename, sFilename.GetElementCount());
    }
    break;

    case Qt::ToolTipRole:
    {
      plStringBuilder sToolTip = pSubAsset->GetName();
      sToolTip.Append("\n", pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath);
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
        case plAssetInfo::MissingDependency:
          sToolTip.Append("Missing Dependency");
          break;
        case plAssetInfo::MissingReference:
          sToolTip.Append("Missing Reference");
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
        if (pSubAsset->m_bIsDir)
        {
          QPixmap pixMap = plQtUiServices::GetCachedPixmapResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon);
          return pixMap;
        }
        plString sThumbnailPath = pSubAsset->m_pAssetInfo->GetManager()->GenerateResourceThumbnailPath(pSubAsset->m_pAssetInfo->m_sAbsolutePath, pSubAsset->m_Data.m_sName);

        plUInt64 uiUserData1, uiUserData2;
        AssetGuid.GetValues(uiUserData1, uiUserData2);

        const QPixmap* pThumbnailPixmap = plQtImageCache::GetSingleton()->QueryPixmapForType(pSubAsset->m_Data.m_sSubAssetsDocumentTypeName,
          sThumbnailPath, index, QVariant(uiUserData1), QVariant(uiUserData2), &asset.m_uiThumbnailID);

        return *pThumbnailPixmap;
      }
      else
      {
        return plQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, plColorScheme::GetGroupColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_IconColorGroup, 2));
      }
    }
    break;

    case UserRoles::SubAssetGuid:
    {
      return QVariant::fromValue(pSubAsset->m_Data.m_Guid);
    }
    case UserRoles::AssetGuid:
    {
      return QVariant::fromValue(pSubAsset->m_pAssetInfo->m_Info->m_DocumentID);
    }
    case UserRoles::AbsolutePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sAbsolutePath, pSubAsset->m_pAssetInfo->m_sAbsolutePath.GetElementCount());

    case UserRoles::RelativePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sDataDirRelativePath, pSubAsset->m_pAssetInfo->m_sDataDirRelativePath.GetElementCount());

    case UserRoles::ParentRelativePath:
      return QString::fromUtf8(pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath, pSubAsset->m_pAssetInfo->m_sDataDirParentRelativePath.GetElementCount());

    case UserRoles::AssetIcon:
    {
      if (pSubAsset->m_bIsDir)
        return QPixmap();
      return plQtUiServices::GetCachedIconResource(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_sIcon, plColorScheme::GetGroupColor(pSubAsset->m_pAssetInfo->m_pDocumentTypeDescriptor->m_IconColorGroup, 2));
    }
    case UserRoles::TransformState:
      return (int)pSubAsset->m_pAssetInfo->m_TransformState;

    case UserRoles::Type:
      return pSubAsset->m_bIsDir;
  }

  return QVariant();
}

Qt::ItemFlags plQtAssetBrowserModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;
}

QVariant plQtAssetBrowserModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return QString("Asset");
    }
  }
  return QVariant();
}

QModelIndex plQtAssetBrowserModel::index(int row, int column, const QModelIndex& parent) const
{
  if (parent.isValid() || column != 0)
    return QModelIndex();

  return createIndex(row, column);
}

QModelIndex plQtAssetBrowserModel::parent(const QModelIndex& index) const
{
  return QModelIndex();
}

int plQtAssetBrowserModel::rowCount(const QModelIndex& parent) const
{
  if (parent.isValid())
    return 0;

  return (int)m_AssetsToDisplay.GetCount();
}

int plQtAssetBrowserModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QStringList plQtAssetBrowserModel::mimeTypes() const
{
  QStringList types;
  types << "application/PlasmaEditor.AssetGuid";
  return types;
}

QMimeData* plQtAssetBrowserModel::mimeData(const QModelIndexList& indexes) const
{
  QMimeData* mimeData = new QMimeData();
  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  QString sGuids;
  QList<QUrl> urls;

  plStringBuilder tmp;

  stream << (int)indexes.size();
  for (int i = 0; i < indexes.size(); ++i)
  {
    QString sGuid(plConversionUtils::ToString(data(indexes[i], UserRoles::SubAssetGuid).value<plUuid>(), tmp).GetData());
    QString sPath = data(indexes[i], UserRoles::AbsolutePath).toString();

    stream << sGuid;
    sGuids += sPath + "\n";

    urls.push_back(QUrl::fromLocalFile(sPath));
  }

  mimeData->setData("application/PlasmaEditor.AssetGuid", encodedData);
  mimeData->setText(sGuids);
  mimeData->setUrls(urls);
  return mimeData;
}

void plQtAssetBrowserModel::Init(AssetEntry& ae, const plSubAsset* pInfo)
{
  ae.m_Guid = pInfo->m_Data.m_Guid;
  ae.m_uiThumbnailID = (plUInt32)-1;
}
