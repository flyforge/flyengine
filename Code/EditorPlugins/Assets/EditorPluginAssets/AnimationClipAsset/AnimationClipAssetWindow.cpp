#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/OrbitCamViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>
#include <EditorPluginAssets/AnimationClipAsset/AnimationClipAssetWindow.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/DockPanels/DocumentPanel.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

plQtAnimationClipAssetDocumentWindow::plQtAnimationClipAssetDocumentWindow(plAnimationClipAssetDocument* pDocument)
  : plQtEngineDocumentWindow(pDocument)
  , m_Clock("AssetClip")
{
  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "AnimationClipAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "AnimationClipAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("AnimationClipAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // 3D View
  plQtViewWidgetContainer* pContainer = nullptr;
  {
    SetTargetFramerate(25);

    m_ViewConfig.m_Camera.LookAt(plVec3(-1.6f, 0, 0), plVec3(0, 0, 0), plVec3(0, 0, 1));
    m_ViewConfig.ApplyPerspectiveSetting(90);

    m_pViewWidget = new plQtOrbitCamViewWidget(this, &m_ViewConfig);
    m_pViewWidget->ConfigureRelative(plVec3(0, 0, 1), plVec3(5.0f), plVec3(5, -2, 3), 2.0f);
    AddViewWidget(m_pViewWidget);
    pContainer = new plQtViewWidgetContainer(this, m_pViewWidget, "AnimationClipAssetViewToolBar");
    setCentralWidget(pContainer);
  }

  // Property Grid
  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("AnimationClipAssetDockWidget");
    pPropertyPanel->setWindowTitle("Animation Clip Properties");
    pPropertyPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);

    pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);
  }

  // Time Scrubber
  {
    m_pTimeScrubber = new plQtTimeScrubberWidget(pContainer);
    m_pTimeScrubber->SetDuration(plTime::MakeFromSeconds(1));

    pContainer->GetLayout()->addWidget(m_pTimeScrubber);

    connect(m_pTimeScrubber, &plQtTimeScrubberWidget::ScrubberPosChangedEvent, this, &plQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new plQtDocumentPanel(this, pDocument);
    m_pEventTrackPanel->setObjectName("AnimClipEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("Event Track");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new plQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pEventTrackPanel);

    UpdateEventTrackEditor();
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::InsertCpEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::CpMovedEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::CpDeletedEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted);

    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::BeginOperationEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::EndOperationEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::BeginCpChangesEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::EndCpChangesEvent, this, &plQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  FinishWindowCreation();

  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.AddEventHandler(plMakeDelegate(&plQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(plMakeDelegate(&plQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

plQtAnimationClipAssetDocumentWindow::~plQtAnimationClipAssetDocumentWindow()
{
  GetAnimationClipDocument()->m_CommonAssetUiChangeEvent.RemoveEventHandler(plMakeDelegate(&plQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(plMakeDelegate(&plQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler, this));
}

plAnimationClipAssetDocument* plQtAnimationClipAssetDocumentWindow::GetAnimationClipDocument()
{
  return static_cast<plAnimationClipAssetDocument*>(GetDocument());
}

void plQtAnimationClipAssetDocumentWindow::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PlaybackPos";
    msg.m_fPayload = m_PlaybackPosition.GetSeconds() / m_ClipDuration.GetSeconds();
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "PreviewMesh";
    msg.m_sPayload = GetAnimationClipDocument()->GetProperties()->m_sPreviewMesh;
    GetDocument()->SendMessageToEngine(&msg);
  }

  {
    plSimpleDocumentConfigMsgToEngine msg;
    msg.m_sWhatToDo = "SimulationSpeed";

    if (GetAnimationClipDocument()->GetCommonAssetUiState(plCommonAssetUiState::Pause) != 0.0f)
      msg.m_fPayload = 0.0;
    else
      msg.m_fPayload = GetAnimationClipDocument()->GetCommonAssetUiState(plCommonAssetUiState::SimulationSpeed);

    GetEditorEngineConnection()->SendMessage(&msg);
  }

  for (auto pView : m_ViewWidgets)
  {
    pView->SetEnablePicking(false);
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }

  QueryObjectBBox();
}

void plQtAnimationClipAssetDocumentWindow::QueryObjectBBox(plInt32 iPurpose /*= 0*/)
{
  plQuerySelectionBBoxMsgToEngine msg;
  msg.m_uiViewID = 0xFFFFFFFF;
  msg.m_iPurpose = iPurpose;
  GetDocument()->SendMessageToEngine(&msg);
}

void plQtAnimationClipAssetDocumentWindow::UpdateEventTrackEditor()
{
  auto* pDoc = GetAnimationClipDocument();

  m_pEventTrackEditor->SetData(pDoc->GetProperties()->m_EventTrack, m_ClipDuration.GetSeconds());
}

void plQtAnimationClipAssetDocumentWindow::InternalRedraw()
{
  if (m_pTimeScrubber == nullptr)
    return;

  if (m_ClipDuration.IsPositive())
  {
    m_Clock.Update();

    const double fSpeed = GetAnimationClipDocument()->GetCommonAssetUiState(plCommonAssetUiState::SimulationSpeed);

    if (GetAnimationClipDocument()->GetCommonAssetUiState(plCommonAssetUiState::Pause) == 0)
    {
      m_PlaybackPosition += m_Clock.GetTimeDiff() * fSpeed;
    }

    if (m_PlaybackPosition > m_ClipDuration)
    {
      if (GetAnimationClipDocument()->GetCommonAssetUiState(plCommonAssetUiState::Loop) != 0)
      {
        m_PlaybackPosition -= m_ClipDuration;
      }
      else
      {
        m_PlaybackPosition = m_ClipDuration;
      }
    }
  }

  m_PlaybackPosition = plMath::Clamp(m_PlaybackPosition, plTime::MakeZero(), m_ClipDuration);
  m_pTimeScrubber->SetScrubberPosition(m_PlaybackPosition);
  m_pEventTrackEditor->SetScrubberPosition(m_PlaybackPosition);

  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtAnimationClipAssetDocumentWindow::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg0)
{
  if (auto pMsg = plDynamicCast<const plQuerySelectionBBoxResultMsgToEditor*>(pMsg0))
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

  if (auto pMsg = plDynamicCast<const plSimpleDocumentConfigMsgToEditor*>(pMsg0))
  {
    if (pMsg->m_sName == "ClipDuration")
    {
      const plTime newDuration = plTime::MakeFromSeconds(pMsg->m_fPayload);

      if (m_ClipDuration != newDuration)
      {
        m_ClipDuration = newDuration;

        m_pTimeScrubber->SetDuration(m_ClipDuration);

        UpdateEventTrackEditor();
      }
    }
  }

  plQtEngineDocumentWindow::ProcessMessageEventHandler(pMsg0);
}

void plQtAnimationClipAssetDocumentWindow::CommonAssetUiEventHandler(const plCommonAssetUiState& e)
{
  plQtEngineDocumentWindow::CommonAssetUiEventHandler(e);

  if (e.m_State == plCommonAssetUiState::Restart)
  {
    m_PlaybackPosition = plTime::MakeFromSeconds(-1);
  }
}

void plQtAnimationClipAssetDocumentWindow::OnScrubberPosChangedEvent(plUInt64 uiNewScrubberTickPos)
{
  if (m_pTimeScrubber == nullptr || m_ClipDuration.IsZeroOrNegative())
    return;

  m_PlaybackPosition = plTime::MakeFromSeconds(uiNewScrubberTickPos / 4800.0);
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackInsertCpAt(plInt64 tickX, QString value)
{
  auto* pDoc = GetAnimationClipDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackCpMoved(plUInt32 cpIdx, plInt64 iTickX)
{
  iTickX = plMath::Max<plInt64>(iTickX, 0);

  auto* pDoc = GetAnimationClipDocument();

  plObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const plUuid trackGuid = accessor.Get<plUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const plDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const plVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackCpDeleted(plUInt32 cpIdx)
{
  auto* pDoc = GetAnimationClipDocument();

  plObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plAnimationClipAssetProperties>()->FindPropertyByName("EventTrack");
  const plUuid trackGuid = accessor.Get<plUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const plDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const plVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).AssertSuccess();
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void plQtAnimationClipAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

void plQtAnimationClipAssetDocumentWindow::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  // also listen to TransactionCanceled, which is sent when a no-op happens (e.g. asset transform with no change)
  // because the event track data object may still get replaced, and we have to get the new pointer
  if (e.m_Type == plCommandHistoryEvent::Type::TransactionEnded || e.m_Type == plCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == plCommandHistoryEvent::Type::RedoEnded ||
      e.m_Type == plCommandHistoryEvent::Type::TransactionCanceled)
  {
    UpdateEventTrackEditor();
  }
}
