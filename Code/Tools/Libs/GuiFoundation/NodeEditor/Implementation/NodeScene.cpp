#include <GuiFoundation/GuiFoundationPCH.h>

#include <Foundation/Strings/TranslationLookup.h>
#include <GuiFoundation/NodeEditor/Connection.h>
#include <GuiFoundation/NodeEditor/Node.h>
#include <GuiFoundation/NodeEditor/Pin.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>
#include <GuiFoundation/Widgets/SearchableMenu.moc.h>
#include <QGraphicsSceneMouseEvent>
#include <QMenu>
#include <ToolsFoundation/Command/NodeCommands.h>
#include <ToolsFoundation/Command/TreeCommands.h>

plRttiMappedObjectFactory<plQtNode> plQtNodeScene::s_NodeFactory;
plRttiMappedObjectFactory<plQtPin> plQtNodeScene::s_PinFactory;
plRttiMappedObjectFactory<plQtConnection> plQtNodeScene::s_ConnectionFactory;
plVec2 plQtNodeScene::s_vLastMouseInteraction(0);

plQtNodeScene::plQtNodeScene(QObject* parent)
  : QGraphicsScene(parent)
{
  setItemIndexMethod(QGraphicsScene::NoIndex);

  connect(this, &QGraphicsScene::selectionChanged, this, &plQtNodeScene::OnSelectionChanged);
}

plQtNodeScene::~plQtNodeScene()
{
  disconnect(this, &QGraphicsScene::selectionChanged, this, &plQtNodeScene::OnSelectionChanged);

  Clear();

  if (m_pManager != nullptr)
  {
    m_pManager->m_NodeEvents.RemoveEventHandler(plMakeDelegate(&plQtNodeScene::NodeEventsHandler, this));
    m_pManager->GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtNodeScene::SelectionEventsHandler, this));
    m_pManager->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtNodeScene::PropertyEventsHandler, this));
  }
}

void plQtNodeScene::InitScene(const plDocumentNodeManager* pManager)
{
  PLASMA_ASSERT_DEV(pManager != nullptr, "Invalid node manager");

  m_pManager = pManager;

  m_pManager->m_NodeEvents.AddEventHandler(plMakeDelegate(&plQtNodeScene::NodeEventsHandler, this));
  m_pManager->GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtNodeScene::SelectionEventsHandler, this));
  m_pManager->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtNodeScene::PropertyEventsHandler, this));

  // Create Nodes
  const auto& rootObjects = pManager->GetRootObject()->GetChildren();
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsNode(pObject))
    {
      CreateQtNode(pObject);
    }
  }
  for (const auto& pObject : rootObjects)
  {
    if (pManager->IsConnection(pObject))
    {
      CreateQtConnection(pObject);
    }
  }
}

const plDocument* plQtNodeScene::GetDocument() const
{
  return m_pManager->GetDocument();
}

const plDocumentNodeManager* plQtNodeScene::GetDocumentNodeManager() const
{
  return m_pManager;
}

plRttiMappedObjectFactory<plQtNode>& plQtNodeScene::GetNodeFactory()
{
  return s_NodeFactory;
}

plRttiMappedObjectFactory<plQtPin>& plQtNodeScene::GetPinFactory()
{
  return s_PinFactory;
}

plRttiMappedObjectFactory<plQtConnection>& plQtNodeScene::GetConnectionFactory()
{
  return s_ConnectionFactory;
}

void plQtNodeScene::SetConnectionStyle(plEnum<ConnectionStyle> style)
{
  m_ConnectionStyle = style;
  invalidate();
}

void plQtNodeScene::SetConnectionDecorationFlags(plBitflags<ConnectionDecorationFlags> flags)
{
  m_ConnectionDecorationFlags = flags;
  invalidate();
}

void plQtNodeScene::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
  m_vMousePos = plVec2(event->scenePos().x(), event->scenePos().y());
  s_vLastMouseInteraction = m_vMousePos;

  if (m_pTempConnection)
  {
    event->accept();

    plVec2 bestPos = m_vMousePos;

    // snap to the closest pin that we can connect to
    if (!m_ConnectablePins.IsEmpty())
    {
      const float fPinSize = m_ConnectablePins[0]->sceneBoundingRect().height();

      // this is also the threshold at which we snap to another position
      float fDistToBest = plMath::Square(fPinSize * 2.5f);

      for (auto pin : m_ConnectablePins)
      {
        const QPointF center = pin->sceneBoundingRect().center();
        const plVec2 pt = plVec2(center.x(), center.y());
        const float lenSqr = (pt - s_vLastMouseInteraction).GetLengthSquared();

        if (lenSqr < fDistToBest)
        {
          fDistToBest = lenSqr;
          bestPos = pt;
        }
      }
    }

    if (m_pStartPin->GetPin()->GetType() == plPin::Type::Input)
    {
      m_pTempConnection->SetPosOut(QPointF(bestPos.x, bestPos.y));
    }
    else
    {
      m_pTempConnection->SetPosIn(QPointF(bestPos.x, bestPos.y));
    }
    return;
  }

  QGraphicsScene::mouseMoveEvent(event);
}

void plQtNodeScene::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
  switch (event->button())
  {
    case Qt::LeftButton:
    {
      QList<QGraphicsItem*> itemList = items(event->scenePos(), Qt::IntersectsItemBoundingRect);
      for (QGraphicsItem* item : itemList)
      {
        if (item->type() != Type::Pin)
          continue;

        event->accept();
        plQtPin* pPin = static_cast<plQtPin*>(item);
        m_pStartPin = pPin;
        m_pTempConnection = new plQtConnection(nullptr);
        addItem(m_pTempConnection);
        m_pTempConnection->SetPosIn(pPin->GetPinPos());
        m_pTempConnection->SetPosOut(pPin->GetPinPos());

        if (pPin->GetPin()->GetType() == plPin::Type::Input)
        {
          m_pTempConnection->SetDirIn(pPin->GetPinDir());
          m_pTempConnection->SetDirOut(-pPin->GetPinDir());
        }
        else
        {
          m_pTempConnection->SetDirIn(-pPin->GetPinDir());
          m_pTempConnection->SetDirOut(pPin->GetPinDir());
        }

        MarkupConnectablePins(pPin);
        return;
      }
    }
    break;
    case Qt::RightButton:
    {
      event->accept();
      return;
    }

    default:
      break;
  }

  QGraphicsScene::mousePressEvent(event);
}

void plQtNodeScene::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
  if (m_pTempConnection && event->button() == Qt::LeftButton)
  {
    event->accept();

    const bool startWasInput = m_pStartPin->GetPin()->GetType() == plPin::Type::Input;
    const QPointF releasePos = startWasInput ? m_pTempConnection->GetOutPos() : m_pTempConnection->GetInPos();

    QList<QGraphicsItem*> itemList = items(releasePos, Qt::IntersectsItemBoundingRect);
    for (QGraphicsItem* item : itemList)
    {
      if (item->type() != Type::Pin)
        continue;

      plQtPin* pPin = static_cast<plQtPin*>(item);
      if (pPin != m_pStartPin && pPin->GetPin()->GetType() != m_pStartPin->GetPin()->GetType())
      {
        const plPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const plPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        ConnectPinsAction(*pSourcePin, *pTargetPin);
        goto cleanup;
      }
    }

    OpenSearchMenu(QCursor::pos());

    if (m_pTempNode)
    {
      const auto Pins = startWasInput ? m_pTempNode->GetOutputPins() : m_pTempNode->GetInputPins();

      for (auto& pPin : Pins)
      {
        const plPin* pSourcePin = startWasInput ? pPin->GetPin() : m_pStartPin->GetPin();
        const plPin* pTargetPin = startWasInput ? m_pStartPin->GetPin() : pPin->GetPin();
        plDocumentNodeManager::CanConnectResult connect;
        plStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), *pSourcePin, *pTargetPin, connect);
        if (res.Succeeded())
        {
          ConnectPinsAction(*pSourcePin, *pTargetPin);
          break;
        }
      }
    }

cleanup:

    delete m_pTempConnection;
    m_pTempConnection = nullptr;
    m_pStartPin = nullptr;
    m_pTempNode = nullptr;

    ResetConnectablePinMarkup();
    return;
  }

  QGraphicsScene::mouseReleaseEvent(event);

  plSet<const plDocumentObject*> moved;
  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetFlags().IsSet(plNodeFlags::Moved))
    {
      moved.Insert(it.Key());
      it.Value()->ResetFlags();
    }
  }

  if (!moved.IsEmpty())
  {
    plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Move Node");

    plStatus res;
    for (auto pObject : moved)
    {
      plMoveNodeCommand move;
      move.m_Object = pObject->GetGuid();
      auto pos = m_Nodes[pObject]->pos();
      move.m_NewPos = plVec2(pos.x(), pos.y());
      res = history->AddCommand(move);
      if (res.m_Result.Failed())
        break;
    }

    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();

    plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Move node failed");
  }
}

void plQtNodeScene::contextMenuEvent(QGraphicsSceneContextMenuEvent* contextMenuEvent)
{
  QTransform id;

  QGraphicsItem* pItem = itemAt(contextMenuEvent->scenePos(), id);
  int iType = pItem != nullptr ? pItem->type() : -1;
  while (pItem && !(iType >= Type::Node && iType <= Type::Connection))
  {
    pItem = pItem->parentItem();
    iType = pItem != nullptr ? pItem->type() : -1;
  }

  QMenu menu;
  if (iType == Type::Pin)
  {
    plQtPin* pPin = static_cast<plQtPin*>(pItem);
    QAction* pAction = new QAction("Disconnect Pin", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pPin](bool bChecked)
    { 
      DisconnectPinsAction(pPin); 
    });

    pPin->ExtendContextMenu(menu);
  }
  else if (iType == Type::Node)
  {
    plQtNode* pNode = static_cast<plQtNode*>(pItem);

    // if we clicked on an unselected item, make it the only selected item
    if (!pNode->isSelected())
    {
      clearSelection();
      pNode->setSelected(true);
    }

    // Delete Node
    {
      QAction* pAction = new QAction("Remove", &menu);
      menu.addAction(pAction);
      connect(pAction, &QAction::triggered, this, [this](bool bChecked) 
      { 
        RemoveSelectedNodesAction(); 
      });
    }

    pNode->ExtendContextMenu(menu);
  }
  else if (iType == Type::Connection)
  {
    plQtConnection* pConnection = static_cast<plQtConnection*>(pItem);
    QAction* pAction = new QAction("Delete Connection", &menu);
    menu.addAction(pAction);
    connect(pAction, &QAction::triggered, this, [this, pConnection](bool bChecked) { DisconnectPinsAction(pConnection); });

    pConnection->ExtendContextMenu(menu);
  }
  else
  {
    OpenSearchMenu(contextMenuEvent->screenPos());
    return;
  }

  menu.exec(contextMenuEvent->screenPos());
}

void plQtNodeScene::keyPressEvent(QKeyEvent* event)
{
  QTransform id;
  QGraphicsItem* pItem = itemAt(QPointF(m_vMousePos.x, m_vMousePos.y), id);
  if (pItem && pItem->type() == Type::Pin)
  {
    plQtPin* pin = static_cast<plQtPin*>(pItem);
    if (event->key() == Qt::Key_Delete)
    {
      DisconnectPinsAction(pin);
    }

    pin->keyPressEvent(event);
  }

  if (event->key() == Qt::Key_Delete)
  {
    RemoveSelectedNodesAction();
  }
  else if (event->key() == Qt::Key_Space)
  {
    OpenSearchMenu(QCursor::pos());
  }
}

void plQtNodeScene::Clear()
{
  while (!m_Connections.IsEmpty())
  {
    DeleteQtConnection(m_Connections.GetIterator().Key());
  }

  while (!m_Nodes.IsEmpty())
  {
    DeleteQtNode(m_Nodes.GetIterator().Key());
  }
}

void plQtNodeScene::CreateQtNode(const plDocumentObject* pObject)
{
  plVec2 vPos = m_pManager->GetNodePos(pObject);

  plQtNode* pNode = s_NodeFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pNode == nullptr)
  {
    pNode = new plQtNode();
  }
  m_Nodes[pObject] = pNode;
  addItem(pNode);
  pNode->InitNode(m_pManager, pObject);
  pNode->setPos(vPos.x, vPos.y);

  pNode->ResetFlags();


  // Note: We dont create connections here as it can cause recusion issues
  if (m_pTempConnection)
  {
    m_pTempNode = pNode;
  }
}

void plQtNodeScene::DeleteQtNode(const plDocumentObject* pObject)
{
  plQtNode* pNode = m_Nodes[pObject];
  m_Nodes.Remove(pObject);

  removeItem(pNode);
  delete pNode;
}

void plQtNodeScene::CreateQtConnection(const plDocumentObject* pObject)
{
  const plConnection& connection = m_pManager->GetConnection(pObject);
  const plPin& pinSource = connection.GetSourcePin();
  const plPin& pinTarget = connection.GetTargetPin();

  plQtNode* pSource = m_Nodes[pinSource.GetParent()];
  plQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  if (!pTarget || !pSource)
    return;

  plQtPin* pOutput = pSource->GetOutputPin(pinSource);
  plQtPin* pInput = pTarget->GetInputPin(pinTarget);
  PLASMA_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  plQtConnection* pQtConnection = s_ConnectionFactory.CreateObject(pObject->GetTypeAccessor().GetType());
  if (pQtConnection == nullptr)
  {
    pQtConnection = new plQtConnection(nullptr);
  }

  addItem(pQtConnection);
  pQtConnection->InitConnection(pObject, &connection);
  pOutput->AddConnection(pQtConnection);
  pInput->AddConnection(pQtConnection);
  m_Connections[pObject] = pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void plQtNodeScene::DeleteQtConnection(const plDocumentObject* pObject)
{
  plQtConnection* pQtConnection = m_Connections[pObject];
  if (!pQtConnection)
    return;
  m_Connections.Remove(pObject);

  const plConnection* pConnection = pQtConnection->GetConnection();
  PLASMA_ASSERT_DEV(pConnection != nullptr, "No connection");

  const plPin& pinSource = pConnection->GetSourcePin();
  const plPin& pinTarget = pConnection->GetTargetPin();

  plQtNode* pSource = m_Nodes[pinSource.GetParent()];
  plQtNode* pTarget = m_Nodes[pinTarget.GetParent()];
  plQtPin* pOutput = pSource->GetOutputPin(pinSource);
  plQtPin* pInput = pTarget->GetInputPin(pinTarget);
  PLASMA_ASSERT_DEV(pOutput != nullptr && pInput != nullptr, "Node does not contain pin!");

  pOutput->RemoveConnection(pQtConnection);
  pInput->RemoveConnection(pQtConnection);

  removeItem(pQtConnection);
  delete pQtConnection;

  // reset flags to update the node's title to reflect connection changes
  pSource->ResetFlags();
  pTarget->ResetFlags();
}

void plQtNodeScene::RecreateQtPins(const plDocumentObject* pObject)
{
  plQtNode* pNode = m_Nodes[pObject];
  pNode->CreatePins();
  pNode->UpdateState();
  pNode->UpdateGeometry();
}

void plQtNodeScene::CreateNodeObject(const plRTTI* pRtti)
{
  plCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Add Node");

  plStatus res;
  {
    plAddObjectCommand cmd;
    cmd.m_pType = pRtti;
    cmd.m_NewObjectGuid.CreateNewUuid();
    cmd.m_Index = -1;

    res = history->AddCommand(cmd);
    if (res.m_Result.Succeeded())
    {
      plMoveNodeCommand move;
      move.m_Object = cmd.m_NewObjectGuid;
      move.m_NewPos = m_vMousePos;
      res = history->AddCommand(move);
    }
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void plQtNodeScene::NodeEventsHandler(const plDocumentNodeManagerEvent& e)
{
  switch (e.m_EventType)
  {
    case plDocumentNodeManagerEvent::Type::NodeMoved:
    {
      plVec2 vPos = m_pManager->GetNodePos(e.m_pObject);
      plQtNode* pNode = m_Nodes[e.m_pObject];
      pNode->setPos(vPos.x, vPos.y);
    }
    break;
    case plDocumentNodeManagerEvent::Type::AfterPinsConnected:
      CreateQtConnection(e.m_pObject);
      break;

    case plDocumentNodeManagerEvent::Type::BeforePinsDisonnected:
      DeleteQtConnection(e.m_pObject);
      break;

    case plDocumentNodeManagerEvent::Type::BeforePinsChanged:
      break;

    case plDocumentNodeManagerEvent::Type::AfterPinsChanged:
      RecreateQtPins(e.m_pObject);
      break;

    case plDocumentNodeManagerEvent::Type::AfterNodeAdded:
      CreateQtNode(e.m_pObject);
      break;

    case plDocumentNodeManagerEvent::Type::BeforeNodeRemoved:
      DeleteQtNode(e.m_pObject);
      break;

    default:
      break;
  }
}

void plQtNodeScene::PropertyEventsHandler(const plDocumentObjectPropertyEvent& e)
{
  auto it = m_Nodes.Find(e.m_pObject);

  if (it.IsValid())
  {
    it.Value()->ResetFlags();
    it.Value()->update();
  }
}

void plQtNodeScene::SelectionEventsHandler(const plSelectionManagerEvent& e)
{
  const plDeque<const plDocumentObject*>& selection = GetDocument()->GetSelectionManager()->GetSelection();

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;

    clearSelection();

    QList<QGraphicsItem*> qSelection;
    for (const plDocumentObject* pObject : selection)
    {
      auto it = m_Nodes.Find(pObject);
      if (!it.IsValid())
        continue;

      it.Value()->setSelected(true);
    }
    m_bIgnoreSelectionChange = false;
  }

  bool bAnyPaintChanges = false;

  for (auto itCon : m_Connections)
  {
    auto pQtCon = itCon.Value();
    if (!pQtCon)
      continue;
    auto pCon = pQtCon->GetConnection();

    const bool prev = pQtCon->m_bAdjacentNodeSelected;

    pQtCon->m_bAdjacentNodeSelected = false;

    for (const plDocumentObject* pObject : selection)
    {
      if (pCon->GetSourcePin().GetParent() == pObject || pCon->GetTargetPin().GetParent() == pObject)
      {
        pQtCon->m_bAdjacentNodeSelected = true;
        break;
      }
    }

    if (prev != pQtCon->m_bAdjacentNodeSelected)
    {
      bAnyPaintChanges = true;
    }
  }

  if (bAnyPaintChanges)
  {
    invalidate();
  }
}

void plQtNodeScene::GetSelectedNodes(plDeque<plQtNode*>& selection) const
{
  selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == plQtNodeScene::Node)
    {
      plQtNode* pNode = static_cast<plQtNode*>(pItem);
      selection.PushBack(pNode);
    }
  }
}

void plQtNodeScene::MarkupConnectablePins(plQtPin* pQtSourcePin)
{
  m_ConnectablePins.Clear();

  const plRTTI* pConnectionType = m_pManager->GetConnectionType();

  const plPin* pSourcePin = pQtSourcePin->GetPin();
  const bool bConnectForward = pSourcePin->GetType() == plPin::Type::Output;

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const plDocumentObject* pDocObject = it.Key();
    plQtNode* pTargetNode = it.Value();

    {
      auto pinArray = bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        plQtPin* pQtTargetPin = bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);

        plDocumentNodeManager::CanConnectResult res;

        if (bConnectForward)
          m_pManager->CanConnect(pConnectionType, *pSourcePin, *pin, res).IgnoreResult();
        else
          m_pManager->CanConnect(pConnectionType, *pin, *pSourcePin, res).IgnoreResult();

        if (res == plDocumentNodeManager::CanConnectResult::ConnectNever)
        {
          pQtTargetPin->SetHighlightState(plQtPinHighlightState::CannotConnect);
        }
        else
        {
          m_ConnectablePins.PushBack(pQtTargetPin);

          if (res == plDocumentNodeManager::CanConnectResult::Connect1toN || res == plDocumentNodeManager::CanConnectResult::ConnectNtoN)
          {
            pQtTargetPin->SetHighlightState(plQtPinHighlightState::CanAddConnection);
          }
          else
          {
            pQtTargetPin->SetHighlightState(plQtPinHighlightState::CanReplaceConnection);
          }
        }
      }
    }

    {
      auto pinArray = !bConnectForward ? m_pManager->GetInputPins(pDocObject) : m_pManager->GetOutputPins(pDocObject);

      for (auto& pin : pinArray)
      {
        plQtPin* pQtTargetPin = !bConnectForward ? pTargetNode->GetInputPin(*pin) : pTargetNode->GetOutputPin(*pin);
        pQtTargetPin->SetHighlightState(plQtPinHighlightState::CannotConnectSameDirection);
      }
    }
  }
}

void plQtNodeScene::ResetConnectablePinMarkup()
{
  m_ConnectablePins.Clear();

  for (auto it = m_Nodes.GetIterator(); it.IsValid(); ++it)
  {
    const plDocumentObject* pDocObject = it.Key();
    plQtNode* pTargetNode = it.Value();

    for (auto& pin : m_pManager->GetInputPins(pDocObject))
    {
      plQtPin* pQtTargetPin = pTargetNode->GetInputPin(*pin);
      pQtTargetPin->SetHighlightState(plQtPinHighlightState::None);
    }

    for (auto& pin : m_pManager->GetOutputPins(pDocObject))
    {
      plQtPin* pQtTargetPin = pTargetNode->GetOutputPin(*pin);
      pQtTargetPin->SetHighlightState(plQtPinHighlightState::None);
    }
  }
}

void plQtNodeScene::OpenSearchMenu(QPoint screenPos)
{
  QMenu menu;
  plQtSearchableMenu* pSearchMenu = new plQtSearchableMenu(&menu);
  menu.addAction(pSearchMenu);

  connect(pSearchMenu, &plQtSearchableMenu::MenuItemTriggered, this, &plQtNodeScene::OnMenuItemTriggered);
  connect(pSearchMenu, &plQtSearchableMenu::MenuItemTriggered, this, [&menu]() { menu.close(); });

  plStringBuilder sFullName, sCleanName2;

  plHybridArray<const plRTTI*, 32> types;
  m_pManager->GetCreateableTypes(types);

  plStringBuilder tmp;

  for (const plRTTI* pRtti : types)
  {
    plStringView sCleanName = pRtti->GetTypeName();

    const char* szColonColon = sCleanName.FindLastSubString("::");
    if (szColonColon != nullptr)
      sCleanName.SetStartPosition(szColonColon + 2);

    const char* szUnderscore = sCleanName.FindLastSubString("_");
    if (szUnderscore != nullptr)
      sCleanName.SetStartPosition(szUnderscore + 1);

    sCleanName2 = sCleanName;
    if (const char* szBracket = sCleanName2.FindLastSubString("<"))
    {
      sCleanName2.SetSubString_FromTo(sCleanName2.GetData(), szBracket);
    }

    sFullName = m_pManager->GetTypeCategory(pRtti);

    if (sFullName.IsEmpty())
    {
      if (auto pAttr = pRtti->GetAttributeByType<plCategoryAttribute>())
      {
        sFullName = pAttr->GetCategory();
      }
    }

    sFullName.AppendPath(plTranslate(sCleanName.GetData(tmp)));

    pSearchMenu->AddItem(sFullName, QVariant::fromValue((void*)pRtti));
  }

  pSearchMenu->Finalize(m_sContextMenuSearchText);

  menu.exec(screenPos);

  m_sContextMenuSearchText = pSearchMenu->GetSearchText();
}

plStatus plQtNodeScene::RemoveNode(plQtNode* pNode)
{
  PLASMA_SUCCEED_OR_RETURN(m_pManager->CanRemove(pNode->GetObject()));

  plRemoveNodeCommand cmd;
  cmd.m_Object = pNode->GetObject()->GetGuid();

  plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  return history->AddCommand(cmd);
}

void plQtNodeScene::RemoveSelectedNodesAction()
{
  plDeque<plQtNode*> selection;
  GetSelectedNodes(selection);

  if (selection.IsEmpty())
    return;

  plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Nodes");

  for (plQtNode* pNode : selection)
  {
    plStatus res = RemoveNode(pNode);

    if (res.m_Result.Failed())
    {
      history->CancelTransaction();

      plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to remove node");
      return;
    }
  }

  history->FinishTransaction();
}

void plQtNodeScene::ConnectPinsAction(const plPin& sourcePin, const plPin& targetPin)
{
  plDocumentNodeManager::CanConnectResult connect;
  plStatus res = m_pManager->CanConnect(m_pManager->GetConnectionType(), sourcePin, targetPin, connect);

  if (connect == plDocumentNodeManager::CanConnectResult::ConnectNever)
  {
    plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Failed to connect nodes.");
    return;
  }

  plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Connect Pins");

  // disconnect everything from the source pin
  if (connect == plDocumentNodeManager::CanConnectResult::Connect1to1 || connect == plDocumentNodeManager::CanConnectResult::Connect1toN)
  {
    const plArrayPtr<const plConnection* const> connections = m_pManager->GetConnections(sourcePin);
    for (const plConnection* pConnection : connections)
    {
      res = plNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // disconnect everything from the target pin
  if (connect == plDocumentNodeManager::CanConnectResult::Connect1to1 || connect == plDocumentNodeManager::CanConnectResult::ConnectNto1)
  {
    const plArrayPtr<const plConnection* const> connections = m_pManager->GetConnections(targetPin);
    for (const plConnection* pConnection : connections)
    {
      res = plNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetParent()->GetGuid());
      if (res.Failed())
      {
        history->CancelTransaction();
        return;
      }
    }
  }

  // connect the two pins
  {
    res = plNodeCommands::AddAndConnectCommand(history, m_pManager->GetConnectionType(), sourcePin, targetPin);
    if (res.Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void plQtNodeScene::DisconnectPinsAction(plQtConnection* pConnection)
{
  plStatus res = m_pManager->CanDisconnect(pConnection->GetConnection());
  if (res.m_Result.Succeeded())
  {
    plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
    history->StartTransaction("Disconnect Pins");

    res = plNodeCommands::DisconnectAndRemoveCommand(history, pConnection->GetConnection()->GetParent()->GetGuid());
    if (res.m_Result.Failed())
      history->CancelTransaction();
    else
      history->FinishTransaction();
  }

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Node disconnect failed.");
}

void plQtNodeScene::DisconnectPinsAction(plQtPin* pPin)
{
  plCommandHistory* history = m_pManager->GetDocument()->GetCommandHistory();
  history->StartTransaction("Disconnect Pins");

  plStatus res = plStatus(PLASMA_SUCCESS);
  for (plQtConnection* pConnection : pPin->GetConnections())
  {
    DisconnectPinsAction(pConnection);
  }

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();

  plQtUiServices::GetSingleton()->MessageBoxStatus(res, "Adding sub-element to the property failed.");
}

void plQtNodeScene::OnMenuItemTriggered(const QString& sName, const QVariant& variant)
{
  const plRTTI* pRtti = static_cast<const plRTTI*>(variant.value<void*>());

  CreateNodeObject(pRtti);
}

void plQtNodeScene::OnSelectionChanged()
{
  plCommandHistory* pHistory = m_pManager->GetDocument()->GetCommandHistory();
  if (pHistory->IsInUndoRedo() || pHistory->IsInTransaction())
    return;

  m_Selection.Clear();
  auto items = selectedItems();
  for (QGraphicsItem* pItem : items)
  {
    if (pItem->type() == plQtNodeScene::Node)
    {
      plQtNode* pNode = static_cast<plQtNode*>(pItem);
      m_Selection.PushBack(pNode->GetObject());
    }
    else if (pItem->type() == plQtNodeScene::Connection)
    {
      plQtConnection* pConnection = static_cast<plQtConnection*>(pItem);
      m_Selection.PushBack(pConnection->GetObject());
    }
  }

  if (!m_bIgnoreSelectionChange)
  {
    m_bIgnoreSelectionChange = true;
    m_pManager->GetDocument()->GetSelectionManager()->SetSelection(m_Selection);
    m_bIgnoreSelectionChange = false;
  }
}
