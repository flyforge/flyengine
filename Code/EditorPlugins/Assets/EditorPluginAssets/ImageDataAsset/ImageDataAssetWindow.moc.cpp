#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorPluginAssets/ImageDataAsset/ImageDataAsset.h>
#include <EditorPluginAssets/ImageDataAsset/ImageDataAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// plQtImageDataAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

plQtImageDataAssetDocumentWindow::plQtImageDataAssetDocumentWindow(plImageDataAssetDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "ImageDataAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "ImageDataAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("ImageDataAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("ImageDataAssetDockWidget");
    pPropertyPanel->setWindowTitle("PROPERTIES");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  m_pImageWidget = new plQtImageWidget(this);

  setCentralWidget(m_pImageWidget);

  FinishWindowCreation();

  UpdatePreview();

  pDocument->Events().AddEventHandler(plMakeDelegate(&plQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler, this), m_EventUnsubscriper);
}

void plQtImageDataAssetDocumentWindow::ImageDataAssetEventHandler(const plImageDataAssetEvent& e)
{
  if (e.m_Type != plImageDataAssetEvent::Type::Transformed)
    return;

  UpdatePreview();
}

void plQtImageDataAssetDocumentWindow::UpdatePreview()
{
  auto pImageDoc = static_cast<plImageDataAssetDocument*>(GetDocument());

  plStringBuilder path = pImageDoc->GetProperties()->m_sInputFile;
  if (!plQtEditorApp::GetSingleton()->MakeDataDirectoryRelativePathAbsolute(path))
    return;

  QPixmap pixmap;
  if (!pixmap.load(path.GetData(), nullptr, Qt::AutoColor))
    return;

  m_pImageWidget->SetImage(pixmap);
}
