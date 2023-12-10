#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAsset.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetScene.moc.h>
#include <EditorPluginAssets/AnimationGraphAsset/AnimationGraphAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/NodeEditor/NodeView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>



plQtAnimationGraphAssetDocumentWindow::plQtAnimationGraphAssetDocumentWindow(plDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "AnimationGraphAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "AnimationGraphAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationGraphAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  m_pScene = new plQtAnimationGraphAssetScene(this);
  m_pScene->InitScene(static_cast<const plDocumentNodeManager*>(pDocument->GetObjectManager()));

  m_pView = new plQtNodeView(this);
  m_pView->SetScene(m_pScene);
  setCentralWidget(m_pView);

  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AnimationGraphAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
  }

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtAnimationGraphAssetDocumentWindow::SelectionEventHandler, this));

  FinishWindowCreation();

  SelectionEventHandler(plSelectionManagerEvent());
}

plQtAnimationGraphAssetDocumentWindow::~plQtAnimationGraphAssetDocumentWindow()
{
  if (GetDocument() != nullptr)
  {
    GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAnimationGraphAssetDocumentWindow::SelectionEventHandler, this));
  }
}

void plQtAnimationGraphAssetDocumentWindow::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  {
    // delayed execution
    QTimer::singleShot(1, [this]() {
      // Check again if the selection is empty. This could have changed due to the delayed execution.
      if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
      {
        GetDocument()->GetSelectionManager()->SetSelection(((plAnimationGraphAssetDocument*)GetDocument())->GetPropertyObject());
      } });
  }
}
