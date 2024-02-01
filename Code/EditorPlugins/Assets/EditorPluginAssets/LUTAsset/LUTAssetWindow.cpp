#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorPluginAssets/LUTAsset/LUTAsset.h>
#include <EditorPluginAssets/LUTAsset/LUTAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

//////////////////////////////////////////////////////////////////////////
// plLUTAssetActions
//////////////////////////////////////////////////////////////////////////


void plLUTAssetActions::RegisterActions() {}

void plLUTAssetActions::UnregisterActions() {}

void plLUTAssetActions::MapActions(plStringView sMapping) {}


//////////////////////////////////////////////////////////////////////////
// plQtTextureAssetDocumentWindow
//////////////////////////////////////////////////////////////////////////

plQtLUTAssetDocumentWindow::plQtLUTAssetDocumentWindow(plLUTAssetDocument* pDocument)
  : plQtDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "LUTAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "LUTAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("LUTAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  {
    /*
        TODO: Add live 3D preview of the LUT with a slider for the strength etc.

        SetTargetFramerate(10);

        m_ViewConfig.m_Camera.LookAt(plVec3(-2, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
        m_ViewConfig.ApplyPerspectiveSetting(90);

        m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
        m_pViewWidget->ConfigureOrbitCameraVolume(plVec3(0), plVec3(1.0f), plVec3(-1, 0, 0));
        AddViewWidget(m_pViewWidget);
        plQtViewWidgetContainer* pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, nullptr);

        setCentralWidget(pContainer);*/
  }

  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("LUTAssetDockWidget");
    pPropertyPanel->setWindowTitle("LUT Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();
}
