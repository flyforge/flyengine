#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

#include <EditorFramework/Assets/AssetDocument.h>

plQtEngineDocumentWindow::plQtEngineDocumentWindow(plAssetDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{
  pDocument->m_ProcessMessageEvent.AddEventHandler(plMakeDelegate(&plQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  pDocument->m_CommonAssetUiChangeEvent.AddEventHandler(plMakeDelegate(&plQtEngineDocumentWindow::CommonAssetUiEventHandler, this));
}

plQtEngineDocumentWindow::~plQtEngineDocumentWindow()
{
  // make sure the selection gets cleared before the views are destroyed, so that dependent code can clean up first
  GetDocument()->GetSelectionManager()->Clear();

  GetDocument()->m_ProcessMessageEvent.RemoveEventHandler(plMakeDelegate(&plQtEngineDocumentWindow::ProcessMessageEventHandler, this));
  GetDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(plMakeDelegate(&plQtEngineDocumentWindow::CommonAssetUiEventHandler, this));

  // delete all view widgets, so that they can send their messages before we clean up the engine connection
  DestroyAllViews();
}


plEditorEngineConnection* plQtEngineDocumentWindow::GetEditorEngineConnection() const
{
  return GetDocument()->GetEditorEngineConnection();
}

static plObjectPickingResult s_DummyResult;

const plObjectPickingResult& plQtEngineDocumentWindow::PickObject(plUInt16 uiScreenPosX, plUInt16 uiScreenPosY, plQtEngineViewWidget* pView) const
{
  if (pView == nullptr)
    pView = GetHoveredViewWidget();

  if (pView != nullptr)
    return pView->PickObject(uiScreenPosX, uiScreenPosY);

  return s_DummyResult;
}


plAssetDocument* plQtEngineDocumentWindow::GetDocument() const
{
  return static_cast<plAssetDocument*>(plQtDocumentWindow::GetDocument());
}

void plQtEngineDocumentWindow::InternalRedraw()
{
  // TODO: Move this to a better place (some kind of regular update function, not redraw)
  GetDocument()->SyncObjectsToEngine();
}

plQtEngineViewWidget* plQtEngineDocumentWindow::GetHoveredViewWidget() const
{
  QWidget* pWidget = QApplication::widgetAt(QCursor::pos());

  while (pWidget != nullptr)
  {
    plQtEngineViewWidget* pCandidate = qobject_cast<plQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

plQtEngineViewWidget* plQtEngineDocumentWindow::GetFocusedViewWidget() const
{
  QWidget* pWidget = QApplication::focusWidget();

  while (pWidget != nullptr)
  {
    plQtEngineViewWidget* pCandidate = qobject_cast<plQtEngineViewWidget*>(pWidget);
    if (pCandidate != nullptr)
    {
      if (m_ViewWidgets.Contains(pCandidate))
        return pCandidate;

      return nullptr;
    }

    pWidget = pWidget->parentWidget();
  }

  return nullptr;
}

plQtEngineViewWidget* plQtEngineDocumentWindow::GetViewWidgetByID(plUInt32 uiViewID) const
{
  for (auto pView : m_ViewWidgets)
  {
    if (pView && pView->GetViewID() == uiViewID)
      return pView;
  }

  return nullptr;
}

plArrayPtr<plQtEngineViewWidget* const> plQtEngineDocumentWindow::GetViewWidgets() const
{
  return m_ViewWidgets;
}

void plQtEngineDocumentWindow::AddViewWidget(plQtEngineViewWidget* pView)
{
  m_ViewWidgets.PushBack(pView);
  plEngineWindowEvent e;
  e.m_Type = plEngineWindowEvent::Type::ViewCreated;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void plQtEngineDocumentWindow::RemoveViewWidget(plQtEngineViewWidget* pView)
{
  m_ViewWidgets.RemoveAndSwap(pView);
  plEngineWindowEvent e;
  e.m_Type = plEngineWindowEvent::Type::ViewDestroyed;
  e.m_pView = pView;
  m_EngineWindowEvent.Broadcast(e);
}

void plQtEngineDocumentWindow::CommonAssetUiEventHandler(const plCommonAssetUiState& e)
{
  plSimpleDocumentConfigMsgToEngine msg;
  msg.m_sWhatToDo = "CommonAssetUiState";
  msg.m_fPayload = e.m_fValue;

  switch (e.m_State)
  {
    case plCommonAssetUiState::Restart:
      msg.m_sPayload = "Restart";
      break;

    case plCommonAssetUiState::Loop:
      msg.m_sPayload = "Loop";
      break;

    case plCommonAssetUiState::Pause:
      msg.m_sPayload = "Pause";
      break;

    case plCommonAssetUiState::Grid:
      msg.m_sPayload = "Grid";
      break;

    case plCommonAssetUiState::SimulationSpeed:
      msg.m_sPayload = "SimulationSpeed";
      break;

    case plCommonAssetUiState::Visualizers:
      msg.m_sPayload = "Visualizers";
      break;

      PLASMA_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  if (!msg.m_sPayload.IsEmpty())
  {
    GetEditorEngineConnection()->SendMessage(&msg);
  }
}

void plQtEngineDocumentWindow::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plEditorEngineViewMsg>())
  {
    const plEditorEngineViewMsg* pViewMsg = static_cast<const plEditorEngineViewMsg*>(pMsg);

    plQtEngineViewWidget* pView = GetViewWidgetByID(pViewMsg->m_uiViewID);

    if (pView != nullptr)
      pView->HandleViewMessage(pViewMsg);
  }
}

void plQtEngineDocumentWindow::DestroyAllViews()
{
  while (!m_ViewWidgets.IsEmpty())
  {
    delete m_ViewWidgets[0];
  }
}
