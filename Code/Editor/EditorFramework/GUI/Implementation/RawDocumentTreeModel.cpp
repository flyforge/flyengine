#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorFramework/GUI/RawDocumentTreeModel.moc.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <ToolsFoundation/Command/TreeCommands.h>

plQtDocumentTreeModelAdapter::plQtDocumentTreeModelAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* szChildProperty)
  : m_pTree(pTree)
  , m_pType(pType)
  , m_sChildProperty(szChildProperty)
{
  if (!m_sChildProperty.IsEmpty())
  {
    auto pProp = pType->FindPropertyByName(m_sChildProperty);
    PLASMA_ASSERT_DEV(pProp != nullptr && (pProp->GetCategory() == plPropertyCategory::Array || pProp->GetCategory() == plPropertyCategory::Set),
      "The visualized object property tree must either be a set or array!");
    PLASMA_ASSERT_DEV(!pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner),
      "The visualized object must have ownership of the property objects!");
  }
}

const plRTTI* plQtDocumentTreeModelAdapter::GetType() const
{
  return m_pType;
}


const plString& plQtDocumentTreeModelAdapter::GetChildProperty() const
{
  return m_sChildProperty;
}

bool plQtDocumentTreeModelAdapter::setData(const plDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  return false;
}

Qt::ItemFlags plQtDocumentTreeModelAdapter::flags(const plDocumentObject* pObject, int iRow, int iColumn) const
{
  if (iColumn == 0)
  {
    return (Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}


plQtDummyAdapter::plQtDummyAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* szChildProperty)
  : plQtDocumentTreeModelAdapter(pTree, pType, szChildProperty)
{
}

QVariant plQtDummyAdapter::data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        plStringBuilder tmp;
        return QString::fromUtf8(pObject->GetTypeAccessor().GetType()->GetTypeName().GetData(tmp));
      }
      break;
    }
  }
  return QVariant();
}

plQtNamedAdapter::plQtNamedAdapter(const plDocumentObjectManager* pTree, const plRTTI* pType, const char* szChildProperty, const char* szNameProperty)
  : plQtDocumentTreeModelAdapter(pTree, pType, szChildProperty)
  , m_sNameProperty(szNameProperty)
{
  auto pProp = pType->FindPropertyByName(m_sNameProperty);
  PLASMA_ASSERT_DEV(pProp != nullptr && pProp->GetCategory() == plPropertyCategory::Member && pProp->GetSpecificType()->GetVariantType() == plVariantType::String, "The name property must be a string member property.");

  m_pTree->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtNamedAdapter::TreePropertyEventHandler, this));
}

plQtNamedAdapter::~plQtNamedAdapter()
{
  m_pTree->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtNamedAdapter::TreePropertyEventHandler, this));
}

QVariant plQtNamedAdapter::data(const plDocumentObject* pObject, int iRow, int iColumn, int iRole) const
{
  if (iColumn == 0)
  {
    switch (iRole)
    {
      case Qt::DisplayRole:
      case Qt::EditRole:
      {
        return QString::fromUtf8(pObject->GetTypeAccessor().GetValue(m_sNameProperty).ConvertTo<plString>().GetData());
      }
      break;
    }
  }
  return QVariant();
}

void plQtNamedAdapter::TreePropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == m_sNameProperty)
  {
    QVector<int> v;
    v.push_back(Qt::DisplayRole);
    v.push_back(Qt::EditRole);
    Q_EMIT dataChanged(e.m_pObject, v);
  }
}

plQtNameableAdapter::plQtNameableAdapter(
  const plDocumentObjectManager* pTree, const plRTTI* pType, const char* szChildProperty, const char* szNameProperty)
  : plQtNamedAdapter(pTree, pType, szChildProperty, szNameProperty)
{
}

plQtNameableAdapter::~plQtNameableAdapter() = default;

bool plQtNameableAdapter::setData(const plDocumentObject* pObject, int iRow, int iColumn, const QVariant& value, int iRole) const
{
  if (iColumn == 0 && iRole == Qt::EditRole)
  {
    auto pHistory = m_pTree->GetDocument()->GetCommandHistory();

    pHistory->StartTransaction(plFmt("Rename to '{0}'", value.toString().toUtf8().data()));

    plSetObjectPropertyCommand cmd;
    cmd.m_NewValue = value.toString().toUtf8().data();
    cmd.m_Object = pObject->GetGuid();
    cmd.m_sProperty = m_sNameProperty;

    pHistory->AddCommand(cmd).AssertSuccess();

    pHistory->FinishTransaction();

    return true;
  }
  return false;
}

Qt::ItemFlags plQtNameableAdapter::flags(const plDocumentObject* pObject, int iRow, int iColumn) const
{
  if (iColumn == 0)
  {
    return (Qt::ItemIsEditable | Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled);
  }

  return Qt::ItemFlag::NoItemFlags;
}

//////////////////////////////////////////////////////////////////////////

plQtDocumentTreeModel::plQtDocumentTreeModel(const plDocumentObjectManager* pTree, const plUuid& root)
  : QAbstractItemModel(nullptr)
  , m_pDocumentTree(pTree)
  , m_Root(root)
{
  m_pDocumentTree->m_StructureEvents.AddEventHandler(plMakeDelegate(&plQtDocumentTreeModel::TreeEventHandler, this));
}

plQtDocumentTreeModel::~plQtDocumentTreeModel()
{
  m_pDocumentTree->m_StructureEvents.RemoveEventHandler(plMakeDelegate(&plQtDocumentTreeModel::TreeEventHandler, this));
}

void plQtDocumentTreeModel::AddAdapter(plQtDocumentTreeModelAdapter* pAdapter)
{
  PLASMA_ASSERT_DEV(!m_Adapters.Contains(pAdapter->GetType()), "An adapter for the given type was already registered.");

  pAdapter->setParent(this);
  connect(pAdapter, &plQtDocumentTreeModelAdapter::dataChanged, this, [this](const plDocumentObject* pObject, QVector<int> roles) {
    if (!pObject)
      return;
    auto index = ComputeModelIndex(pObject);
    if (!index.isValid())
      return;
    dataChanged(index, index, roles); });
  m_Adapters.Insert(pAdapter->GetType(), pAdapter);
  beginResetModel();
  endResetModel();
}

const plQtDocumentTreeModelAdapter* plQtDocumentTreeModel::GetAdapter(const plRTTI* pType) const
{
  while (pType != nullptr)
  {
    if (const plQtDocumentTreeModelAdapter* const* adapter = m_Adapters.GetValue(pType))
    {
      return *adapter;
    }
    pType = pType->GetParentType();
  }
  return nullptr;
}

void plQtDocumentTreeModel::TreeEventHandler(const plDocumentObjectStructureEvent& e)
{
  const plDocumentObject* pParent = nullptr;
  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::BeforeReset:
      beginResetModel();
      return;
    case plDocumentObjectStructureEvent::Type::AfterReset:
      endResetModel();
      return;
    case plDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
      pParent = e.m_pPreviousParent;
      break;
    case plDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      pParent = e.m_pNewParent;
      break;
  }
  PLASMA_ASSERT_DEV(pParent != nullptr, "Each structure event should have a parent set.");
  if (!IsUnderRoot(pParent))
    return;
  auto pType = pParent->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return;

  if (pAdapter->GetChildProperty() != e.m_sParentProperty)
    return;

  // TODO: BLA root object could have other objects instead of m_pBaseClass, in which case indices are broken on root.

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::BeforeObjectAdded:
    {
      plInt32 iIndex = (plInt32)e.m_NewPropertyIndex.ConvertTo<plInt32>();
      if (e.m_pNewParent == GetRoot())
        beginInsertRows(QModelIndex(), iIndex, iIndex);
      else
        beginInsertRows(ComputeModelIndex(e.m_pNewParent), iIndex, iIndex);
    }
    break;
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    {
      endInsertRows();
    }
    break;
    case plDocumentObjectStructureEvent::Type::BeforeObjectRemoved:
    {
      plInt32 iIndex = ComputeIndex(e.m_pObject);

      beginRemoveRows(ComputeParent(e.m_pObject), iIndex, iIndex);
    }
    break;
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    {
      endRemoveRows();
    }
    break;
    case plDocumentObjectStructureEvent::Type::BeforeObjectMoved:
    {
      plInt32 iNewIndex = (plInt32)e.m_NewPropertyIndex.ConvertTo<plInt32>();
      plInt32 iIndex = ComputeIndex(e.m_pObject);
      beginMoveRows(ComputeModelIndex(e.m_pPreviousParent), iIndex, iIndex, ComputeModelIndex(e.m_pNewParent), iNewIndex);
    }
    break;
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved:
    {
      endMoveRows();
    }
    break;
    default:
      break;
  }
}

QModelIndex plQtDocumentTreeModel::index(int iRow, int iColumn, const QModelIndex& parent) const
{
  const plDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const plDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();
  if (iRow >= pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty()))
    return QModelIndex();

  plVariant value = pObject->GetTypeAccessor().GetValue(pAdapter->GetChildProperty(), iRow);
  PLASMA_ASSERT_DEV(value.IsValid() && value.IsA<plUuid>(), "Tree corruption!");
  const plDocumentObject* pChild = m_pDocumentTree->GetObject(value.Get<plUuid>());
  return createIndex(iRow, iColumn, const_cast<plDocumentObject*>(pChild));
}

plInt32 plQtDocumentTreeModel::ComputeIndex(const plDocumentObject* pObject) const
{
  plInt32 iIndex = pObject->GetPropertyIndex().ConvertTo<plInt32>();
  return iIndex;
}

const plDocumentObject* plQtDocumentTreeModel::GetRoot() const
{
  if (m_Root.IsValid())
  {
    return m_pDocumentTree->GetObject(m_Root);
  }
  return m_pDocumentTree->GetRootObject();
}

bool plQtDocumentTreeModel::IsUnderRoot(const plDocumentObject* pObject) const
{
  const plDocumentObject* pRoot = GetRoot();
  while (pObject)
  {
    if (pRoot == pObject)
      return true;

    pObject = pObject->GetParent();
  }
  return false;
}

QModelIndex plQtDocumentTreeModel::ComputeModelIndex(const plDocumentObject* pObject) const
{
  // Filter out objects that are not under the child property of the
  // parents adapter.
  if (pObject == GetRoot())
    return QModelIndex();

  auto pType = pObject->GetParent()->GetTypeAccessor().GetType();
  auto pAdapter = GetAdapter(pType);
  if (!pAdapter)
    return QModelIndex();

  if (pAdapter->GetChildProperty() != pObject->GetParentProperty())
    return QModelIndex();

  return index(ComputeIndex(pObject), 0, ComputeParent(pObject));
}


void plQtDocumentTreeModel::SetAllowDragDrop(bool bAllow)
{
  m_bAllowDragDrop = bAllow;
}

QModelIndex plQtDocumentTreeModel::ComputeParent(const plDocumentObject* pObject) const
{
  const plDocumentObject* pParent = pObject->GetParent();

  if (pParent == GetRoot())
    return QModelIndex();

  plInt32 iIndex = ComputeIndex(pParent);

  return createIndex(iIndex, 0, const_cast<plDocumentObject*>(pParent));
}

QModelIndex plQtDocumentTreeModel::parent(const QModelIndex& child) const
{
  const plDocumentObject* pObject = (const plDocumentObject*)child.internalPointer();

  return ComputeParent(pObject);
}

int plQtDocumentTreeModel::rowCount(const QModelIndex& parent) const
{
  int iCount = 0;
  const plDocumentObject* pObject = nullptr;
  if (!parent.isValid())
  {
    pObject = GetRoot();
  }
  else
  {
    pObject = (const plDocumentObject*)parent.internalPointer();
  }

  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    if (!pAdapter->GetChildProperty().IsEmpty())
    {
      iCount = pObject->GetTypeAccessor().GetCount(pAdapter->GetChildProperty());
    }
  }

  return iCount;
}

int plQtDocumentTreeModel::columnCount(const QModelIndex& parent) const
{
  return 1;
}

QVariant plQtDocumentTreeModel::data(const QModelIndex& index, int iRole) const
{
  // if (index.isValid())
  {
    const plDocumentObject* pObject = (const plDocumentObject*)index.internalPointer();
    auto pType = pObject->GetTypeAccessor().GetType();
    if (auto pAdapter = GetAdapter(pType))
    {
      return pAdapter->data(pObject, index.row(), index.column(), iRole);
    }
  }

  return QVariant();
}

Qt::DropActions plQtDocumentTreeModel::supportedDropActions() const
{
  if (m_bAllowDragDrop)
    return Qt::MoveAction | Qt::CopyAction;

  return Qt::IgnoreAction;
}

Qt::ItemFlags plQtDocumentTreeModel::flags(const QModelIndex& index) const
{
  if (!index.isValid())
    return Qt::ItemIsDropEnabled;

  const plDocumentObject* pObject = (const plDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->flags(pObject, index.row(), index.column());
  }

  return Qt::ItemFlag::NoItemFlags;
}


bool plQtDocumentTreeModel::canDropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent) const
{
  const plDocumentObject* pNewParent = (const plDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  plDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData = pData;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());

  if (plDragDropHandler::CanDropOnly(&info))
    return true;

  {
    // Test 'CanMove' of the target object manager.
    QByteArray encodedData = pData->data("application/plEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    plHybridArray<plDocumentObject*, 32> Dragged;
    stream >> Dragged;

    auto pType = pNewParent->GetTypeAccessor().GetType();
    auto pAdapter = GetAdapter(pType);
    const plString& sProperty = pAdapter->GetChildProperty();
    for (const plDocumentObject* pItem : Dragged)
    {
      // If the item's and the target tree's document don't match we can't operate via this code.
      if (pItem->GetDocumentObjectManager()->GetDocument() != m_pDocumentTree->GetDocument())
        return false;
      if (m_pDocumentTree->CanMove(pItem, pNewParent, sProperty, info.m_iTargetObjectInsertChildIndex).Failed())
        return false;
    }
    return QAbstractItemModel::canDropMimeData(pData, action, iRow, iColumn, parent);
  }
  return false;
}

bool plQtDocumentTreeModel::dropMimeData(const QMimeData* pData, Qt::DropAction action, int iRow, int iColumn, const QModelIndex& parent)
{
  if (!m_bAllowDragDrop)
    return false;

  if (iColumn > 0)
    return false;

  const plDocumentObject* pNewParent = (const plDocumentObject*)parent.internalPointer();
  if (!pNewParent)
    pNewParent = GetRoot();

  plDragDropInfo info;
  info.m_iTargetObjectInsertChildIndex = iRow;
  info.m_pMimeData = pData;
  info.m_sTargetContext = m_sTargetContext;
  info.m_TargetDocument = m_pDocumentTree->GetDocument()->GetGuid();
  info.m_TargetObject = pNewParent->GetGuid();
  info.m_bCtrlKeyDown = QApplication::queryKeyboardModifiers() & Qt::ControlModifier;
  info.m_bShiftKeyDown = QApplication::queryKeyboardModifiers() & Qt::ShiftModifier;
  info.m_pAdapter = GetAdapter(pNewParent->GetType());
  if (plDragDropHandler::DropOnly(&info))
    return true;

  return plQtDocumentTreeModel::MoveObjects(info);
}


bool plQtDocumentTreeModel::MoveObjects(const plDragDropInfo& info)
{
  if (info.m_pMimeData->hasFormat("application/plEditor.ObjectSelection"))
  {
    auto pDoc = plDocumentManager::GetDocumentByGuid(info.m_TargetDocument);
    const plDocumentObject* pTarget = pDoc->GetObjectManager()->GetObject(info.m_TargetObject);
    PLASMA_ASSERT_DEBUG(pTarget != nullptr, "object from info should always be valid");

    QByteArray encodedData = info.m_pMimeData->data("application/plEditor.ObjectSelection");
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    plHybridArray<plDocumentObject*, 32> Dragged;
    stream >> Dragged;

    for (const plDocumentObject* pDocObject : Dragged)
    {
      // if (action != Qt::DropAction::MoveAction)
      {
        bool bCanMove = true;
        const plDocumentObject* pCurParent = pTarget;

        while (pCurParent)
        {
          if (pCurParent == pDocObject)
          {
            bCanMove = false;
            break;
          }

          pCurParent = pCurParent->GetParent();
        }

        if (!bCanMove)
        {
          plQtUiServices::MessageBoxInformation("Cannot move an object to one of its own children");
          return false;
        }
      }
    }

    auto pHistory = pDoc->GetCommandHistory();
    pHistory->StartTransaction("Reparent Object");

    plStatus res(PLASMA_SUCCESS);
    for (plUInt32 i = 0; i < Dragged.GetCount(); ++i)
    {
      plMoveObjectCommand cmd;
      cmd.m_Object = Dragged[i]->GetGuid();
      cmd.m_Index = info.m_iTargetObjectInsertChildIndex;
      cmd.m_sParentProperty = info.m_pAdapter->GetChildProperty();
      cmd.m_NewParent = pTarget->GetGuid();

      res = pHistory->AddCommand(cmd);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      pHistory->CancelTransaction();
    else
      pHistory->FinishTransaction();

    plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node move failed.");
    return true;
  }

  return false;
}

QStringList plQtDocumentTreeModel::mimeTypes() const
{
  QStringList types;
  if (m_bAllowDragDrop)
  {
    types << "application/plEditor.ObjectSelection";
  }

  return types;
}

QMimeData* plQtDocumentTreeModel::mimeData(const QModelIndexList& indexes) const
{
  if (!m_bAllowDragDrop)
    return nullptr;

  plHybridArray<void*, 1> ptrs;
  for (const QModelIndex& index : indexes)
  {
    if (index.isValid())
    {
      void* pObject = index.internalPointer();
      ptrs.PushBack(pObject);
    }
  }

  QByteArray encodedData;
  QDataStream stream(&encodedData, QIODevice::WriteOnly);
  stream << ptrs;

  QMimeData* mimeData = new QMimeData();
  mimeData->setData("application/plEditor.ObjectSelection", encodedData);
  return mimeData;
}

bool plQtDocumentTreeModel::setData(const QModelIndex& index, const QVariant& value, int iRole)
{
  const plDocumentObject* pObject = (const plDocumentObject*)index.internalPointer();
  auto pType = pObject->GetTypeAccessor().GetType();
  if (auto pAdapter = GetAdapter(pType))
  {
    return pAdapter->setData(pObject, index.row(), index.column(), value, iRole);
  }

  return false;
}
