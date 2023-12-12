#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/StateMachineAsset/StateMachineAsset.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraph.h>
#include <EditorPluginAssets/StateMachineAsset/StateMachineGraphQt.moc.h>
#include <Foundation/Math/ColorScheme.h>

plQtStateMachinePin::plQtStateMachinePin() = default;

void plQtStateMachinePin::SetPin(const plPin& pin)
{
  plQtPin::SetPin(pin);

  constexpr int padding = 3;

  m_pLabel->setPlainText("     +     ");
  m_pLabel->setPos(0, 0);

  auto bounds = m_pLabel->boundingRect();
  m_PinCenter = bounds.center();

  QPainterPath p;
  p.addRect(bounds);
  setPath(p);

  if (pin.GetType() == plPin::Type::Input)
  {
    m_pLabel->setPlainText("");
  }
  else
  {
    m_pLabel->setToolTip("Add Transition");
  }
}

QRectF plQtStateMachinePin::GetPinRect() const
{
  auto rect = path().boundingRect();
  rect.translate(pos());
  return rect;
}

//////////////////////////////////////////////////////////////////////////

plQtStateMachineConnection::plQtStateMachineConnection()
{
  setFlag(QGraphicsItem::ItemIsSelectable);
}

//////////////////////////////////////////////////////////////////////////

plQtStateMachineNode::plQtStateMachineNode() = default;

void plQtStateMachineNode::InitNode(const plDocumentNodeManager* pManager, const plDocumentObject* pObject)
{
  plQtNode::InitNode(pManager, pObject);

  UpdateHeaderColor();
}

void plQtStateMachineNode::UpdateGeometry()
{
  prepareGeometryChange();

  auto labelRect = m_pTitleLabel->boundingRect();

  constexpr int padding = 5;
  const int headerWidth = labelRect.width();
  const int headerHeight = labelRect.height() + padding * 2;

  int h = headerHeight;
  int w = headerWidth;

  for (plQtPin* pQtPin : GetInputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = plMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
  }

  for (plQtPin* pQtPin : GetOutputPins())
  {
    auto rectPin = pQtPin->GetPinRect();
    w = plMath::Max(w, (int)rectPin.width());

    pQtPin->setPos((w - rectPin.width()) / 2.0, h);
    h += rectPin.height();
  }

  w += padding * 2;
  h += padding * 2;

  m_HeaderRect = QRectF(-padding, -padding, w, headerHeight);

  {
    QPainterPath p;
    p.addRoundedRect(-padding, -padding, w, h, padding, padding);
    setPath(p);
  }
}

void plQtStateMachineNode::UpdateState()
{
  UpdateHeaderColor();

  if (IsAnyState())
  {
    m_pTitleLabel->setPlainText("Any State");
  }
  else
  {
    plStringBuilder sName;

    auto& typeAccessor = GetObject()->GetTypeAccessor();

    plVariant name = typeAccessor.GetValue("Name");
    if (name.IsA<plString>() && name.Get<plString>().IsEmpty() == false)
    {
      sName = name.Get<plString>();
    }
    else
    {
      sName = typeAccessor.GetType()->GetTypeName();
    }

    if (IsInitialState())
    {
      sName.Append(" [Initial State]");
    }

    m_pTitleLabel->setPlainText(sName.GetData());
  }
}

void plQtStateMachineNode::ExtendContextMenu(QMenu& menu)
{
  if (IsAnyState())
    return;

  QAction* pAction = new QAction("Set as Initial State", &menu);
  pAction->setEnabled(IsInitialState() == false);
  pAction->connect(pAction, &QAction::triggered,
    [this]() {
      auto pScene = static_cast<plQtStateMachineAssetScene*>(scene());
      pScene->SetInitialState(this);
    });

  menu.addAction(pAction);
}

bool plQtStateMachineNode::IsInitialState() const
{
  auto pManager = static_cast<const plStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsInitialState(GetObject());
}

bool plQtStateMachineNode::IsAnyState() const
{
  auto pManager = static_cast<const plStateMachineNodeManager*>(GetObject()->GetDocumentObjectManager());
  return pManager->IsAnyState(GetObject());
}

void plQtStateMachineNode::UpdateHeaderColor()
{
  plColorScheme::Enum schemeColor = plColorScheme::Gray;

  if (IsAnyState())
  {
    schemeColor = plColorScheme::Violet;
  }
  else if (IsInitialState())
  {
    schemeColor = plColorScheme::Teal;
  }

  m_HeaderColor = plToQtColor(plColorScheme::DarkUI(schemeColor));

  update();
}

//////////////////////////////////////////////////////////////////////////

plQtStateMachineAssetScene::plQtStateMachineAssetScene(QObject* parent /*= nullptr*/)
  : plQtNodeScene(parent)
{
  SetConnectionStyle(plQtNodeScene::ConnectionStyle::StraightLine);
  SetConnectionDecorationFlags(plQtNodeScene::ConnectionDecorationFlags::DirectionArrows);
}

plQtStateMachineAssetScene::~plQtStateMachineAssetScene() = default;

void plQtStateMachineAssetScene::SetInitialState(plQtStateMachineNode* pNode)
{
  plCommandHistory* history = GetDocumentNodeManager()->GetDocument()->GetCommandHistory();
  history->StartTransaction("Set Initial State");

  plStateMachine_SetInitialStateCommand cmd;
  cmd.m_NewInitialStateObject = pNode->GetObject()->GetGuid();

  plStatus res = history->AddCommand(cmd);

  if (res.m_Result.Failed())
    history->CancelTransaction();
  else
    history->FinishTransaction();
}

plStatus plQtStateMachineAssetScene::RemoveNode(plQtNode* pNode)
{
  auto pManager = static_cast<const plStateMachineNodeManager*>(GetDocumentNodeManager());
  const bool bWasInitialState = pManager->IsInitialState(pNode->GetObject());

  auto res = plQtNodeScene::RemoveNode(pNode);
  if (res.m_Result.Succeeded() && bWasInitialState)
  {
    // Find another node
    plUuid newInitialStateObject;
    for (auto it : m_Nodes)
    {
      if (it.Value() != pNode && pManager->IsAnyState(it.Key()) == false)
      {
        newInitialStateObject = it.Key()->GetGuid();
      }
    }

    plCommandHistory* history = pManager->GetDocument()->GetCommandHistory();

    plStateMachine_SetInitialStateCommand cmd;
    cmd.m_NewInitialStateObject = newInitialStateObject;

    plStatus res = history->AddCommand(cmd);
  }

  return res;
}
