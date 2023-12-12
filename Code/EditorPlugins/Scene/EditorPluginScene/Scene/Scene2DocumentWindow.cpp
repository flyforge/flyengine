#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorPluginScene/Panels/LayerPanel/LayerPanel.moc.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/Scene2DocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtScene2DocumentWindow::plQtScene2DocumentWindow(plScene2Document* pDocument)
  : plQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](plQtEngineDocumentWindow* pWindow, PlasmaEngineViewConfig* pConfig) -> plQtEngineViewWidget* {
    plQtSceneViewWidget* pWidget = new plQtSceneViewWidget(nullptr, static_cast<plQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new plQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](plGameObjectEditTool* pTool) { pTool->ConfigureTool(static_cast<plGameObjectDocument*>(GetDocument()), this, this); });

  setCentralWidget(m_pQuadViewWidget);

  SetTargetFramerate(60);

  {
    // Menu Bar
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2MenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "EditorPluginScene_Scene2ToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  const plSceneDocument* pSceneDoc = static_cast<const plSceneDocument*>(GetDocument());

  {
    // Panels
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("PROPERTIES");
    pPropertyPanel->show();

    plQtDocumentPanel* pPanelTree = new plQtScenegraphPanel(this, pDocument);
    pPanelTree->show();

    plQtLayerPanel* pLayers = new plQtLayerPanel(this, pDocument);
    pLayers->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    PLASMA_VERIFY(connect(pPropertyGrid, &plQtPropertyGridWidget::ExtendContextMenu, this, &plQtScene2DocumentWindow::ExtendPropertyGridContextMenu), "");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pLayers);
  }
  FinishWindowCreation();
}

plQtScene2DocumentWindow::~plQtScene2DocumentWindow()
{
}

bool plQtScene2DocumentWindow::InternalCanCloseWindow()
{
  // I guess this is to remove the focus from other widgets like input boxes, such that they may modify the document.
  setFocus();
  clearFocus();

  plScene2Document* pDoc = static_cast<plScene2Document*>(GetDocument());
  if (pDoc && pDoc->IsAnyLayerModified())
  {
    QMessageBox::StandardButton res = plQtUiServices::MessageBoxQuestion("Save scene and all layers before closing?", QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Cancel, QMessageBox::StandardButton::Cancel);

    if (res == QMessageBox::StandardButton::Cancel)
      return false;

    if (res == QMessageBox::StandardButton::Yes)
    {
      plStatus err = SaveAllLayers();

      if (err.Failed())
      {
        plQtUiServices::GetSingleton()->MessageBoxStatus(err, "Saving the scene failed.");
        return false;
      }
    }
  }

  return true;
}

plStatus plQtScene2DocumentWindow::SaveAllLayers()
{
  plScene2Document* pDoc = static_cast<plScene2Document*>(GetDocument());

  plHybridArray<plSceneDocument*, 16> layers;
  pDoc->GetLoadedLayers(layers);

  for (auto pLayer : layers)
  {
    plStatus res = pLayer->SaveDocument();

    if (res.Failed())
    {
      return res;
    }
  }

  return plStatus(PLASMA_SUCCESS);
}
