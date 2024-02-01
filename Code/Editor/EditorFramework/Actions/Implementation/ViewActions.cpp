#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Actions/ViewActions.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>

plActionDescriptorHandle plViewActions::s_hRenderMode;
plActionDescriptorHandle plViewActions::s_hPerspective;
plActionDescriptorHandle plViewActions::s_hActivateRemoteProcess;
plActionDescriptorHandle plViewActions::s_hLinkDeviceCamera;


void plViewActions::RegisterActions()
{
  s_hRenderMode = PL_REGISTER_DYNAMIC_MENU("View.RenderMode", plRenderModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hPerspective = PL_REGISTER_DYNAMIC_MENU("View.RenderPerspective", plPerspectiveAction, ":/EditorFramework/Icons/Perspective.svg");
  s_hActivateRemoteProcess = PL_REGISTER_ACTION_1(
    "View.ActivateRemoteProcess", plActionScope::Window, "View", "", plViewAction, plViewAction::ButtonType::ActivateRemoteProcess);
  s_hLinkDeviceCamera =
    PL_REGISTER_ACTION_1("View.LinkDeviceCamera", plActionScope::Window, "View", "", plViewAction, plViewAction::ButtonType::LinkDeviceCamera);
}

void plViewActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hRenderMode);
  plActionManager::UnregisterAction(s_hPerspective);
  plActionManager::UnregisterAction(s_hActivateRemoteProcess);
  plActionManager::UnregisterAction(s_hLinkDeviceCamera);
}

void plViewActions::MapToolbarActions(plStringView sMapping, plUInt32 uiFlags)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  if (uiFlags & Flags::PerspectiveMode)
    pMap->MapAction(s_hPerspective, "", 1.0f);

  if (uiFlags & Flags::RenderMode)
    pMap->MapAction(s_hRenderMode, "", 2.0f);

  if (uiFlags & Flags::ActivateRemoteProcess)
  {
    pMap->MapAction(s_hActivateRemoteProcess, "", 4.0f);
    pMap->MapAction(s_hLinkDeviceCamera, "", 5.0f);
  }
}

////////////////////////////////////////////////////////////////////////
// plRenderModeAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRenderModeAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plRenderModeAction::plRenderModeAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plEnumerationMenuAction(context, szName, szIconPath)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(context.m_pWindow);
  PL_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'plQtEngineViewWidget'!");
  InitEnumerationType(plGetStaticRTTI<plViewRenderMode>());
}

plInt64 plRenderModeAction::GetValue() const
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  return (plInt64)pView->m_pViewConfig->m_RenderMode;
}

void plRenderModeAction::Execute(const plVariant& value)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  pView->m_pViewConfig->m_RenderMode = (plViewRenderMode::Enum)value.ConvertTo<plInt64>();
  TriggerUpdate();
}

////////////////////////////////////////////////////////////////////////
// plPerspectiveAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plPerspectiveAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;

plPerspectiveAction::plPerspectiveAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plEnumerationMenuAction(context, szName, szIconPath)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(context.m_pWindow);
  PL_ASSERT_DEV(pView != nullptr, "context.m_pWindow must be derived from type 'plQtEngineViewWidget'!");
  InitEnumerationType(plGetStaticRTTI<plSceneViewPerspective>());
}

plInt64 plPerspectiveAction::GetValue() const
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  return (plInt64)pView->m_pViewConfig->m_Perspective;
}

void plPerspectiveAction::Execute(const plVariant& value)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);
  auto newValue = (plSceneViewPerspective::Enum)value.ConvertTo<plInt64>();

  if (pView->m_pViewConfig->m_Perspective != newValue)
  {
    pView->m_pViewConfig->m_Perspective = newValue;
    pView->m_pViewConfig->ApplyPerspectiveSetting();
    TriggerUpdate();
  }
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plViewAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;

plViewAction::plViewAction(const plActionContext& context, const char* szName, ButtonType button)
  : plButtonAction(context, szName, false, "")
{
  m_ButtonType = button;
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case plViewAction::ButtonType::ActivateRemoteProcess:
      SetIconPath(":/EditorFramework/Icons/SwitchToRemoteProcess.svg");
      break;
    case plViewAction::ButtonType::LinkDeviceCamera:
      SetIconPath(":/EditorFramework/Icons/LinkDeviceCamera.svg");
      SetCheckable(true);
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      break;
  }
}

plViewAction::~plViewAction() = default;

void plViewAction::Execute(const plVariant& value)
{
  plQtEngineViewWidget* pView = qobject_cast<plQtEngineViewWidget*>(m_Context.m_pWindow);

  switch (m_ButtonType)
  {
    case plViewAction::ButtonType::ActivateRemoteProcess:
    {
      plEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(plDynamicCast<plAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;

    case plViewAction::ButtonType::LinkDeviceCamera:
    {
      pView->m_pViewConfig->m_bUseCameraTransformOnDevice = !pView->m_pViewConfig->m_bUseCameraTransformOnDevice;
      SetChecked(pView->m_pViewConfig->m_bUseCameraTransformOnDevice);
      plEditorEngineProcessConnection::GetSingleton()->ActivateRemoteProcess(plDynamicCast<plAssetDocument*>(m_Context.m_pDocument), pView->GetViewID());
    }
    break;
  }
}
