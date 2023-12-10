#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>

plQtPropertyAnimModel::plQtPropertyAnimModel(plPropertyAnimAssetDocument* pDocument, QObject* pParent)
  : QAbstractItemModel(pParent)
  , m_pAssetDoc(pDocument)
{
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.AddEventHandler(plMakeDelegate(&plQtPropertyAnimModel::DocumentStructureEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtPropertyAnimModel::DocumentPropertyEventHandler, this));

  TriggerBuildMapping();
}

plQtPropertyAnimModel::~plQtPropertyAnimModel()
{
  m_pAssetDoc->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtPropertyAnimModel::DocumentPropertyEventHandler, this));
  m_pAssetDoc->GetObjectManager()->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plQtPropertyAnimModel::DocumentStructureEventHandler, this));
}

QVariant plQtPropertyAnimModel::data(const QModelIndex& index, int iRole) const
{
  if (!index.isValid() || index.column() != 0)
    return QVariant();

  plQtPropertyAnimModelTreeEntry* pItem = static_cast<plQtPropertyAnimModelTreeEntry*>(index.internalPointer());
  PLASMA_ASSERT_DEBUG(pItem != nullptr, "Invalid model index");

  switch (iRole)
  {
    case Qt::DisplayRole:
      return QString(pItem->m_sDisplay.GetData());

    case Qt::DecorationRole:
      return pItem->m_Icon;

    case UserRoles::TrackPtr:
      return QVariant::fromValue((void*)pItem->m_pTrack);

    case UserRoles::TreeItem:
      return QVariant::fromValue((void*)pItem);

    case UserRoles::TrackIdx:
      return pItem->m_iTrackIdx;

    case UserRoles::Path:
      return QString(pItem->m_sPathToItem.GetData());
  }

  return QVariant();
}

Qt::ItemFlags plQtPropertyAnimModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemFlags();

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QModelIndex plQtPropertyAnimModel::index(int iRow, int iColumn, const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (iColumn != 0)
    return QModelIndex();

  plQtPropertyAnimModelTreeEntry* pParentItem = static_cast<plQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  if (pParentItem != nullptr)
  {
    return createIndex(iRow, iColumn, (void*)&m_AllEntries[m_iInUse][pParentItem->m_Children[iRow]]);
  }
  else
  {
    if (iRow >= (int)m_TopLevelEntries[m_iInUse].GetCount())
      return QModelIndex();

    return createIndex(iRow, iColumn, (void*)&m_AllEntries[m_iInUse][m_TopLevelEntries[m_iInUse][iRow]]);
  }
}

QModelIndex plQtPropertyAnimModel::parent(const QModelIndex& index) const
{
  if (!index.isValid() || index.column() != 0)
    return QModelIndex();

  plQtPropertyAnimModelTreeEntry* pItem = static_cast<plQtPropertyAnimModelTreeEntry*>(index.internalPointer());

  if (pItem->m_iParent < 0)
    return QModelIndex();

  return createIndex(m_AllEntries[m_iInUse][pItem->m_iParent].m_uiOwnRowIndex, index.column(), (void*)&m_AllEntries[m_iInUse][pItem->m_iParent]);
}

int plQtPropertyAnimModel::rowCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  if (!parent.isValid())
    return m_TopLevelEntries[m_iInUse].GetCount();

  plQtPropertyAnimModelTreeEntry* pItem = static_cast<plQtPropertyAnimModelTreeEntry*>(parent.internalPointer());
  return pItem->m_Children.GetCount();
}

int plQtPropertyAnimModel::columnCount(const QModelIndex& parent /*= QModelIndex()*/) const
{
  return 1;
}

void plQtPropertyAnimModel::DocumentStructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      TriggerBuildMapping();
      break;

    default:
      break;
  }
}

void plQtPropertyAnimModel::DocumentPropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_EventType == plDocumentObjectPropertyEvent::Type::PropertySet)
  {
    if (e.m_sProperty == "ObjectPath")
    {
      TriggerBuildMapping();
      return;
    }
  }
}

void plQtPropertyAnimModel::TriggerBuildMapping()
{
  if (m_bBuildMappingQueued)
    return;

  m_bBuildMappingQueued = true;
  QTimer::singleShot(100, this, SLOT(onBuildMappingTriggered()));
}

void plQtPropertyAnimModel::onBuildMappingTriggered()
{
  BuildMapping();
  m_bBuildMappingQueued = false;
}

void plQtPropertyAnimModel::BuildMapping()
{
  const plInt32 iToUse = (m_iInUse + 1) % 2;
  BuildMapping(iToUse);

  if (m_AllEntries[0] != m_AllEntries[1])
  {
    beginResetModel();
    m_iInUse = iToUse;
    endResetModel();
  }
}

void plQtPropertyAnimModel::BuildMapping(plInt32 iToUse)
{
  m_TopLevelEntries[iToUse].Clear();
  m_AllEntries[iToUse].Clear();

  const plPropertyAnimationTrackGroup& group = *m_pAssetDoc->GetProperties();

  plStringBuilder tmp;

  for (plUInt32 tIdx = 0; tIdx < group.m_Tracks.GetCount(); ++tIdx)
  {
    plPropertyAnimationTrack* pTrack = group.m_Tracks[tIdx];

    tmp = pTrack->m_sObjectSearchSequence;
    if (!pTrack->m_sComponentType.IsEmpty())
    {
      tmp.AppendPath(":");
      tmp.Append(pTrack->m_sComponentType.GetData());
    }
    tmp.AppendPath(pTrack->m_sPropertyPath);

    BuildMapping(iToUse, tIdx, pTrack, m_TopLevelEntries[iToUse], -1, tmp);
  }
}

void plQtPropertyAnimModel::BuildMapping(
  plInt32 iToUse, plInt32 iTrackIdx, plPropertyAnimationTrack* pTrack, plDynamicArray<plInt32>& treeItems, plInt32 iParentEntry, const char* szPath)
{
  const char* szSubPath = plStringUtils::FindSubString(szPath, "/");

  plStringBuilder name, sDisplayString;

  bool bIsComponent = false;
  if (szPath[0] == ':')
  {
    ++szPath;
    bIsComponent = true;
  }

  if (szSubPath != nullptr)
    name.SetSubString_FromTo(szPath, szSubPath);
  else
    name = szPath;

  if (bIsComponent)
    sDisplayString = plTranslate(name);
  else
    sDisplayString = name;

  plInt32 iThisEntry = -1;

  for (plUInt32 i = 0; i < treeItems.GetCount(); ++i)
  {
    if (m_AllEntries[iToUse][treeItems[i]].m_sDisplay.IsEqual_NoCase(sDisplayString))
    {
      iThisEntry = treeItems[i];
      break;
    }
  }

  plQtPropertyAnimModelTreeEntry* pThisEntry = nullptr;

  if (iThisEntry < 0)
  {
    pThisEntry = &m_AllEntries[iToUse].ExpandAndGetRef();
    iThisEntry = m_AllEntries[iToUse].GetCount() - 1;
    treeItems.PushBack(iThisEntry);

    pThisEntry->m_iParent = iParentEntry;
    pThisEntry->m_uiOwnRowIndex = treeItems.GetCount() - 1;
    pThisEntry->m_sDisplay = sDisplayString;

    if (bIsComponent)
    {
      sDisplayString.Set(":/TypeIcons/", name);
      pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(sDisplayString);
    }

    if (iParentEntry >= 0)
    {
      plStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
  else
  {
    pThisEntry = &m_AllEntries[iToUse][iThisEntry];
  }

  if (szSubPath != nullptr)
  {
    szSubPath += 1;
    BuildMapping(iToUse, iTrackIdx, pTrack, pThisEntry->m_Children, iThisEntry, szSubPath);
  }
  else
  {
    pThisEntry->m_iTrackIdx = iTrackIdx;
    pThisEntry->m_pTrack = pTrack;

    switch (pTrack->m_Target)
    {
      case plPropertyAnimTarget::Color:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/ColorGradient.svg");
        break;
      case plPropertyAnimTarget::Number:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/AssetIcons/Curve1D.svg");
        break;
      case plPropertyAnimTarget::VectorX:
      case plPropertyAnimTarget::RotationX:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveX.svg");
        name.Append(".x");
        break;
      case plPropertyAnimTarget::VectorY:
      case plPropertyAnimTarget::RotationY:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveY.svg");
        name.Append(".y");
        break;
      case plPropertyAnimTarget::VectorZ:
      case plPropertyAnimTarget::RotationZ:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveZ.svg");
        name.Append(".z");
        break;
      case plPropertyAnimTarget::VectorW:
        pThisEntry->m_Icon = plQtUiServices::GetSingleton()->GetCachedIconResource(":/EditorPluginAssets/CurveW.svg");
        name.Append(".w");
        break;
    }

    pThisEntry->m_sDisplay = name;

    if (iParentEntry >= 0)
    {
      plStringBuilder tmp = m_AllEntries[iToUse][iParentEntry].m_sPathToItem;
      tmp.AppendPath(name);
      pThisEntry->m_sPathToItem = tmp;
    }
    else
    {
      pThisEntry->m_sPathToItem = name;
    }
  }
}
