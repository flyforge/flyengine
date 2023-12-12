#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>
#include <EditorFramework/InputContexts/SelectionContext.h>

plQtGameObjectViewWidget::plQtGameObjectViewWidget(QWidget* pParent, plQtGameObjectDocumentWindow* pOwnerWindow, PlasmaEngineViewConfig* pViewConfig)
  : plQtEngineViewWidget(pParent, pOwnerWindow, pViewConfig)
{
  m_pSelectionContext = PLASMA_DEFAULT_NEW(plSelectionContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);
  m_pCameraMoveContext = PLASMA_DEFAULT_NEW(plCameraMoveContext, pOwnerWindow, this);
  m_pOrthoGizmoContext = PLASMA_DEFAULT_NEW(plOrthoGizmoContext, pOwnerWindow, this, &m_pViewConfig->m_Camera);

  m_pCameraMoveContext->SetCamera(&m_pViewConfig->m_Camera);
  m_pCameraMoveContext->LoadState();

  // add the input contexts in the order in which they are supposed to be processed
  m_InputContexts.PushBack(m_pOrthoGizmoContext);
  m_InputContexts.PushBack(m_pSelectionContext);
  m_InputContexts.PushBack(m_pCameraMoveContext);
}

plQtGameObjectViewWidget::~plQtGameObjectViewWidget()
{
  PLASMA_DEFAULT_DELETE(m_pOrthoGizmoContext);
  PLASMA_DEFAULT_DELETE(m_pSelectionContext);
  PLASMA_DEFAULT_DELETE(m_pCameraMoveContext);
}

void plQtGameObjectViewWidget::SyncToEngine()
{
  m_pSelectionContext->SetWindowConfig(plVec2I32(width(), height()));

  plQtEngineViewWidget::SyncToEngine();
}

void plQtGameObjectViewWidget::HandleMarqueePickingResult(const plViewMarqueePickingResultMsgToEditor* pMsg)
{
  auto pSelMan = GetDocumentWindow()->GetDocument()->GetSelectionManager();
  auto pObjMan = GetDocumentWindow()->GetDocument()->GetObjectManager();

  if (m_uiLastMarqueeActionID != pMsg->m_uiActionIdentifier)
  {
    m_uiLastMarqueeActionID = pMsg->m_uiActionIdentifier;

    m_MarqueeBaseSelection.Clear();

    if (pMsg->m_uiWhatToDo == 0) // set selection
      pSelMan->Clear();

    const auto& curSel = pSelMan->GetSelection();
    for (auto pObj : curSel)
    {
      m_MarqueeBaseSelection.PushBack(pObj->GetGuid());
    }
  }

  plDeque<const plDocumentObject*> newSelection;

  for (plUuid guid : m_MarqueeBaseSelection)
  {
    auto pObject = pObjMan->GetObject(guid);
    newSelection.PushBack(pObject);
  }

  const plDocumentObject* pRoot = pObjMan->GetRootObject();

  for (plUuid guid : pMsg->m_ObjectGuids)
  {
    const plDocumentObject* pObject = pObjMan->GetObject(guid);

    if (pMsg->m_uiWhatToDo == 2) // remove from selection
    {
      // keep selection order
      newSelection.RemoveAndCopy(pObject);
    }
    else // add/set selection
    {
      if (!newSelection.Contains(pObject))
        newSelection.PushBack(pObject);
    }
  }

  pSelMan->SetSelection(newSelection);
}
