#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/DocumentWindow/QuadViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <EditorPluginScene/Panels/ScenegraphPanel/ScenegraphPanel.moc.h>
#include <EditorPluginScene/Scene/SceneDocument.h>
#include <EditorPluginScene/Scene/SceneDocumentWindow.moc.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/ActionViews/MenuBarActionMapView.moc.h>
#include <GuiFoundation/ActionViews/ToolBarActionMapView.moc.h>
#include <GuiFoundation/PropertyGrid/PropertyGridWidget.moc.h>
#include <QInputDialog>
#include <ToolsFoundation/Object/ObjectAccessorBase.h>

plQtSceneDocumentWindow::plQtSceneDocumentWindow(plSceneDocument* pDocument)
  : plQtSceneDocumentWindowBase(pDocument)
{
  auto ViewFactory = [](plQtEngineDocumentWindow* pWindow, plEngineViewConfig* pConfig) -> plQtEngineViewWidget*
  {
    plQtSceneViewWidget* pWidget = new plQtSceneViewWidget(nullptr, static_cast<plQtSceneDocumentWindowBase*>(pWindow), pConfig);
    pWindow->AddViewWidget(pWidget);
    return pWidget;
  };
  m_pQuadViewWidget = new plQtQuadViewWidget(pDocument, this, ViewFactory, "EditorPluginScene_ViewToolBar");

  pDocument->SetEditToolConfigDelegate(
    [this](plGameObjectEditTool* pTool)
    { pTool->ConfigureTool(static_cast<plGameObjectDocument*>(GetDocument()), this, this); });

  setCentralWidget(m_pQuadViewWidget);

  plEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<plEditorPreferencesUser>();
  SetTargetFramerate(pPreferences->GetMaxFramerate());

  {
    // Menu Bar
    plQtMenuBarActionMapView* pMenuBar = static_cast<plQtMenuBarActionMapView*>(menuBar());
    plActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentMenuBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pMenuBar->SetActionContext(context);
  }

  {
    // Tool Bar
    plQtToolBarActionMapView* pToolBar = new plQtToolBarActionMapView("Toolbar", this);
    plActionContext context;
    context.m_sMapping = "EditorPluginScene_DocumentToolBar";
    context.m_pDocument = pDocument;
    context.m_pWindow = this;
    pToolBar->SetActionContext(context);
    pToolBar->setObjectName("SceneDocumentWindow_ToolBar");
    addToolBar(pToolBar);
  }

  {
    plQtDocumentPanel* pPropertyPanel = new plQtDocumentPanel(this, pDocument);
    pPropertyPanel->setObjectName("PropertyPanel");
    pPropertyPanel->setWindowTitle("Properties");
    pPropertyPanel->show();

    plQtDocumentPanel* pPanelTree = new plQtScenegraphPanel(this, static_cast<plSceneDocument*>(pDocument));
    pPanelTree->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPropertyPanel, pDocument);
    pPropertyPanel->setWidget(pPropertyGrid);
    PL_VERIFY(connect(pPropertyGrid, &plQtPropertyGridWidget::ExtendContextMenu, this, &plQtSceneDocumentWindow::ExtendPropertyGridContextMenu), "");

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPropertyPanel);
    addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, pPanelTree);
  }

  // Exposed Parameters
  if (GetSceneDocument()->IsPrefab())
  {
    plQtDocumentPanel* pPanel = new plQtDocumentPanel(this, pDocument);
    pPanel->setObjectName("SceneSettingsDockWidget");
    pPanel->setWindowTitle(GetSceneDocument()->IsPrefab() ? "Prefab Settings" : "Scene Settings");
    pPanel->show();

    plQtPropertyGridWidget* pPropertyGrid = new plQtPropertyGridWidget(pPanel, pDocument, false);
    plDeque<const plDocumentObject*> selection;
    selection.PushBack(pDocument->GetSettingsObject());
    pPropertyGrid->SetSelection(selection);
    pPanel->setWidget(pPropertyGrid);

    addDockWidget(Qt::DockWidgetArea::RightDockWidgetArea, pPanel);
  }

  FinishWindowCreation();
}

plQtSceneDocumentWindow::~plQtSceneDocumentWindow() = default;

plQtSceneDocumentWindowBase::plQtSceneDocumentWindowBase(plSceneDocument* pDocument)
  : plQtGameObjectDocumentWindow(pDocument)
{
  const plSceneDocument* pSceneDoc = static_cast<const plSceneDocument*>(GetDocument());
  pSceneDoc->m_GameObjectEvents.AddEventHandler(plMakeDelegate(&plQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

plQtSceneDocumentWindowBase::~plQtSceneDocumentWindowBase()
{
  GetSceneDocument()->m_GameObjectEvents.RemoveEventHandler(plMakeDelegate(&plQtSceneDocumentWindowBase::GameObjectEventHandler, this));
}

plSceneDocument* plQtSceneDocumentWindowBase::GetSceneDocument() const
{
  return static_cast<plSceneDocument*>(GetDocument());
}

void plQtSceneDocumentWindowBase::CreateImageCapture(const char* szOutputPath)
{
  m_pQuadViewWidget->GetActiveMainViews()[0]->GetViewWidget()->TakeScreenshot(szOutputPath);
}

void plQtSceneDocumentWindowBase::ToggleViews(QWidget* pView)
{
  m_pQuadViewWidget->ToggleViews(pView);
}


plObjectAccessorBase* plQtSceneDocumentWindowBase::GetObjectAccessor()
{
  return GetDocument()->GetObjectAccessor();
}

bool plQtSceneDocumentWindowBase::CanDuplicateSelection() const
{
  return true;
}

void plQtSceneDocumentWindowBase::DuplicateSelection()
{
  GetSceneDocument()->DuplicateSelection();
}

void plQtSceneDocumentWindowBase::SnapSelectionToPosition(bool bSnapEachObject)
{
  const float fSnap = plSnapProvider::GetTranslationSnapValue();

  if (fSnap == 0.0f)
    return;

  const plDeque<const plDocumentObject*>& selection = GetSceneDocument()->GetSelectionManager()->GetSelection();
  if (selection.IsEmpty())
    return;

  const auto& pivotObj = selection.PeekBack();

  plVec3 vPivotSnapOffset;

  if (!bSnapEachObject)
  {
    // if we snap by the pivot object only, the last selected object must be a valid game object
    if (!pivotObj->GetTypeAccessor().GetType()->IsDerivedFrom<plGameObject>())
      return;

    const plVec3 vPivotPos = GetSceneDocument()->GetGlobalTransform(pivotObj).m_vPosition;
    plVec3 vSnappedPos = vPivotPos;
    plSnapProvider::SnapTranslation(vSnappedPos);

    vPivotSnapOffset = vSnappedPos - vPivotPos;

    if (vPivotSnapOffset.IsZero())
      return;
  }

  plDeque<plSelectedGameObject> gizmoSelection;
  GetGameObjectDocument()->ComputeTopLevelSelectedGameObjects(gizmoSelection);

  if (gizmoSelection.IsEmpty())
    return;

  auto CmdHistory = GetDocument()->GetCommandHistory();

  CmdHistory->StartTransaction("Snap to Position");

  bool bDidAny = false;

  for (plUInt32 sel = 0; sel < gizmoSelection.GetCount(); ++sel)
  {
    const auto& obj = gizmoSelection[sel];

    plTransform vSnappedPos = obj.m_GlobalTransform;

    // if we snap each object individually, compute the snap position for each one here
    if (bSnapEachObject)
    {
      vSnappedPos.m_vPosition = obj.m_GlobalTransform.m_vPosition;
      plSnapProvider::SnapTranslation(vSnappedPos.m_vPosition);

      if (obj.m_GlobalTransform.m_vPosition == vSnappedPos.m_vPosition)
        continue;
    }
    else
    {
      // otherwise use the offset from the pivot point for repositioning
      vSnappedPos.m_vPosition += vPivotSnapOffset;
    }

    bDidAny = true;
    GetSceneDocument()->SetGlobalTransform(obj.m_pObject, vSnappedPos, TransformationChanges::Translation);
  }

  if (bDidAny)
    CmdHistory->FinishTransaction();
  else
    CmdHistory->CancelTransaction();

  gizmoSelection.Clear();

  ShowTemporaryStatusBarMsg(plFmt("Snap to Grid ({})", bSnapEachObject ? "Each Object" : "Pivot"));
}

void plQtSceneDocumentWindowBase::GameObjectEventHandler(const plGameObjectEvent& e)
{
  switch (e.m_Type)
  {
    case plGameObjectEvent::Type::TriggerFocusOnSelection_Hovered:
      // Focus is done by plQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(plSceneDocument::ShowOrHide::Show);
      break;

    case plGameObjectEvent::Type::TriggerFocusOnSelection_All:
      // Focus is done by plQtGameObjectDocumentWindow
      GetSceneDocument()->ShowOrHideSelectedObjects(plSceneDocument::ShowOrHide::Show);
      break;

    case plGameObjectEvent::Type::TriggerSnapSelectionPivotToGrid:
      SnapSelectionToPosition(false);
      break;

    case plGameObjectEvent::Type::TriggerSnapEachSelectedObjectToGrid:
      SnapSelectionToPosition(true);
      break;

    default:
      break;
  }
}

void plQtSceneDocumentWindowBase::InternalRedraw()
{
  // If play the game is on, only render (in editor) if the window is active
  plSceneDocument* doc = GetSceneDocument();
  if (doc->GetGameMode() == GameMode::Play && !window()->isActiveWindow())
    return;

  plEditorInputContext::UpdateActiveInputContext();
  SendRedrawMsg();
  plQtEngineDocumentWindow::InternalRedraw();
}

void plQtSceneDocumentWindowBase::SendRedrawMsg()
{
  // do not try to redraw while the process is crashed, it is obviously futile
  if (plEditorEngineProcessConnection::GetSingleton()->IsProcessCrashed())
    return;

  {
    plSimulationSettingsMsgToEngine msg;
    auto pSceneDoc = GetSceneDocument();
    msg.m_bSimulateWorld = pSceneDoc->GetGameMode() != GameMode::Off;
    msg.m_fSimulationSpeed = pSceneDoc->GetSimulationSpeed();
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
    pView->SetPickTransparent(GetGameObjectDocument()->GetPickTransparent());
    pView->UpdateCameraInterpolation();
    pView->SyncToEngine();
  }
}

void plQtSceneDocumentWindowBase::ExtendPropertyGridContextMenu(
  QMenu& menu, const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProp)
{
  if (!GetSceneDocument()->IsPrefab())
    return;

  plUInt32 iExposed = 0;
  for (plUInt32 i = 0; i < items.GetCount(); i++)
  {
    plInt32 index = GetSceneDocument()->FindExposedParameter(items[i].m_pObject, pProp, items[i].m_Index);
    if (index != -1)
      iExposed++;
  }
  menu.addSeparator();
  {
    QAction* pAction = menu.addAction("Expose as Parameter");
    pAction->setEnabled(iExposed < items.GetCount());
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]()
      {
      while (true)
      {
        bool bOk = false;
        QString name = QInputDialog::getText(this, "Parameter Name", "Name:", QLineEdit::Normal, pProp->GetPropertyName(), &bOk);

        if (!bOk)
          return;

        if (!plStringUtils::IsValidIdentifierName(name.toUtf8().data()))
        {
          plQtUiServices::GetSingleton()->MessageBoxInformation("This name is not a valid identifier.\nAllowed characters are a-z, A-Z, "
                                                                "0-9 and _.\nWhitespace and special characters are not allowed.");
          continue; // try again
        }

        auto pAccessor = GetSceneDocument()->GetObjectAccessor();
        pAccessor->StartTransaction("Expose as Parameter");
        for (const plPropertySelection& sel : items)
        {
          plInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
          if (index == -1)
          {
            GetSceneDocument()->AddExposedParameter(name.toUtf8(), sel.m_pObject, pProp, sel.m_Index).LogFailure();
          }
        }
        pAccessor->FinishTransaction();
        return;
      } });
  }
  {
    QAction* pAction = menu.addAction("Remove Exposed Parameter");
    pAction->setEnabled(iExposed > 0);
    connect(pAction, &QAction::triggered, pAction, [this, &menu, &items, pProp]()
      {
      auto pAccessor = GetSceneDocument()->GetObjectAccessor();
      pAccessor->StartTransaction("Remove Exposed Parameter");
      for (const plPropertySelection& sel : items)
      {
        plInt32 index = GetSceneDocument()->FindExposedParameter(sel.m_pObject, pProp, sel.m_Index);
        if (index != -1)
        {
          GetSceneDocument()->RemoveExposedParameter(index).LogFailure();
        }
      }
      pAccessor->FinishTransaction(); });
  }
}

void plQtSceneDocumentWindowBase::ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg)
{
  plQtGameObjectDocumentWindow::ProcessMessageEventHandler(pMsg);
}
