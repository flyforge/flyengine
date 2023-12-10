#include <EditorPluginScene/EditorPluginScenePCH.h>

#include <EditorFramework/Actions/GameObjectSelectionActions.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DragDrop/DragDropHandler.h>
#include <EditorFramework/DragDrop/DragDropInfo.h>
#include <EditorPluginScene/Actions/SceneActions.h>
#include <EditorPluginScene/Actions/SelectionActions.h>
#include <EditorPluginScene/InputContexts/SceneSelectionContext.h>
#include <EditorPluginScene/Scene/Scene2Document.h>
#include <EditorPluginScene/Scene/SceneViewWidget.moc.h>
#include <GuiFoundation/Action/ActionMapManager.h>
#include <GuiFoundation/Action/EditActions.h>
#include <GuiFoundation/ActionViews/MenuActionMapView.moc.h>
#include <QKeyEvent>

bool plQtSceneViewWidget::s_bContextMenuInitialized = false;

plQtSceneViewWidget::plQtSceneViewWidget(QWidget* pParent, plQtGameObjectDocumentWindow* pOwnerWindow, plEngineViewConfig* pViewConfig)
  : plQtGameObjectViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  setAcceptDrops(true);

  m_bAllowPickSelectedWhileDragging = false;

  if (plDynamicCast<plScene2Document*>(pOwnerWindow->GetDocument()))
  {
    //#TODO Not the cleanest solution but this replaces the default selection context of the base class.
    const plUInt32 uiSelectionIndex = m_InputContexts.IndexOf(m_pSelectionContext);
    PLASMA_DEFAULT_DELETE(m_pSelectionContext);
    m_pSelectionContext = PLASMA_DEFAULT_NEW(plSceneSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
    m_InputContexts[uiSelectionIndex] = m_pSelectionContext;
  }
}

plQtSceneViewWidget::~plQtSceneViewWidget() {}

bool plQtSceneViewWidget::IsPickingAgainstSelectionAllowed() const
{
  if (m_bInDragAndDropOperation && m_bAllowPickSelectedWhileDragging)
  {
    return true;
  }

  return plQtEngineViewWidget::IsPickingAgainstSelectionAllowed();
}

void plQtSceneViewWidget::OnOpenContextMenu(QPoint globalPos)
{
  if (!s_bContextMenuInitialized)
  {
    s_bContextMenuInitialized = true;

    plActionMapManager::RegisterActionMap("SceneViewContextMenu").IgnoreResult();

    plGameObjectSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    plSelectionActions::MapViewContextMenuActions("SceneViewContextMenu");
    plEditActions::MapViewContextMenuActions("SceneViewContextMenu");
    plSceneActions::MapViewContextMenuActions("SceneViewContextMenu");
  }

  {
    plQtMenuActionMapView menu(nullptr);

    plActionContext context;
    context.m_sMapping = "SceneViewContextMenu";
    context.m_pDocument = GetDocumentWindow()->GetDocument();
    context.m_pWindow = this;
    menu.SetActionContext(context);

    menu.exec(globalPos);
  }
}

void plQtSceneViewWidget::dragEnterEvent(QDragEnterEvent* e)
{
  plQtEngineViewWidget::dragEnterEvent(e);

  // can only drag & drop objects around in perspective mode
  // when dragging between two windows, the editor crashes
  // can be reproduced with two perspective windows as well
  // if (m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective)
  // return;

  m_LastDragMoveEvent = plTime::Now();
  m_bAllowPickSelectedWhileDragging = false;

  {
    plObjectPickingResult res = PickObject(e->pos().x(), e->pos().y());

    plDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    plDragDropConfig cfg;
    if (plDragDropHandler::BeginDragDropOperation(&info, &cfg))
    {
      m_bAllowPickSelectedWhileDragging = cfg.m_bPickSelectedObjects;

      e->acceptProposedAction();
      return;
    }
  }

  m_bInDragAndDropOperation = false;
}

void plQtSceneViewWidget::dragLeaveEvent(QDragLeaveEvent* e)
{
  plDragDropHandler::CancelDragDrop();

  plQtEngineViewWidget::dragLeaveEvent(e);
}

void plQtSceneViewWidget::dragMoveEvent(QDragMoveEvent* e)
{
  const plTime tNow = plTime::Now();

  if (tNow - m_LastDragMoveEvent < plTime::Seconds(1.0 / 25.0))
    return;

  m_LastDragMoveEvent = tNow;

  if (plDragDropHandler::IsHandlerActive())
  {
    plObjectPickingResult res = PickObject(e->pos().x(), e->pos().y());

    plDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    plDragDropHandler::UpdateDragDropOperation(&info);
  }
}

void plQtSceneViewWidget::dropEvent(QDropEvent* e)
{
  if (plDragDropHandler::IsHandlerActive())
  {
    plObjectPickingResult res = PickObject(e->pos().x(), e->pos().y());

    plDragDropInfo info;
    info.m_pMimeData = e->mimeData();
    info.m_TargetDocument = GetDocumentWindow()->GetDocument()->GetGuid();
    info.m_sTargetContext = "viewport";
    info.m_iTargetObjectInsertChildIndex = -1;
    info.m_vDropPosition = res.m_vPickedPosition;
    info.m_vDropNormal = res.m_vPickedNormal;
    info.m_iTargetObjectSubID = res.m_uiPartIndex;
    info.m_TargetObject = res.m_PickedObject;
    info.m_TargetComponent = res.m_PickedComponent;
    info.m_bShiftKeyDown = e->keyboardModifiers() & Qt::ShiftModifier;
    info.m_bCtrlKeyDown = e->keyboardModifiers() & Qt::ControlModifier;

    plDragDropHandler::FinishDragDrop(&info);
  }

  plQtEngineViewWidget::dropEvent(e);
}
