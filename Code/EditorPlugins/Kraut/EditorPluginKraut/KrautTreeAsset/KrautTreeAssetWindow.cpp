#include <EditorPluginKraut/EditorPluginKrautPCH.h>

#include <Foundation/Utilities/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QBoxLayout>
#include <QComboBox>

plQtKrautTreeAssetDocumentWindow::plQtKrautTreeAssetDocumentWindow(plAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "KrautTreeAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "KrautTreeAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("KrautTreeAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  plQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(plVec3(0, 0, 2), plVec3(10.0f), plVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "MeshAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    plDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0];

    plDeque<const plDocumentObject*> sel;
    sel.PushBack(pRootObject);

    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("KrautTreeAssetDockWidget");
    pPropertyPanel->setWindowTitle("Kraut Tree Properties");
    pPropertyPanel->show();

    QComboBox* pBranchTypeCombo = new QComboBox(pPropertyPanel);

    QTabWidget* pTabWidget = new QTabWidget(pPropertyPanel);

    QWidget* pGroup = new QWidget(pPropertyPanel);
    pGroup->setLayout(new QVBoxLayout);
    pGroup->layout()->addWidget(pBranchTypeCombo);
    pGroup->layout()->addWidget(pTabWidget);

    pPropertyPanel->setWidget(pGroup);

    plQtPropertyGridWidget* pAssetProps = new plQtPropertyGridWidget(pTabWidget, pDocument, false);
    pAssetProps->SetSelectionIncludeExcludeProperties(nullptr, "Materials;BT_Trunk1;BT_Trunk2;BT_Trunk3;BT_MainBranch1;BT_MainBranch2;BT_MainBranch3;BT_SubBranch1;BT_SubBranch2;BT_SubBranch3;BT_Twig1;BT_Twig2;BT_Twig3");
    pAssetProps->SetSelection(sel);
    pTabWidget->addTab(pAssetProps, "Asset");

    //plQtPropertyGridWidget* pMaterialProps = new plQtPropertyGridWidget(pTabWidget, pDocument, false);
    //pMaterialProps->SetSelectionIncludeExcludeProperties("Materials");
    //pMaterialProps->SetSelection(sel);
    //pTabWidget->addTab(pMaterialProps, "Materials");

    m_pBranchProps = new plQtPropertyGridWidget(pTabWidget, pDocument, false);
    //m_pBranchProps->SetSelectionIncludeExcludeProperties("BT_Trunk1;BT_MainBranch1;BT_MainBranch2");
    //m_pBranchProps->SetSelection(sel);
    pTabWidget->addTab(m_pBranchProps, "Branch Type");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

    {
      connect(pBranchTypeCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(onBranchTypeSelected(int)));
      pBranchTypeCombo->addItem("Trunk");
      //pBranchTypeCombo->addItem("Trunk 2");
      //pBranchTypeCombo->addItem("Trunk 3");
      pBranchTypeCombo->addItem("Main Branch 1");
      pBranchTypeCombo->addItem("Main Branch 2");
      pBranchTypeCombo->addItem("Main Branch 3");
      pBranchTypeCombo->addItem("Sub Branch 1");
      pBranchTypeCombo->addItem("Sub Branch 2");
      pBranchTypeCombo->addItem("Sub Branch 3");
      pBranchTypeCombo->addItem("Twig 1");
      pBranchTypeCombo->addItem("Twig 2");
      pBranchTypeCombo->addItem("Twig 3");

      pBranchTypeCombo->setCurrentIndex(0);
    }
  }

  m_pAssetDoc = static_cast<plKrautTreeAssetDocument*>(pDocument);

  FinishWindowCreation();

  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));
}

plQtKrautTreeAssetDocumentWindow::~plQtKrautTreeAssetDocumentWindow()
{
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtKrautTreeAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

void plQtKrautTreeAssetDocumentWindow::SendRedrawMsg()
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

void plQtKrautTreeAssetDocumentWindow::QueryObjectBBox(plInt32 iPurpose /*= 0*/)
{
  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtKrautTreeAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtKrautTreeAssetDocumentWindow::UpdatePreview()
{
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Kraut Tree";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);

  // Write Path
  plStringBuilder sAbsFilePath = GetDocument()->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plKrautTree");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(GetDocument()->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, GetDocument()->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).AssertSuccess();

  // Write Asset Data
  GetKrautDocument()->WriteKrautAsset(memoryWriter).IgnoreResult();
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  plEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtKrautTreeAssetDocumentWindow::InternalRedraw()
{
  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtKrautTreeAssetDocumentWindow::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg)
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

void plQtKrautTreeAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  if (e.m_sProperty == "DisplayRandomSeed")
  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "UpdateTree";
    msg.m_sPayload = "DisplayRandomSeed";
    msg.m_fPayload = static_cast<plKrautTreeAssetDocument*>(GetDocument())->GetProperties()->m_uiRandomSeedForDisplay;

    GetDocument()->SendMessageToEngine(&msg);
  }
  else
  {
    UpdatePreview();
  }
}

void plQtKrautTreeAssetDocumentWindow::onBranchTypeSelected(int index)
{
  if (m_pBranchProps == nullptr)
    return;

  plDocumentObject* pRootObject = GetDocument()->GetObjectManager()->GetRootObject()->GetChildren()[0];

  plDocumentObject* pSelected = nullptr;

  for (plDocumentObject* pChild : pRootObject->GetChildren())
  {
    if (index == 0 && pChild->GetParentProperty().IsEqual("BT_Trunk1"))
    {
      pSelected = pChild;
      break;
    }
    //if (index == 1 && pChild->GetParentProperty().IsEqual("BT_Trunk2"))
    //{
    //  pSelected = pChild;
    //  break;
    //}
    //if (index == 2 && pChild->GetParentProperty().IsEqual("BT_Trunk3"))
    //{
    //  pSelected = pChild;
    //  break;
    //}
    if (index == 1 && pChild->GetParentProperty().IsEqual("BT_MainBranch1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 2 && pChild->GetParentProperty().IsEqual("BT_MainBranch2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 3 && pChild->GetParentProperty().IsEqual("BT_MainBranch3"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 4 && pChild->GetParentProperty().IsEqual("BT_SubBranch1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 5 && pChild->GetParentProperty().IsEqual("BT_SubBranch2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 6 && pChild->GetParentProperty().IsEqual("BT_SubBranch3"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 7 && pChild->GetParentProperty().IsEqual("BT_Twig1"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 8 && pChild->GetParentProperty().IsEqual("BT_Twig2"))
    {
      pSelected = pChild;
      break;
    }
    if (index == 9 && pChild->GetParentProperty().IsEqual("BT_Twig3"))
    {
      pSelected = pChild;
      break;
    }
  }

  plDeque<const plDocumentObject*> sel;
  sel.PushBack(pSelected);
  GetDocument()->GetSelectionManager()->SetSelection(pSelected);
  m_pBranchProps->SetSelection(sel);
}

