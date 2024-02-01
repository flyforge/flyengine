#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/TextureAsset/TextureAsset.h>
#include <EditorPluginAssets/TextureAsset/TextureAssetWindow.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

////////////////////////////////////////////////////////////////////////
// plTextureChannelModeAction
////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureChannelModeAction, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plTextureChannelModeAction::plTextureChannelModeAction(const plActionContext& context, const char* szName, const char* szIconPath)
  : plEnumerationMenuAction(context, szName, szIconPath)
{
  InitEnumerationType(plGetStaticRTTI<plTextureChannelMode>());
}

plInt64 plTextureChannelModeAction::GetValue() const
{
  return static_cast<const plTextureAssetDocument*>(m_Context.m_pDocument)->m_ChannelMode.GetValue();
}

void plTextureChannelModeAction::Execute(const plVariant& value)
{
  ((plTextureAssetDocument*)m_Context.m_pDocument)->m_ChannelMode.SetValue(value.ConvertTo<plInt32>());
}

//////////////////////////////////////////////////////////////////////////
// plTextureLodSliderAction
//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plTextureLodSliderAction, 1, plRTTINoAllocator)
  ;
PL_END_DYNAMIC_REFLECTED_TYPE;


plTextureLodSliderAction::plTextureLodSliderAction(const plActionContext& context, const char* szName)
  : plSliderAction(context, szName)
{
  m_pDocument = const_cast<plTextureAssetDocument*>(static_cast<const plTextureAssetDocument*>(context.m_pDocument));

  SetRange(-1, 13);
  SetValue(m_pDocument->m_iTextureLod);
}

void plTextureLodSliderAction::Execute(const plVariant& value)
{
  m_pDocument->m_iTextureLod = value.Get<plInt32>();
}


//////////////////////////////////////////////////////////////////////////
// plTextureAssetActions
//////////////////////////////////////////////////////////////////////////

plActionDescriptorHandle plTextureAssetActions::s_hTextureChannelMode;
plActionDescriptorHandle plTextureAssetActions::s_hLodSlider;

void plTextureAssetActions::RegisterActions()
{
  s_hTextureChannelMode = PL_REGISTER_DYNAMIC_MENU("TextureAsset.ChannelMode", plTextureChannelModeAction, ":/EditorFramework/Icons/RenderMode.svg");
  s_hLodSlider = PL_REGISTER_ACTION_0("TextureAsset.LodSlider", plActionScope::Document, "Texture 2D", "", plTextureLodSliderAction);
}

void plTextureAssetActions::UnregisterActions()
{
  plActionManager::UnregisterAction(s_hTextureChannelMode);
  plActionManager::UnregisterAction(s_hLodSlider);
}

void plTextureAssetActions::MapToolbarActions(plStringView sMapping)
{
  plActionMap* pMap = plActionMapManager::GetActionMap(sMapping);
  PL_ASSERT_DEV(pMap != nullptr, "The given mapping ('{0}') does not exist, mapping the actions failed!", sMapping);

  pMap->MapAction(s_hLodSlider, "", 14.0f);
  pMap->MapAction(s_hTextureChannelMode, "", 15.0f);
}


//////////////////////////////////////////////////////////////////////////
// plQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

plQtTextureAssetDocumentWindow::plQtTextureAssetDocumentWindow(plTextureAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "TextureAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "TextureAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("TextureAssetWindowToolBar");
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
    pPropertyPanel->setObjectName("TextureAssetDockWidget");
    pPropertyPanel->setWindowTitle("Texture Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}

void plQtTextureAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtTextureAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    const plTextureAssetDocument* pDoc = static_cast<const plTextureAssetDocument*>(GetDocument());

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
