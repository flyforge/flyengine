#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

plActionDescriptorHandle plQuadViewActions::s_hToggleViews;
plActionDescriptorHandle plQuadViewActions::s_hSpawnView;

void plQuadViewActions::RegisterActions()
{
  s_hToggleViews = PL_REGISTER_ACTION_1("Scene.View.Toggle", plActionScope::Window, "Scene", "", plQuadViewAction, plQuadViewAction::ButtonType::ToggleViews);
  s_hSpawnView = PL_REGISTER_ACTION_1("Scene.View.Span", plActionScope::Window, "Scene", "", plQuadViewAction, plQuadViewAction::ButtonType::SpawnView);
}

void plQuadViewActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hToggleViews);
  plActionManager::UnregisterAction(s_hSpawnView);
}

void plQuadViewActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hToggleViews, "", 3.0f);
}

////////////////////////////////////////////////////////////////////////
// plSceneViewAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuadViewAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plQuadViewAction::plQuadViewAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(context.m_pWindow);
  PL_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'plQtGameObjectViewWidget'!");
  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      SetIconPath(":/EditorFramework/Icons/ToggleViews.svg");
      break;
    case ButtonType::SpawnView:
      SetIconPath(":/EditorFramework/Icons/SpawnView.svg");
      break;
  }
}

plQuadViewAction::~plQuadViewAction() = default;

void plQuadViewAction::Execute(const plVariant& value)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  plQtEngineDocumentWindow* pWindow = static_cast<plQtEngineDocumentWindow*>(pView->GetDocumentWindow());

  switch (m_ButtonType)
  {
    case ButtonType::ToggleViews:
      // Duck-typing to the rescue!
      QMetaObject::invokeMethod(pWindow, "ToggleViews", Qt::ConnectionType::QueuedConnection, Q_ARG(QWidget*, pView));
      break;
    case ButtonType::SpawnView:
      break;
  }
}
