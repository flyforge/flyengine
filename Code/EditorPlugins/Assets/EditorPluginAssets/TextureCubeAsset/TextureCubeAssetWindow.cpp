#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAsset.h>
#include <EditorPluginAssets/TextureCubeAsset/TextureCubeAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// plTextureCubeChannelModeAction
////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeChannelModeAction, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plTextureCubeChannelModeAction::plTextureCubeChannelModeAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(plGetStaticRTTI<plTextureCubeChannelMode>());
}

plInt64 plTextureCubeChannelModeAction::GetValue() const
{
  return static_cast<const plTextureCubeAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void plTextureCubeChannelModeAction::Execute(const plVariant& value)
{
  ((plTextureCubeAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<plInt32>());
}

//////////////////////////////////////////////////////////////////////////
// plTextureCubeLodSliderAction
//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureCubeLodSliderAction, 1, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;


plTextureCubeLodSliderAction::plTextureCubeLodSliderAction(const plActionContext& context, const char* szName)
  : plSliderAction(context, szName)
{
  m_pDocument = const_cast<plTextureCubeAssetDocument*>(static_cast<const plTextureCubeAssetDocument*>(context.m_pDocument));

  SetRange(-1, 13);
  SetValue(m_pDocument->m_iTextureLod);
}

void plTextureCubeLodSliderAction::Execute(const plVariant& value)
{
  const plInt32 iValue = value.Get<plInt32>();

  m_pDocument->m_iTextureLod = value.Get<plInt32>();
}


//////////////////////////////////////////////////////////////////////////
// plTextureCubeAssetActions
//////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plTextureCubeAssetActions::s_hTextureChannelMode;
plActionDescriptorHandle plTextureCubeAssetActions::s_hLodSlider;

void plTextureCubeAssetActions::RegisterActions()
{
  s_hTextureChannelMode =
    PLASMA_REGISTER_DYNAMIC_MENU("TextureCubeAsset.ChannelMode", plTextureCubeChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = PLASMA_REGISTER_ACTION_0("TextureCubeAsset.LodSlider", plActionScope::Document, "CompatibleAsset_Texture_Cube", "", plTextureCubeLodSliderAction);
}

void plTextureCubeAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hTextureChannelMode);
  plActionManager::UnregisterAction(s_hLodSlider);
}

void plTextureCubeAssetActions::MapActions(const char* szMapping, const char* szPath)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(szMapping);
  PLASMA_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", szMapping);

  pMap->MapAction(s_hLodSlider, szPath, 14.0f);
  pMap->MapAction(s_hTextureChannelMode, szPath, 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// plQtTextureCubeAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

plQtTextureCubeAssetDocumentWindow::plQtTextureCubeAssetDocumentWindow(plTextureCubeAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "TextureCubeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "TextureCubeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureCubeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-2, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureFixed(plVec3(0), plVec3(0.0f), plVec3(-1, 0, 0));
    AddViewWidget(m_pViewWidget);
    plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, nullptr);

    setCentralWidget(pContainer);
  }

  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("TextureCubeAssetDockWidget");
    pPropertyPanel->setWindowTitle("TEXTURE PROPERTIES");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void plQtTextureCubeAssetDocumentWindow::InternalRedraw()
{
  PlasmaEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtTextureCubeAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const plTextureCubeAssetDocument* pDoc = static_cast<const plTextureCubeAssetDocument*>(GetDocument());

    plDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChannelMode";
    msg.m_iValue = pDoc->m_ChannelMode.GetValue();
    msg.m_fValue = pDoc->m_iTextureLod;

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}
