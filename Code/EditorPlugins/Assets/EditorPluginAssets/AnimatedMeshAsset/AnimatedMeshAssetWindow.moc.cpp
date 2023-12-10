#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <SharedPluginAssets/Common/Messages.h>

plQtAnimatedMeshAssetDocumentWindow::plQtAnimatedMeshAssetDocumentWindow(plAnimatedMeshAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "AnimatedMeshAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "AnimatedMeshAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimatedMeshAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  plQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(plVec3(0, 0, 1), plVec3(10.0f), plVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "AnimatedMeshAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AnimatedMeshAssetDockWidget");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  FinishWindowCreation();

  UpdatePreview();

  m_pHighlightTimer = new QTimer();
  connect(m_pHighlightTimer, &QTimer::timeout, this, &plQtAnimatedMeshAssetDocumentWindow::HighlightTimer);
  m_pHighlightTimer->setInterval(500);
  m_pHighlightTimer->start();
}

plQtAnimatedMeshAssetDocumentWindow::~plQtAnimatedMeshAssetDocumentWindow()
{
  m_pHighlightTimer->stop();

  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler, this));
}

plAnimatedMeshAssetDocument* plQtAnimatedMeshAssetDocumentWindow::GetMeshDocument()
{
  return static_cast<plAnimatedMeshAssetDocument*>(GetDocument());
}

void plQtAnimatedMeshAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void plQtAnimatedMeshAssetDocumentWindow::QueryObjectBBox(plInt32 iPurpose /* = 0*/)
{
  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtAnimatedMeshAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  // if (e.m_sProperty == "Resource") // any material change
  {
    UpdatePreview();
  }
}

bool plQtAnimatedMeshAssetDocumentWindow::UpdatePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return false;

  if (GetMeshDocument()->GetProperties() == nullptr)
    return false;

  const auto& materials = GetMeshDocument()->GetProperties()->m_Slots;

  plEditorEngineSetMaterialsMsg msg;
  msg.m_Materials.SetCount(materials.GetCount());

  plUInt32 uiSlot = 0;
  bool bHighlighted = false;

  for (plUInt32 i = 0; i < materials.GetCount(); ++i)
  {
    msg.m_Materials[i] = materials[i].m_sResource;

    if (materials[i].m_bHighlight)
    {
      if (uiSlot == m_uiHighlightSlots)
      {
        bHighlighted = true;
        msg.m_Materials[i] = "Editor/Materials/HighlightMesh.plMaterial";
      }

      ++uiSlot;
    }
  }

  GetEditorEngineConnection()->SendMessage(&msg);

  return bHighlighted;
}

void plQtAnimatedMeshAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtAnimatedMeshAssetDocumentWindow::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plQuerySelectionBBoxResultMsgToEditor>())
  {
    const plQuerySelectionBBoxResultMsgToEditor* pMessage = static_cast<const plQuerySelectionBBoxResultMsgToEditor*>(pMsg);

    if (pMessage->m_vCenter.IsValid() && pMessage->m_vHalfExtents.IsValid())
    {
      m_pViewWidget->SetOrbitVolume(pMessage->m_vCenter, pMessage->m_vHalfExtents.CompMax(plVec3(0.1f)));
    }
    else
    {
      // try again
      QueryObjectBBox(pMessage->m_iPurpose);
    }

    return;
  }

  plQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg);
}

void plQtAnimatedMeshAssetDocumentWindow::HighlightTimer()
{
  if (m_uiHighlightSlots & PLASMA_BIT(31))
    m_uiHighlightSlots &= ~PLASMA_BIT(31);
  else
    m_uiHighlightSlots |= PLASMA_BIT(31);

  if (m_uiHighlightSlots & PLASMA_BIT(31))
  {
    UpdatePreview();
  }
  else
  {
    if (UpdatePreview())
    {
      ++m_uiHighlightSlots;
    }
    else
    {
      m_uiHighlightSlots = PLASMA_BIT(31);
    }
  }
}
