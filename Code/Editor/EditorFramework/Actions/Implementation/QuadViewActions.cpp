#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/QuadViewActions.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

plActionDescriptorHandle plQuadViewActions::s_hToggleViews;
plActionDescriptorHandle plQuadViewActions::s_hSpawnView;


void plQuadViewActions::RegisterActions()
{
  s_hToggleViews =
    PLASMA_REGISTER_ACTION_1("Scene.View.Toggle", plActionScope::Window, "Scene", "", plQuadViewAction, plQuadViewAction::ButtonType::ToggleViews);
  s_hSpawnView =
    PLASMA_REGISTER_ACTION_1("Scene.View.Span", plActionScope::Window, "Scene", "", plQuadViewAction, plQuadViewAction::ButtonType::SpawnView);
}

void plQuadViewActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hToggleViews);
  plActionManager::UnregisterAction(s_hSpawnView);
}

void plQuadViewActions::MapActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hToggleViews, szPath, 3.0f);
  // pMap->MapAction(s_hSpawnView, szPath, 4.0f);
}

////////////////////////////////////////////////////////////////////////
// plSceneViewAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plQuadViewAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plQuadViewAction::plQuadViewAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(context.m_pWindow);
  PLASMA_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'plQtGameObjectViewWidget'!");
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

plQuadViewAction::~plQuadViewAction() {}

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
