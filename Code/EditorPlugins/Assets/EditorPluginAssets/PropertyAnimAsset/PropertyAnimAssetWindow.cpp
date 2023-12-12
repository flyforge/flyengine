#include <EditorPluginAssets/EditorPluginAssetsPCH.h>

#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectModel.moc.h>
#include <EditorFramework/Panels/GameObjectPanel/GameObjectPanel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAsset.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimAssetWindow.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimModel.moc.h>
#include <EditorPluginAssets/PropertyAnimAsset/PropertyAnimObjectManager.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <GuiFoundation/Widgets/ColorGradientEditorWidget.moc.h>
#include <GuiFoundation/Widgets/Curve1DEditorWidget.moc.h>
#include <GuiFoundation/Widgets/EventTrackEditorWidget.moc.h>
#include <GuiFoundation/Widgets/TimeScrubberWidget.moc.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>

plQtPropertyAnimAssetDocumentWindow::plQtPropertyAnimAssetDocumentWindow(plPropertyAnimAssetDocument* pDocument)
  : plQtGameObjectDocumentWindow(pDocument)
{
  auto ViewFactory = [](plQtEngineDocumentWindow* pWindow, PlasmaEngineViewConfig* pConfig) -> plQtEngineViewWidget* {
    plQtGameObjectViewWidget* pWidget = new plQtGameObjectViewWidget(nullptr, static_cast<plQtPropertyAnimAssetDocumentWindow*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new plQtQuadViewWidget(pDocument, this, ViewFactory, "PropertyAnimAssetViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](plGameObjectEditTool* pTool) { pTool->ConfigureTool(static_cast<plGameObjectDocument*>(GetDocument()), this, this); });

  pDocument->m_PropertyAnimEvents.AddEventHandler(plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));

  setCentralWidget(m_pQuadViewWidget);
  SetTargetFramerate(25);

  // Menu Bar
  {
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "PropertyAnimAssetMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  // Tool Bar
  {
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "PropertyAnimAssetToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("PropertyAnimAssetWindowToolBar");
    addToolBar(pToolBar);
  }

  // Game Object Graph
  {
    std::unique_ptr<plQtDocumentTreeModel> pModel(new plQtGameObjectModel(pDocument->GetObjectManager()));
    pModel->AddAdapter(new plQtDummyAdapter(pDocument->GetObjectManager(), plGetStaticRTTI<plDocumentRoot>(), "TempObjects"));
    pModel->AddAdapter(new plQtGameObjectAdapter(pDocument->GetObjectManager()));

    plQtDocumentPanel* pGameObjectPanel = new plQtGameObjectPanel(this, pDocument, "PropertyAnimAsset_ScenegraphContextMenu", std::move(pModel));
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pGameObjectPanel);
  }

  // Property Grid
  {
    plQtDocumentPanel* pPanel = new plQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("PropertyAnimAssetDockWidget");
    pPanel->setWindowTitle("OBJECT PROPERTIES");
    pPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPanel, pDocument);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  // Property Tree View
  {
    plQtDocumentPanel* pPanel = new plQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("PropertyAnimPropertiesDockWidget");
    pPanel->setWindowTitle("ANIMATED PROPERTIES");
    pPanel->show();

    m_pPropertyTreeView = new plQtPropertyAnimAssetTreeView(pPanel);
    m_pPropertyTreeView->setHeaderHidden(true);
    m_pPropertyTreeView->setRootIsDecorated(true);
    m_pPropertyTreeView->setUniformRowHeights(true);
    m_pPropertyTreeView->setExpandsOnDoubleClick(false);
    pPanel->setWidget(m_pPropertyTreeView);

    connect(m_pPropertyTreeView, &plQtPropertyAnimAssetTreeView::DeleteSelectedItemsEvent, this,
      &plQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems);
    connect(m_pPropertyTreeView, &plQtPropertyAnimAssetTreeView::RebindSelectedItemsEvent, this,
      &plQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems);

    connect(m_pPropertyTreeView, &QTreeView::doubleClicked, this, &plQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked);
    connect(m_pPropertyTreeView, &plQtPropertyAnimAssetTreeView::FrameSelectedItemsEvent, this,
      &plQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks);

    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanel);
  }

  // Property Model
  {
    m_pPropertiesModel = new plQtPropertyAnimModel(GetPropertyAnimDocument(), this);
    m_pPropertyTreeView->setModel(m_pPropertiesModel);
    m_pPropertyTreeView->expandToDepth(2);
    m_pPropertyTreeView->initialize();
  }

  // Selection Model
  {
    m_pPropertyTreeView->setSelectionBehavior(QAbstractItemView::SelectionBehavior::SelectRows);
    m_pPropertyTreeView->setSelectionMode(QAbstractItemView::SelectionMode::ExtendedSelection);

    m_pSelectionModel = new QItemSelectionModel(m_pPropertiesModel, this);
    m_pPropertyTreeView->setSelectionModel(m_pSelectionModel);

    connect(m_pSelectionModel, &QItemSelectionModel::selectionChanged, this, &plQtPropertyAnimAssetDocumentWindow::onSelectionChanged);
  }

  // Float Curve Panel
  {
    m_pCurvePanel = new plQtDocumentPanel(this, pDocument);
    m_pCurvePanel->setObjectName("PropertyAnimFloatCurveDockWidget");
    m_pCurvePanel->setWindowTitle("CURVES");
    m_pCurvePanel->show();

    m_pCurveEditor = new plQtCurve1DEditorWidget(m_pCurvePanel);
    m_pCurvePanel->setWidget(m_pCurveEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pCurvePanel);
  }

  // Color Gradient Panel
  {
    m_pColorGradientPanel = new plQtDocumentPanel(this, pDocument);
    m_pColorGradientPanel->setObjectName("PropertyAnimColorGradientDockWidget");
    m_pColorGradientPanel->setWindowTitle("COLOR GRADIENT");
    m_pColorGradientPanel->show();

    m_pGradientEditor = new plQtColorGradientEditorWidget(m_pColorGradientPanel);
    m_pColorGradientPanel->setWidget(m_pGradientEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pColorGradientPanel);
  }

  // Event Track Panel
  {
    m_pEventTrackPanel = new plQtDocumentPanel(this, pDocument);
    m_pEventTrackPanel->setObjectName("PropertyAnimEventTrackDockWidget");
    m_pEventTrackPanel->setWindowTitle("EVENT TRACK");
    m_pEventTrackPanel->show();

    m_pEventTrackEditor = new plQtEventTrackEditorWidget(m_pEventTrackPanel);
    m_pEventTrackPanel->setWidget(m_pEventTrackEditor);

    addDockWidget(Qt::DockWidgetArea::BottomDockWidgetArea, m_pEventTrackPanel);
  }

  // Time Scrubber
  {
    m_pScrubberToolbar = new plQtTimeScrubberToolbar(this);
    connect(m_pScrubberToolbar, &plQtTimeScrubberToolbar::ScrubberPosChangedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged);
    connect(m_pScrubberToolbar, &plQtTimeScrubberToolbar::PlayPauseEvent, this, &plQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked);
    connect(m_pScrubberToolbar, &plQtTimeScrubberToolbar::RepeatEvent, this, &plQtPropertyAnimAssetDocumentWindow::onRepeatClicked);
    connect(m_pScrubberToolbar, &plQtTimeScrubberToolbar::DurationChangedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent);
    connect(m_pScrubberToolbar, &plQtTimeScrubberToolbar::AdjustDurationEvent, this, &plQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked);

    addToolBar(Qt::ToolBarArea::BottomToolBarArea, m_pScrubberToolbar);
  }

  // this would show the document properties
  // pDocument->GetSelectionManager()->SetSelection(pDocument->GetObjectManager()->GetRootObject()->GetChildren()[0]);

  // Curve editor events
  {
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::InsertCpEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpMovedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveCpMoved);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpDeletedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::TangentMovedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::TangentLinkEvent, this, &plQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::CpTangentModeEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged);

    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::BeginOperationEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::EndOperationEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveEndOperation);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::BeginCpChangesEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges);
    connect(m_pCurveEditor, &plQtCurve1DEditorWidget::EndCpChangesEvent, this, &plQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges);
  }

  // Gradient editor events
  {
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpAdded, this, &plQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpMoved, this, &plQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpDeleted, this, &plQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::ColorCpChanged, this, &plQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged);

    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpAdded, this, &plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpMoved, this, &plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpDeleted, this, &plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::AlphaCpChanged, this, &plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged);

    connect(
      m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpAdded, this, &plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded);
    connect(
      m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpMoved, this, &plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpDeleted, this,
      &plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::IntensityCpChanged, this,
      &plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged);

    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::BeginOperation, this, &plQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation);
    connect(m_pGradientEditor, &plQtColorGradientEditorWidget::EndOperation, this, &plQtPropertyAnimAssetDocumentWindow::onGradientEndOperation);

    // connect(m_pGradientEditor, &plQtColorGradientEditorWidget::NormalizeRange, this,
    // &plQtPropertyAnimAssetDocumentWindow::onGradientNormalizeRange);
  }

  // Event track editor events
  {
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::InsertCpEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::CpMovedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved);
    connect(m_pEventTrackEditor, &plQtEventTrackEditorWidget::CpDeletedEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted);

    connect(
      m_pEventTrackEditor, &plQtEventTrackEditorWidget::BeginOperationEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation);
    connect(
      m_pEventTrackEditor, &plQtEventTrackEditorWidget::EndOperationEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation);
    connect(
      m_pEventTrackEditor, &plQtEventTrackEditorWidget::BeginCpChangesEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges);
    connect(
      m_pEventTrackEditor, &plQtEventTrackEditorWidget::EndCpChangesEvent, this, &plQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges);
  }

  // GetDocument()->GetObjectManager()->m_PropertyEvents.AddEventHandler(plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.AddEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.AddEventHandler(plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.AddEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));

  FinishWindowCreation();

  {
    const plUInt64 uiDuration = GetPropertyAnimDocument()->GetAnimationDurationTicks();
    m_pScrubberToolbar->SetDuration(uiDuration);
  }

  UpdateCurveEditor();
  UpdateGradientEditor();
  UpdateEventTrackEditor();
}

plQtPropertyAnimAssetDocumentWindow::~plQtPropertyAnimAssetDocumentWindow()
{
  GetPropertyAnimDocument()->m_PropertyAnimEvents.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler, this));
  // GetDocument()->GetObjectManager()->m_PropertyEvents.RemoveEventHandler(plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::PropertyEventHandler,
  // this));
  GetDocument()->GetObjectManager()->m_StructureEvents.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::StructureEventHandler, this));
  GetDocument()->GetSelectionManager()->m_Events.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::SelectionEventHandler, this));
  GetDocument()->GetCommandHistory()->m_Events.RemoveEventHandler(
    plMakeDelegate(&plQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler, this));
}

void plQtPropertyAnimAssetDocumentWindow::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}

plObjectAccessorBase* plQtPropertyAnimAssetDocumentWindow::GetObjectAccessor()
{
  return GetPropertyAnimDocument()->GetObjectAccessor();
}

bool plQtPropertyAnimAssetDocumentWindow::CanDuplicateSelection() const
{
  return false;
}

void plQtPropertyAnimAssetDocumentWindow::DuplicateSelection()
{
  PLASMA_ASSERT_NOT_IMPLEMENTED;
}


void plQtPropertyAnimAssetDocumentWindow::InternalRedraw()
{
  PlasmaEditorInputContext::UpdateActiveInputContext();
  {
    // do not try to redraw while the process is crashed, it is obviously futile
    if (PlasmaEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
      return;

    {
      plSimulationSettingsMsgToEngine msg;
      msg.m_bSimulateWorld = false;
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      plGridSettingsMsgToEngine msg = GetGridSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }
    {
      plWorldSettingsMsgToEngine msg = GetWorldSettings();
      GetEditorEngineConnection()->SendMessage(&msg);
    }

    GetGameObjectDocument()->SendObjectSelection();

    auto pHoveredView = GetHoveredViewWidget();

    for (auto pView : m_ViewWidgets)
    {
      pView->SetEnablePicking(pView == pHoveredView);
      pView->UpdateCameraInterpolation();
      pView->SyncToEngine();
    }
  }
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtPropertyAnimAssetDocumentWindow::PropertyAnimAssetEventHandler(const plPropertyAnimAssetDocumentEvent& e)
{
  if (e.m_Type == plPropertyAnimAssetDocumentEvent::Type::AnimationLengthChanged)
  {
    const plUInt64 uiDuration = e.m_pDocument->GetAnimationDurationTicks();

    m_pScrubberToolbar->SetDuration(uiDuration);
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
  else if (e.m_Type == plPropertyAnimAssetDocumentEvent::Type::ScrubberPositionChanged)
  {
    m_pScrubberToolbar->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pCurveEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pGradientEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
    m_pEventTrackEditor->SetScrubberPosition(e.m_pDocument->GetScrubberPosition());
  }
  else if (e.m_Type == plPropertyAnimAssetDocumentEvent::Type::PlaybackChanged)
  {
    if (!m_bAnimTimerInFlight && GetPropertyAnimDocument()->GetPlayAnimation())
    {
      m_bAnimTimerInFlight = true;
      QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
    }

    m_pScrubberToolbar->SetButtonState(GetPropertyAnimDocument()->GetPlayAnimation(), GetPropertyAnimDocument()->GetRepeatAnimation());
  }
}

void plQtPropertyAnimAssetDocumentWindow::onSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
  UpdateSelectionData();
}

void plQtPropertyAnimAssetDocumentWindow::UpdateSelectionData()
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  m_MapSelectionToTrack.Clear();
  m_pGradientToDisplay = nullptr;
  m_iMapGradientToTrack = -1;
  m_CurvesToDisplay.Clear();
  m_CurvesToDisplay.m_bOwnsData = false;
  m_CurvesToDisplay.m_uiFramesPerSecond = pDoc->GetProperties()->m_uiFramesPerSecond;

  plSet<plInt32> tracks;

  for (const QModelIndex& selIdx : m_pSelectionModel->selection().indexes())
  {
    plQtPropertyAnimModelTreeEntry* pTreeItem =
      reinterpret_cast<plQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(selIdx, plQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

    plQtPropertyAnimModel* pModel = m_pPropertiesModel;

    auto addRecursive = [&tracks, pModel](auto& self, const plQtPropertyAnimModelTreeEntry* pTreeItem) -> void {
      if (pTreeItem->m_pTrack != nullptr)
        tracks.Insert(pTreeItem->m_iTrackIdx);

      for (plInt32 iChild : pTreeItem->m_Children)
      {
        // cannot use 'addRecursive' here, because the name is not yet fully defined
        self(self, &pModel->GetAllEntries()[iChild]);
      }
    };

    addRecursive(addRecursive, pTreeItem);
  }

  auto& trackArray = pDoc->GetProperties()->m_Tracks;
  for (auto it = tracks.GetIterator(); it.IsValid(); ++it)
  {
    const plInt32 iTrackIdx = it.Key();

    // this can happen during undo/redo when the selection still names data that has just been removed
    if (iTrackIdx >= (plInt32)trackArray.GetCount())
      continue;

    if (trackArray[iTrackIdx]->m_Target != plPropertyAnimTarget::Color)
    {
      m_MapSelectionToTrack.PushBack(iTrackIdx);

      m_CurvesToDisplay.m_Curves.PushBack(&trackArray[iTrackIdx]->m_FloatCurve);
    }
    else
    {
      m_pGradientToDisplay = &trackArray[iTrackIdx]->m_ColorGradient;
      m_iMapGradientToTrack = iTrackIdx;
    }
  }

  m_pCurveEditor->ClearSelection();

  UpdateCurveEditor();
  UpdateGradientEditor();

  if (m_pEventTrackPanel->hasFocus() || m_pEventTrackEditor->hasFocus() || m_pEventTrackEditor->EventTrackEdit->hasFocus())
  {
  }
  else if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pColorGradientPanel->raise();
  }
}

void plQtPropertyAnimAssetDocumentWindow::onScrubberPosChanged(plUInt64 uiTick)
{
  GetPropertyAnimDocument()->SetScrubberPosition(uiTick);
}

void plQtPropertyAnimAssetDocumentWindow::onDeleteSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  pHistory->StartTransaction("Delete Tracks");

  m_pGradientToDisplay = nullptr;
  m_CurvesToDisplay.Clear();

  // delete the tracks with the highest index first, otherwise the lower indices become invalid
  // do this before modifying anything, as m_MapSelectionToTrack will change once the remove commands are executed
  plHybridArray<plInt32, 16> sortedTrackIDs;
  {
    for (plInt32 iTrack : m_MapSelectionToTrack)
    {
      sortedTrackIDs.PushBack(iTrack);
    }

    if (m_iMapGradientToTrack >= 0)
    {
      sortedTrackIDs.PushBack(m_iMapGradientToTrack);
    }

    sortedTrackIDs.Sort();
  }

  for (plUInt32 i = sortedTrackIDs.GetCount(); i > 0; --i)
  {
    const plInt32 iTrack = sortedTrackIDs[i - 1];

    const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
    {
      plRemoveObjectCommand cmd;
      cmd.m_Object = trackGuid.Get<plUuid>();

      pHistory->AddCommand(cmd).IgnoreResult();
    }
  }

  m_MapSelectionToTrack.Clear();
  m_iMapGradientToTrack = -1;

  pHistory->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onRebindSelectedItems()
{
  auto pDoc = GetPropertyAnimDocument();
  auto pHistory = pDoc->GetCommandHistory();

  plHybridArray<plUuid, 16> rebindTracks;

  for (plInt32 iTrack : m_MapSelectionToTrack)
  {
    const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<plUuid>());
  }

  if (m_iMapGradientToTrack >= 0)
  {
    const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);

    if (trackGuid.IsValid())
      rebindTracks.PushBack(trackGuid.Get<plUuid>());
  }

  bool ok = false;
  QString result = QInputDialog::getText(this, "Change Animation Binding", "New Binding Path:", QLineEdit::Normal, "", &ok);

  if (!ok)
    return;

  m_pSelectionModel->clear();

  plStringBuilder path = result.toUtf8().data();
  ;
  path.MakeCleanPath();
  const plVariant varRes = path.GetData();

  pHistory->StartTransaction("Rebind Tracks");

  for (const plUuid guid : rebindTracks)
  {
    plSetObjectPropertyCommand cmdSet;
    cmdSet.m_Object = guid;

    cmdSet.m_sProperty = "ObjectPath";
    cmdSet.m_NewValue = varRes;
    pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
  }

  pHistory->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onPlaybackTick()
{
  m_bAnimTimerInFlight = false;

  if (!GetPropertyAnimDocument()->GetPlayAnimation())
    return;

  GetPropertyAnimDocument()->ExecuteAnimationPlaybackStep();

  m_bAnimTimerInFlight = true;
  QTimer::singleShot(0, this, SLOT(onPlaybackTick()));
}

void plQtPropertyAnimAssetDocumentWindow::onPlayPauseClicked()
{
  GetPropertyAnimDocument()->SetPlayAnimation(!GetPropertyAnimDocument()->GetPlayAnimation());
}

void plQtPropertyAnimAssetDocumentWindow::onRepeatClicked()
{
  GetPropertyAnimDocument()->SetRepeatAnimation(!GetPropertyAnimDocument()->GetRepeatAnimation());
}

void plQtPropertyAnimAssetDocumentWindow::onAdjustDurationClicked()
{
  GetPropertyAnimDocument()->AdjustDuration();
}

void plQtPropertyAnimAssetDocumentWindow::onDurationChangedEvent(double duration)
{
  GetPropertyAnimDocument()->SetAnimationDurationTicks((plUInt64)(duration * 4800.0));
}

void plQtPropertyAnimAssetDocumentWindow::onTreeItemDoubleClicked(const QModelIndex& index)
{
  plQtPropertyAnimModelTreeEntry* pTreeItem =
    reinterpret_cast<plQtPropertyAnimModelTreeEntry*>(m_pPropertiesModel->data(index, plQtPropertyAnimModel::UserRoles::TreeItem).value<void*>());

  if (pTreeItem != nullptr && pTreeItem->m_pTrack != nullptr)
  {
    if (pTreeItem->m_pTrack->m_Target == plPropertyAnimTarget::Color)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
    else
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
  }
  else
  {
    if (!m_CurvesToDisplay.m_Curves.IsEmpty())
    {
      m_pCurveEditor->FrameCurve();
      m_pCurvePanel->raise();
    }
    else if (m_pGradientToDisplay != nullptr)
    {
      m_pGradientEditor->FrameGradient();
      m_pColorGradientPanel->raise();
    }
  }
}

void plQtPropertyAnimAssetDocumentWindow::onFrameSelectedTracks()
{
  if (!m_CurvesToDisplay.m_Curves.IsEmpty())
  {
    m_pCurveEditor->FrameCurve();
    m_pCurvePanel->raise();
  }
  else if (m_pGradientToDisplay != nullptr)
  {
    m_pGradientEditor->FrameGradient();
    m_pColorGradientPanel->raise();
  }
}

plPropertyAnimAssetDocument* plQtPropertyAnimAssetDocumentWindow::GetPropertyAnimDocument()
{
  return static_cast<plPropertyAnimAssetDocument*>(GetDocument());
}

// void plQtPropertyAnimAssetDocumentWindow::PropertyEventHandler(const plDocumentObjectPropertyEvent& e)
//{
//  if (static_cast<plPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pObject, e.m_sProperty))
//    return;
//
//  // TODO: only update what needs to be updated
//
//  //m_bUpdateEventTrackEditor = true;
//  //m_bUpdateCurveEditor = true;
//  //m_bUpdateGradientEditor = true;
//}

void plQtPropertyAnimAssetDocumentWindow::StructureEventHandler(const plDocumentObjectStructureEvent& e)
{
  if (e.m_pNewParent &&
      static_cast<plPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pNewParent, e.m_sParentProperty))
    return;
  if (e.m_pPreviousParent &&
      static_cast<plPropertyAnimObjectManager*>(GetDocument()->GetObjectManager())->IsTemporary(e.m_pPreviousParent, e.m_sParentProperty))
    return;

  switch (e.m_EventType)
  {
    case plDocumentObjectStructureEvent::Type::AfterObjectAdded:
    case plDocumentObjectStructureEvent::Type::AfterObjectRemoved:
    case plDocumentObjectStructureEvent::Type::AfterObjectMoved2:
      UpdateSelectionData();
      break;

    default:
      break;
  }
}


void plQtPropertyAnimAssetDocumentWindow::SelectionEventHandler(const plSelectionManagerEvent& e)
{
  // this would show the document properties
  // if (GetDocument()->GetSelectionManager()->IsSelectionEmpty())
  //{
  //  // delayed execution
  //  QTimer::singleShot(1, [this]()
  //  {
  //    GetDocument()->GetSelectionManager()->SetSelection(GetPropertyAnimDocument()->GetPropertyObject());
  //  });
  //}
}


void plQtPropertyAnimAssetDocumentWindow::CommandHistoryEventHandler(const plCommandHistoryEvent& e)
{
  if (e.m_Type == plCommandHistoryEvent::Type::TransactionEnded || e.m_Type == plCommandHistoryEvent::Type::UndoEnded ||
      e.m_Type == plCommandHistoryEvent::Type::RedoEnded)
  {
    UpdateCurveEditor();
    UpdateGradientEditor();
    UpdateEventTrackEditor();
  }
}

void plQtPropertyAnimAssetDocumentWindow::UpdateCurveEditor()
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pCurveEditor->SetCurveExtents(0, pDoc->GetAnimationDurationTime().GetSeconds(), true, true);
  m_pCurveEditor->SetCurves(m_CurvesToDisplay);
}


void plQtPropertyAnimAssetDocumentWindow::UpdateGradientEditor()
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_pGradientToDisplay == nullptr || m_iMapGradientToTrack < 0)
  {
    // TODO: clear gradient editor ?
    plColorGradient empty;
    m_pGradientEditor->SetColorGradient(empty);
  }
  else
  {
    plColorGradient gradient;
    m_pGradientToDisplay->FillGradientData(gradient);
    m_pGradientEditor->SetColorGradient(gradient);
  }
}


void plQtPropertyAnimAssetDocumentWindow::UpdateEventTrackEditor()
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  m_pEventTrackEditor->SetData(GetPropertyAnimDocument()->GetProperties()->m_EventTrack, pDoc->GetAnimationDurationTime().GetSeconds());
}

void plQtPropertyAnimAssetDocumentWindow::onCurveBeginOperation(QString name)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands(name.toUtf8().data());
}

void plQtPropertyAnimAssetDocumentWindow::onCurveEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void plQtPropertyAnimAssetDocumentWindow::onCurveBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void plQtPropertyAnimAssetDocumentWindow::onCurveEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateCurveEditor();
}

void plQtPropertyAnimAssetDocumentWindow::onCurveInsertCpAt(plUInt32 uiCurveIdx, plInt64 tickX, double clickPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  pDoc->InsertCurveCpAt(trackGuid.Get<plUuid>(), tickX, clickPosY);
}

void plQtPropertyAnimAssetDocumentWindow::onCurveCpMoved(plUInt32 uiCurveIdx, plUInt32 cpIdx, plInt64 iTickX, double newPosY)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  iTickX = plMath::Max<plInt64>(iTickX, 0);

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Value";
  cmdSet.m_NewValue = newPosY;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
}

void plQtPropertyAnimAssetDocumentWindow::onCurveCpDeleted(plUInt32 uiCurveIdx, plUInt32 cpIdx)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
}

void plQtPropertyAnimAssetDocumentWindow::onCurveTangentMoved(plUInt32 uiCurveIdx, plUInt32 cpIdx, float newPosX, float newPosY, bool rightTangent)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  // clamp tangents to one side
  if (rightTangent)
    newPosX = plMath::Max(newPosX, 0.0f);
  else
    newPosX = plMath::Min(newPosX, 0.0f);

  cmdSet.m_sProperty = rightTangent ? "RightTangent" : "LeftTangent";
  cmdSet.m_NewValue = plVec2(newPosX, newPosY);
  GetDocument()->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
}

void plQtPropertyAnimAssetDocumentWindow::onLinkCurveTangents(plUInt32 uiCurveIdx, plUInt32 cpIdx, bool bLink)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdLink;
  cmdLink.m_Object = cpGuid.Get<plUuid>();
  cmdLink.m_sProperty = "Linked";
  cmdLink.m_NewValue = bLink;
  GetDocument()->GetCommandHistory()->AddCommand(cmdLink).IgnoreResult();

  if (bLink)
  {
    const plVec2 leftTangent = pDoc->GetProperties()->m_Tracks[iTrackIdx]->m_FloatCurve.m_ControlPoints[cpIdx].m_LeftTangent;
    const plVec2 rightTangent(-leftTangent.x, -leftTangent.y);

    onCurveTangentMoved(uiCurveIdx, cpIdx, rightTangent.x, rightTangent.y, true);
  }
}

void plQtPropertyAnimAssetDocumentWindow::onCurveTangentModeChanged(plUInt32 uiCurveIdx, plUInt32 cpIdx, bool rightTangent, int mode)
{
  if (uiCurveIdx >= m_MapSelectionToTrack.GetCount())
    return;

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  auto pProp = pDoc->GetPropertyObject();

  const plInt32 iTrackIdx = m_MapSelectionToTrack[uiCurveIdx];
  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", iTrackIdx);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plVariant curveGuid = trackObject->GetTypeAccessor().GetValue("FloatCurve");

  const plDocumentObject* pCurvesArray = pDoc->GetObjectManager()->GetObject(curveGuid.Get<plUuid>());
  const plVariant cpGuid = pCurvesArray->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmd;
  cmd.m_Object = cpGuid.Get<plUuid>();
  cmd.m_sProperty = rightTangent ? "RightTangentMode" : "LeftTangentMode";
  cmd.m_NewValue = mode;
  GetDocument()->GetCommandHistory()->AddCommand(cmd).IgnoreResult();
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


void plQtPropertyAnimAssetDocumentWindow::onGradientColorCpAdded(double posX, const plColorGammaUB& color)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  plInt64 tickX = plColorGradientAssetData::TickFromTime(plTime::Seconds(posX));
  pDoc->InsertGradientColorCpAt(trackGuid.Get<plUuid>(), tickX, color);
}


void plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpAdded(double posX, plUInt8 alpha)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  plInt64 tickX = plColorGradientAssetData::TickFromTime(plTime::Seconds(posX));
  pDoc->InsertGradientAlphaCpAt(trackGuid.Get<plUuid>(), tickX, alpha);
}


void plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpAdded(double posX, float intensity)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  plInt64 tickX = plColorGradientAssetData::TickFromTime(plTime::Seconds(posX));
  pDoc->InsertGradientIntensityCpAt(trackGuid.Get<plUuid>(), tickX, intensity);
}

void plQtPropertyAnimAssetDocumentWindow::MoveGradientCP(plInt32 idx, double newPosX, const char* szArrayName)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  plVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Move Control Point");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = pDoc->GetProperties()->m_Tracks[m_iMapGradientToTrack]->m_ColorGradient.TickFromTime(plTime::Seconds(newPosX));
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onGradientColorCpMoved(plInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "ColorCPs");
}

void plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpMoved(plInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "AlphaCPs");
}


void plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpMoved(plInt32 idx, double newPosX)
{
  MoveGradientCP(idx, newPosX, "IntensityCPs");
}

void plQtPropertyAnimAssetDocumentWindow::RemoveGradientCP(plInt32 idx, const char* szArrayName)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  plVariant objGuid = gradientObject->GetTypeAccessor().GetValue(szArrayName, idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Remove Control Point");

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onGradientColorCpDeleted(plInt32 idx)
{
  RemoveGradientCP(idx, "ColorCPs");
}

void plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpDeleted(plInt32 idx)
{
  RemoveGradientCP(idx, "AlphaCPs");
}

void plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpDeleted(plInt32 idx)
{
  RemoveGradientCP(idx, "IntensityCPs");
}

void plQtPropertyAnimAssetDocumentWindow::onGradientColorCpChanged(plInt32 idx, const plColorGammaUB& color)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  plVariant objGuid = gradientObject->GetTypeAccessor().GetValue("ColorCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Color");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Red";
  cmdSet.m_NewValue = color.r;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Green";
  cmdSet.m_NewValue = color.g;
  history->AddCommand(cmdSet).IgnoreResult();

  cmdSet.m_sProperty = "Blue";
  cmdSet.m_NewValue = color.b;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}


void plQtPropertyAnimAssetDocumentWindow::onGradientAlphaCpChanged(plInt32 idx, plUInt8 alpha)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  plVariant objGuid = gradientObject->GetTypeAccessor().GetValue("AlphaCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Alpha");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Alpha";
  cmdSet.m_NewValue = alpha;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onGradientIntensityCpChanged(plInt32 idx, float intensity)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  if (m_iMapGradientToTrack < 0)
    return;

  const plVariant trackGuid = pDoc->GetPropertyObject()->GetTypeAccessor().GetValue("Tracks", m_iMapGradientToTrack);
  const plDocumentObject* trackObject = pDoc->GetObjectManager()->GetObject(trackGuid.Get<plUuid>());
  const plUuid gradientGuid = trackObject->GetTypeAccessor().GetValue("Gradient").Get<plUuid>();
  const plDocumentObject* gradientObject = pDoc->GetObjectManager()->GetObject(gradientGuid);

  plVariant objGuid = gradientObject->GetTypeAccessor().GetValue("IntensityCPs", idx);

  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->StartTransaction("Change Intensity");

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = objGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Intensity";
  cmdSet.m_NewValue = intensity;
  history->AddCommand(cmdSet).IgnoreResult();

  history->FinishTransaction();
}

void plQtPropertyAnimAssetDocumentWindow::onGradientBeginOperation()
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Gradient");
}

void plQtPropertyAnimAssetDocumentWindow::onGradientEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackInsertCpAt(plInt64 tickX, QString value)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();
  pDoc->InsertEventTrackCpAt(tickX, value.toUtf8().data());
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackCpMoved(plUInt32 cpIdx, plInt64 iTickX)
{
  iTickX = plMath::Max<plInt64>(iTickX, 0);

  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  plObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const plUuid trackGuid = accessor.Get<plUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const plDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const plVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  plSetObjectPropertyCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();

  cmdSet.m_sProperty = "Tick";
  cmdSet.m_NewValue = iTickX;
  pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackCpDeleted(plUInt32 cpIdx)
{
  plPropertyAnimAssetDocument* pDoc = GetPropertyAnimDocument();

  plObjectCommandAccessor accessor(pDoc->GetCommandHistory());

  const plAbstractProperty* pTrackProp = plGetStaticRTTI<plPropertyAnimationTrackGroup>()->FindPropertyByName("EventTrack");
  const plUuid trackGuid = accessor.Get<plUuid>(pDoc->GetPropertyObject(), pTrackProp);
  const plDocumentObject* pTrackObj = accessor.GetObject(trackGuid);

  const plVariant cpGuid = pTrackObj->GetTypeAccessor().GetValue("ControlPoints", cpIdx);

  if (!cpGuid.IsValid())
    return;

  plRemoveObjectCommand cmdSet;
  cmdSet.m_Object = cpGuid.Get<plUuid>();
  pDoc->GetCommandHistory()->AddCommand(cmdSet).IgnoreResult();
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackBeginOperation(QString name)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();
  history->BeginTemporaryCommands("Modify Events");
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackEndOperation(bool commit)
{
  plCommandHistory* history = GetDocument()->GetCommandHistory();

  if (commit)
    history->FinishTemporaryCommands();
  else
    history->CancelTemporaryCommands();
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackBeginCpChanges(QString name)
{
  GetDocument()->GetCommandHistory()->StartTransaction(name.toUtf8().data());
}

void plQtPropertyAnimAssetDocumentWindow::onEventTrackEndCpChanges()
{
  GetDocument()->GetCommandHistory()->FinishTransaction();

  UpdateEventTrackEditor();
}

//////////////////////////////////////////////////////////////////////////

plQtPropertyAnimAssetTreeView::plQtPropertyAnimAssetTreeView(QWidget* parent)
  : QTreeView(parent)
{
  setContextMenuPolicy(Qt::ContextMenuPolicy::DefaultContextMenu);
}

void plQtPropertyAnimAssetTreeView::initialize()
{
  connect(model(), &QAbstractItemModel::modelAboutToBeReset, this, &plQtPropertyAnimAssetTreeView::onBeforeModelReset);
  connect(model(), &QAbstractItemModel::modelReset, this, &plQtPropertyAnimAssetTreeView::onAfterModelReset);
}

void plQtPropertyAnimAssetTreeView::storeExpandState(const QModelIndex& parent)
{
  const QAbstractItemModel* pModel = model();

  const plUInt32 numRows = pModel->rowCount(parent);
  for (plUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    const bool expanded = isExpanded(idx);

    QString path = pModel->data(idx, plQtPropertyAnimModel::UserRoles::Path).toString();

    if (!expanded)
      m_NotExpandedState.insert(path);

    storeExpandState(idx);
  }
}

void plQtPropertyAnimAssetTreeView::restoreExpandState(const QModelIndex& parent, QModelIndexList& newSelection)
{
  const QAbstractItemModel* pModel = model();

  const plUInt32 numRows = pModel->rowCount(parent);
  for (plUInt32 row = 0; row < numRows; ++row)
  {
    QModelIndex idx = pModel->index(row, 0, parent);

    QString path = pModel->data(idx, plQtPropertyAnimModel::UserRoles::Path).toString();

    const bool notExpanded = m_NotExpandedState.contains(path);

    if (!notExpanded)
      setExpanded(idx, true);

    if (m_SelectedItems.contains(path))
      newSelection.append(idx);

    restoreExpandState(idx, newSelection);
  }
}

void plQtPropertyAnimAssetTreeView::onBeforeModelReset()
{
  m_NotExpandedState.clear();
  m_SelectedItems.clear();

  storeExpandState(QModelIndex());

  const QAbstractItemModel* pModel = model();

  for (QModelIndex idx : selectionModel()->selectedRows())
  {
    QString path = pModel->data(idx, plQtPropertyAnimModel::UserRoles::Path).toString();
    m_SelectedItems.insert(path);
  }
}

void plQtPropertyAnimAssetTreeView::onAfterModelReset()
{
  QModelIndexList newSelection;
  restoreExpandState(QModelIndex(), newSelection);

  // changing the selection is not possible in onAfterModelReset, probably because the items are not yet fully valid
  // has to be done shortly after
  QTimer::singleShot(0, this, [this, newSelection]() {
    selectionModel()->clearSelection();
    for (const auto& idx : newSelection)
    {
      selectionModel()->select(idx, QItemSelectionModel::SelectionFlag::Select | QItemSelectionModel::SelectionFlag::Rows);
    }
  });
}

void plQtPropertyAnimAssetTreeView::keyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key::Key_Delete)
  {
    Q_EMIT DeleteSelectedItemsEvent();
  }
  else
  {
    QTreeView::keyPressEvent(e);
  }
}

void plQtPropertyAnimAssetTreeView::contextMenuEvent(QContextMenuEvent* event)
{
  QMenu m;
  m.setToolTipsVisible(true);
  QAction* pFrameAction = m.addAction("Frame Curve");
  QAction* pRemoveAction = m.addAction("Remove Track");
  QAction* pBindingAction = m.addAction("Change Binding...");
  m.setDefaultAction(pFrameAction);

  pRemoveAction->setShortcut(Qt::Key_Delete);

  connect(pFrameAction, &QAction::triggered, this, [this](bool) { Q_EMIT FrameSelectedItemsEvent(); });
  connect(pRemoveAction, &QAction::triggered, this, [this](bool) { Q_EMIT DeleteSelectedItemsEvent(); });
  connect(pBindingAction, &QAction::triggered, this, [this](bool) { Q_EMIT RebindSelectedItemsEvent(); });

  m.exec(QCursor::pos());
}
