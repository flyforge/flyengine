#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAssetWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonPanel.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>

plQtSkeletonAssetDocumentWindow::plQtSkeletonAssetDocumentWindow(plSkeletonAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "SkeletonAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "SkeletonAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SkeletonAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  plQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig, true);
    m_pViewWidget->ConfigureRelative(plVec3(0, 0, 1), plVec3(5.0f), plVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "SkeletonAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("SkeletonAssetDockWidget");
    pPropertyPanel->setWindowTitle("Skeleton Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Tree View
  {
    plQtDocumentPanel* pPanelTree = new plQtSkeletonPanel(this, static_cast<plSkeletonAssetDocument*>(pDocument));
    pPanelTree->show();

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  pDocument->Events().AddEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::CommandEventHandler, this));

  FinishWindowCreation();
}

plQtSkeletonAssetDocumentWindow::~plQtSkeletonAssetDocumentWindow()
{
  static_cast<plSkeletonAssetDocument*>(GetDocument())->Events().RemoveEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler, this));

  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::CommandEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtSkeletonAssetDocumentWindow::PropertyEventHandler, this));

  RestoreResource();
}

plSkeletonAssetDocument* plQtSkeletonAssetDocumentWindow::GetSkeletonDocument()
{
  return static_cast<plSkeletonAssetDocument*>(GetDocument());
}

void plQtSkeletonAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  auto* pDoc = GetSkeletonDocument();

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderBones";
    msg.m_fPayload = pDoc->GetRenderBones() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderColliders";
    msg.m_fPayload = pDoc->GetRenderColliders() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderJoints";
    msg.m_fPayload = pDoc->GetRenderJoints() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderSwingLimits";
    msg.m_fPayload = pDoc->GetRenderSwingLimits() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "RenderTwistLimits";
    msg.m_fPayload = pDoc->GetRenderTwistLimits() ? 1.0f : 0.0f;
    pDoc->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";

    if (pDoc->GetRenderPreviewMesh())
      msg.m_sPayload = pDoc->GetProperties()->m_sPreviewMesh;
    else
      msg.m_sPayload = "";

    GetDocument()->SendMessageToEngine(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(true);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void plQtSkeletonAssetDocumentWindow::QueryObjectBBox(plInt32 iPurpose /*= 0*/)
{
  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}


void plQtSkeletonAssetDocumentWindow::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  plStringBuilder filter;

  switch (e.m_Type)
  {
    case plSelectionManagerEvent::Type::SelectionCleared:
    case plSelectionManagerEvent::Type::SelectionSet:
    case plSelectionManagerEvent::Type::ObjectAdded:
    case plSelectionManagerEvent::Type::ObjectRemoved:
    {
      const auto& sel = GetDocument()->GetSelectionManager()->GetSelection();

      for (auto pObj : sel)
      {
        plVariant name = pObj->GetTypeAccessor().GetValue("Name");
        if (name.IsValid() && name.CanConvertTo<plString>())
        {
          filter.Append(name.ConvertTo<plString>().GetData(), ";");
        }
      }

      plSimpleDocumentConfigMsgToEngine msg;
      msg.m_sWhatToDo = "HighlightBones";
      msg.m_sPayload = filter;

      GetDocument()->SendMessageToEngine(&msg);
    }
    break;
  }
}

void plQtSkeletonAssetDocumentWindow::SkeletonAssetEventHandler(const plSkeletonAssetEvent& e)
{
  if (e.m_Type == plSkeletonAssetEvent::Transformed)
  {
    SendLiveResourcePreview();
  }
}

void plQtSkeletonAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
{
  // additionally do live updates for these specific properties
  if (e.m_sProperty == "LocalRotation" ||                                                 // joint offset rotation
      e.m_sProperty == "Offset" || e.m_sProperty == "Rotation" ||                         // all shapes
      e.m_sProperty == "Radius" || e.m_sProperty == "Length" ||                           // sphere and capsule
      e.m_sProperty == "Width" || e.m_sProperty == "Thickness" ||                         // box
      e.m_sProperty == "SwingLimitY" || e.m_sProperty == "SwingLimitZ" ||                 // joint swing limit
      e.m_sProperty == "TwistLimitHalfAngle" || e.m_sProperty == "TwistLimitCenterAngle") // joint twist limit
  {
    SendLiveResourcePreview();
  }
}

void plQtSkeletonAssetDocumentWindow::CommandEventHandler(const plCommandHistoryEvent& e)
{
  if (e.m_Type == plCommandHistoryEvent::Type::TransactionEnded || e.m_Type == plCommandHistoryEvent::Type::UndoEnded || e.m_Type == plCommandHistoryEvent::Type::RedoEnded)
  {
    SendLiveResourcePreview();
  }
}

void plQtSkeletonAssetDocumentWindow::SendLiveResourcePreview()
{
  if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  plSkeletonAssetDocument* pDoc = plDynamicCast<plSkeletonAssetDocument*>(GetDocument());

  if (pDoc->m_bIsTransforming)
    return;

  plResourceUpdateMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  plContiguousMemoryStreamStorage streamStorage;
  plMemoryStreamWriter memoryWriter(&streamStorage);


  // Write Path
  plStringBuilder sAbsFilePath = pDoc->GetDocumentPath();
  sAbsFilePath.ChangeFileExtension("plSkeleton");

  // Write Header
  memoryWriter << sAbsFilePath;
  const plUInt64 uiHash = plAssetCurator::GetSingleton()->GetAssetDependencyHash(pDoc->GetGuid());
  plAssetFileHeader AssetHeader;
  AssetHeader.SetFileHashAndVersion(uiHash, pDoc->GetAssetTypeVersion());
  AssetHeader.Write(memoryWriter).IgnoreResult();

  // Write Asset Data
  pDoc->WriteResource(memoryWriter, *pDoc->GetProperties()).LogFailure();
  msg.m_Data = plArrayPtr<const plUInt8>(streamStorage.GetData(), streamStorage.GetStorageSize32());

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtSkeletonAssetDocumentWindow::RestoreResource()
{
  plRestoreResourceMsgToEngine msg;
  msg.m_sResourceType = "Skeleton";

  plStringBuilder tmp;
  msg.m_sResourceID = plConversionUtils::ToString(GetDocument()->GetGuid(), tmp);

  PlasmaEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}

void plQtSkeletonAssetDocumentWindow::InternalRedraw()
{
  PlasmaEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtSkeletonAssetDocumentWindow::ProcessMessageEventHandler(const PlasmaEditorEngineDocumentMsg* pMsg)
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
