#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/World/GameObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>

plQtGameObjectAdapter::plQtGameObjectAdapter(plDocumentObjectManager* pObjectManager, plObjectMetaData<plUuid, plDocumentObjectMetaData>* pObjectMetaData, plObjectMetaData<plUuid, plGameObjectMetaData>* pGameObjectMetaData)
  : plQtNameableAdapter(pObjectManager, plGetStaticRTTI<plGameObject>(), "Children", "Name")
{
  m_pObjectManager = pObjectManager;
  m_pGameObjectDocument = plDynamicCast<plGameObjectDocument*>(pObjectManager->GetDocument());

  m_pObjectMetaData = pObjectMetaData;
  if (!m_pObjectMetaData)
    m_pObjectMetaData = m_pGameObjectDocument->m_DocumentObjectMetaData.Borrow();

  m_pGameObjectMetaData = pGameObjectMetaData;
  if (!m_pGameObjectMetaData)
    m_pGameObjectMetaData = m_pGameObjectDocument->m_GameObjectMetaData.Borrow();

  m_GameObjectMetaDataSubscription = m_pGameObjectMetaData->m_DataModifiedEvent.AddEventHandler(
    plMakeDelegate(&plQtGameObjectAdapter::GameObjectMetaDataEventHandler, this));
  m_DocumentObjectMetaDataSubscription = m_pObjectMetaData->m_DataModifiedEvent.AddEventHandler(
    plMakeDelegate(&plQtGameObjectAdapter::DocumentObjectMetaDataEventHandler, this));
}

plQtGameObjectAdapter::~plQtGameObjectAdapter()
{
  m_pGameObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(m_GameObjectMetaDataSubscription);
  m_pObjectMetaData->m_DataModifiedEvent.RemoveEventHandler(m_DocumentObjectMetaDataSubscription);
}

QVariant plQtGameObjectAdapter::data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  switch (iRole)
  {
    case Qt::DisplayRole:
    {
      plStringBuilder sName;
      plUuid prefabGuid;
      QIcon icon;

      m_pGameObjectDocument->QueryCachedNodeName(pObject, sName, &prefabGuid, &icon);

      const QString sQtName = QString::fromUtf8(sName.GetData());

      if (prefabGuid.IsValid())
        return QStringLiteral("[") + sQtName + QStringLiteral("]");

      return sQtName;
    }
    break;

    case Qt::DecorationRole:
    {
      plStringBuilder sName;
      plUuid prefabGuid;
      QIcon icon;

      m_pGameObjectDocument->QueryCachedNodeName(pObject, sName, &prefabGuid, &icon);
      return icon;
    }
    break;

    case Qt::EditRole:
    {
      plStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();

      if (sName.IsEmpty())
      {
        auto pMeta = m_pGameObjectMetaData->BeginReadMetaData(pObject->GetGuid());
        sName = pMeta->m_CachedNodeName;
        m_pGameObjectMetaData->EndReadMetaData();
      }

      return QString::fromUtf8(sName.GetData());
    }
    break;

    case Qt::ToolTipRole:
    {
      auto pMeta = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const plUuid prefab = pMeta->m_CreateFromPrefab;
      m_pObjectMetaData->EndReadMetaData();

      if (prefab.IsValid())
      {
        auto pInfo = plAssetCurator::GetSingleton()->GetSubAsset(prefab);

        if (pInfo)
          return plMakeQString(pInfo->m_pAssetInfo->m_Path.GetDataDirParentRelativePath());

        return QStringLiteral("Prefab asset could not be found");
      }
    }
    break;

    case Qt::FontRole:
    {
      auto pMeta = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const bool bHidden = pMeta->m_bHidden;
      m_pObjectMetaData->EndReadMetaData();

      const bool bHasName = !pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>().IsEmpty();

      if (bHidden || bHasName)
      {
        QFont font;

        if (bHidden)
          font.setStrikeOut(true);
        if (bHasName)
          font.setBold(true);

        return font;
      }
    }
    break;

    case Qt::ForegroundRole:
    {
      plStringBuilder sName = pObject->GetTypeAccessor().GetValue("Name").ConvertTo<plString>();

      auto pMeta = m_pObjectMetaData->BeginReadMetaData(pObject->GetGuid());
      const bool bPrefab = pMeta->m_CreateFromPrefab.IsValid();
      m_pObjectMetaData->EndReadMetaData();

      if (bPrefab)
      {
        return plToQtColor(plColorScheme::LightUI(plColorScheme::Blue));
      }

      if (sName.IsEmpty())
      {
        return QVariant();
      }
    }
    break;
  }

  return plQtNameableAdapter::data(pObject, iRow, iColumn, iRole);
}

bool plQtGameObjectAdapter::setData(const plDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  if (iRole == Qt::EditRole)
  {
    auto pMetaWrite = m_pGameObjectMetaData->BeginModifyMetaData(pObject->GetGuid());

    plStringBuilder sNewValue = value.toString().toUtf8().data();

    const plStringBuilder sOldValue = pMetaWrite->m_CachedNodeName;

    // pMetaWrite->m_CachedNodeName.Clear();
    m_pGameObjectMetaData->EndModifyMetaData(0); // no need to broadcast this change

    if (sOldValue == sNewValue && !sOldValue.IsEmpty())
      return false;

    sNewValue.Trim("[]{}() \t\r"); // forbid these

    return plQtNameableAdapter::setData(pObject, iRow, iColumn, QString::fromUtf8(sNewValue.GetData()), iRole);
  }

  return false;
}

void plQtGameObjectAdapter::DocumentObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plDocumentObjectMetaData>::EventData& e)
{
  if ((e.m_uiModifiedFlags & (plDocumentObjectMetaData::HiddenFlag | plDocumentObjectMetaData::PrefabFlag)) == 0)
    return;

  auto pObject = m_pObjectManager->GetObject(e.m_ObjectKey);

  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(pObject, v);
}

void plQtGameObjectAdapter::GameObjectMetaDataEventHandler(const plObjectMetaData<plUuid, plGameObjectMetaData>::EventData& e)
{
  if (e.m_uiModifiedFlags == 0)
    return;

  auto pObject = m_pObjectManager->GetObject(e.m_ObjectKey);

  if (pObject == nullptr)
  {
    // The object was destroyed due to a clear of the redo queue, i.e. it is not contained in the scene anymore.
    // So we of course won't find it in the model and thus can skip it.
    return;
  }

  // ignore all components etc.
  if (!pObject->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
    return;

  QVector<int> v;
  v.push_back(Qt::FontRole);
  dataChanged(pObject, v);
}

plQtGameObjectModel::plQtGameObjectModel(const plDocumentObjectManager* pObjectManager, const plUuid& root)
  : plQtDocumentTreeModel(pObjectManager, root)
{
}

plQtGameObjectModel::~plQtGameObjectModel() = default;
