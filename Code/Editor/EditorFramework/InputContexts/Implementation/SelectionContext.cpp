#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/SelectionContext.h>
#include <Foundation/Utilities/GraphicsUtils.h>

plSelectionContext::plSelectionContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera)
{
  m_pCamera = pCamera;

  SetOwner(pOwnerWindow, pOwnerView);

  m_hMarqueeGizmo.ConfigureHandle(nullptr, plEngineGizmoHandleType::LineBox, plColor::CadetBlue, plGizmoFlags::ShowInOrtho | plGizmoFlags::OnTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hMarqueeGizmo);
}

plSelectionContext::~plSelectionContext()
{
  // if anyone is registered for object picking, tell them that nothing was picked,
  // so that they reset their state
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride(nullptr);
    ResetPickObjectOverride();
  }
}

void plSelectionContext::SetPickObjectOverride(plDelegate<void(const plDocumentObject*)> pickOverride)
{
  m_PickObjectOverride = pickOverride;
  GetOwnerView()->setCursor(Qt::CrossCursor);
}

void plSelectionContext::ResetPickObjectOverride()
{
  if (m_PickObjectOverride.IsValid())
  {
    m_PickObjectOverride.Invalidate();
    GetOwnerView()->unsetCursor();
  }
}

plEditorInput plSelectionContext::DoMousePressEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::LeftButton)
  {
    const plObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

    if (res.m_PickedOther.IsValid())
    {
      auto pSO = GetOwnerWindow()->GetDocument()->FindSyncObject(res.m_PickedOther);

      if (pSO != nullptr)
      {
        if (pSO->GetDynamicRTTI()->IsDerivedFrom<plGizmoHandle>())
        {
          plGizmoHandle* pGizmoHandle = static_cast<plGizmoHandle*>(pSO);
          plGizmo* pGizmo = pGizmoHandle->GetOwnerGizmo();

          if (pGizmo)
          {
            pGizmo->ConfigureInteraction(pGizmoHandle, m_pCamera, res.m_vPickedPosition, m_vViewport);
            return pGizmo->MousePressEvent(e);
          }
        }
      }
    }

    m_Mode = Mode::Single;

    if (m_bPressedSpace && !m_PickObjectOverride.IsValid())
    {
      m_uiMarqueeID += 23;
      m_vMarqueeStartPos.Set(e->pos().x(), e->pos().y(), 0.01f);

      // only shift -> add, shift AND control -> remove
      m_Mode = e->modifiers().testFlag(Qt::ControlModifier) ? Mode::MarqueeRemove : Mode::MarqueeAdd;
      MakeActiveInputContext();

      if (m_Mode == Mode::MarqueeAdd)
        m_hMarqueeGizmo.SetColor(plColor::LightSkyBlue);
      else
        m_hMarqueeGizmo.SetColor(plColor::PaleVioletRed);

      return plEditorInput::WasExclusivelyHandled;
    }
  }

  return plEditorInput::MayBeHandledByOthers;
}

plEditorInput plSelectionContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == Qt::MouseButton::MiddleButton)
  {
    if (e->modifiers() & Qt::KeyboardModifier::ControlModifier)
    {
      const plObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      OpenDocumentForPickedObject(res);
    }
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    if (m_Mode == Mode::Single)
    {
      const plObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      const bool bToggle = (e->modifiers() & Qt::KeyboardModifier::ControlModifier) != 0;
      const bool bDirect = (e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0;
      SelectPickedObject(res, bToggle, bDirect);

      DoFocusLost(false);

      // we handled the mouse click event
      // but this is it, we don't stay active
      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
    {
      SendMarqueeMsg(e, (m_Mode == Mode::MarqueeAdd) ? 1 : 2);

      const bool bPressedSpace = m_bPressedSpace;
      DoFocusLost(false);
      m_bPressedSpace = bPressedSpace;
      return plEditorInput::WasExclusivelyHandled;
    }
  }

  return plEditorInput::MayBeHandledByOthers;
}


void plSelectionContext::OpenDocumentForPickedObject(const plObjectPickingResult& res) const
{
  if (!res.m_PickedComponent.IsValid())
    return;

  auto* pDocument = GetOwnerWindow()->GetDocument();

  const plDocumentObject* pPickedComponent = pDocument->GetObjectManager()->GetObject(res.m_PickedComponent);

  for (auto pDocMan : plDocumentManager::GetAllDocumentManagers())
  {
    if (plAssetDocumentManager* pAssetMan = plDynamicCast<plAssetDocumentManager*>(pDocMan))
    {
      if (pAssetMan->OpenPickedDocument(pPickedComponent, res.m_uiPartIndex).Succeeded())
      {
        return;
      }
    }
  }

  GetOwnerWindow()->ShowTemporaryStatusBarMsg("Could not open a document for the picked object");
}

void plSelectionContext::SelectPickedObject(const plObjectPickingResult& res, bool bToggle, bool bDirect) const
{
  if (res.m_PickedObject.IsValid())
  {
    auto* pDocument = GetOwnerWindow()->GetDocument();
    const plDocumentObject* pObject = pDocument->GetObjectManager()->GetObject(res.m_PickedObject);
    if (!pObject)
      return;

    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(pObject);
    }
    else
    {
      if (bToggle)
        pDocument->GetSelectionManager()->ToggleObject(determineObjectToSelect(pObject, true, bDirect));
      else
        pDocument->GetSelectionManager()->SetSelection(determineObjectToSelect(pObject, false, bDirect));
    }
  }
}

void plSelectionContext::SendMarqueeMsg(QMouseEvent* e, plUInt8 uiWhatToDo)
{
  plVec2I32 curPos;
  curPos.Set(e->pos().x(), e->pos().y());

  plMat4 mView = m_pCamera->GetViewMatrix();
  plMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);

  plMat4 mViewProj = mProj * mView;
  plMat4 mInvViewProj = mViewProj;
  if (mInvViewProj.Invert(0.0f).Failed())
  {
    // if this fails, the marquee will not be rendered correctly
    PLASMA_ASSERT_DEBUG(false, "Failed to invert view projection matrix.");
  }

  const plVec3 vMousePos(e->pos().x(), e->pos().y(), 0.01f);

  const plVec3 vScreenSpacePos0(vMousePos.x, m_vViewport.y - vMousePos.y, vMousePos.z);
  const plVec3 vScreenSpacePos1(m_vMarqueeStartPos.x, m_vViewport.y - m_vMarqueeStartPos.y, m_vMarqueeStartPos.z);

  plVec3 vPosOnNearPlane0, vRayDir0;
  plVec3 vPosOnNearPlane1, vRayDir1;
  plGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos0, vPosOnNearPlane0, &vRayDir0).IgnoreResult();
  plGraphicsUtils::ConvertScreenPosToWorldPos(mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vScreenSpacePos1, vPosOnNearPlane1, &vRayDir1).IgnoreResult();

  plTransform t;
  t.SetIdentity();
  t.m_vPosition = plMath::Lerp(vPosOnNearPlane0, vPosOnNearPlane1, 0.5f);
  t.m_qRotation = plQuat::MakeFromMat3(m_pCamera->GetViewMatrix().GetRotationalPart());

  // box coordinates in screen space
  plVec3 vBoxPosSS0 = t.m_qRotation * vPosOnNearPlane0;
  plVec3 vBoxPosSS1 = t.m_qRotation * vPosOnNearPlane1;

  t.m_qRotation = t.m_qRotation.GetInverse();

  t.m_vScale.x = plMath::Abs(vBoxPosSS0.x - vBoxPosSS1.x);
  t.m_vScale.y = plMath::Abs(vBoxPosSS0.y - vBoxPosSS1.y);
  t.m_vScale.z = 0.0f;

  m_hMarqueeGizmo.SetTransformation(t);
  m_hMarqueeGizmo.SetVisible(true);

  {
    plViewMarqueePickingMsgToEngine msg;
    msg.m_uiViewID = GetOwnerView()->GetViewID();
    msg.m_uiPickPosX0 = (plUInt16)m_vMarqueeStartPos.x;
    msg.m_uiPickPosY0 = (plUInt16)m_vMarqueeStartPos.y;
    msg.m_uiPickPosX1 = (plUInt16)(e->pos().x());
    msg.m_uiPickPosY1 = (plUInt16)(e->pos().y());
    msg.m_uiWhatToDo = uiWhatToDo;
    msg.m_uiActionIdentifier = m_uiMarqueeID;

    GetOwnerView()->GetDocumentWindow()->GetDocument()->SendMessageToEngine(&msg);
  }
}

plEditorInput plSelectionContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (IsActiveInputContext() && (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove))
  {
    SendMarqueeMsg(e, 0xFF);

    return plEditorInput::WasExclusivelyHandled;
  }
  else
  {
    plViewHighlightMsgToEngine msg;

    {
      const plObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

      if (res.m_PickedComponent.IsValid())
        msg.m_HighlightObject = res.m_PickedComponent;
      else if (res.m_PickedOther.IsValid())
        msg.m_HighlightObject = res.m_PickedOther;
      else
        msg.m_HighlightObject = res.m_PickedObject;
    }

    GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

    // we only updated the highlight, so others may do additional stuff, if they like
    return plEditorInput::MayBeHandledByOthers;
  }
}

plEditorInput plSelectionContext::DoKeyPressEvent(QKeyEvent* e)
{
  /// \todo Handle the current cursor (icon) across all active input contexts

  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = true;
    return plEditorInput::MayBeHandledByOthers;
  }

  if (e->key() == Qt::Key_Delete)
  {
    GetOwnerWindow()->GetDocument()->DeleteSelectedObjects();
    return plEditorInput::WasExclusivelyHandled;
  }

  if (e->key() == Qt::Key_Escape)
  {
    if (m_PickObjectOverride.IsValid())
    {
      m_PickObjectOverride(nullptr);
      ResetPickObjectOverride();
    }
    else
    {
      if (m_Mode == Mode::MarqueeAdd || m_Mode == Mode::MarqueeRemove)
      {
        const bool bPressedSpace = m_bPressedSpace;
        FocusLost(true);
        m_bPressedSpace = bPressedSpace;
      }
      else
      {
        GetOwnerWindow()->GetDocument()->GetSelectionManager()->Clear();
      }
    }

    return plEditorInput::WasExclusivelyHandled;
  }

  return plEditorInput::MayBeHandledByOthers;
}

plEditorInput plSelectionContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_Space)
  {
    m_bPressedSpace = false;
  }

  return plEditorInput::MayBeHandledByOthers;
}

static const bool IsInSelection(const plDeque<const plDocumentObject*>& selection, const plDocumentObject* pObject, const plDocumentObject*& out_pParentInSelection, const plDocumentObject*& out_pParentChild, const plDocumentObject* pRootObject)
{
  if (pObject == pRootObject)
    return false;

  if (selection.IndexOf(pObject) != plInvalidIndex)
  {
    out_pParentInSelection = pObject;
    return true;
  }

  const plDocumentObject* pParent = pObject->GetParent();

  if (IsInSelection(selection, pParent, out_pParentInSelection, out_pParentChild, pRootObject))
  {
    if (out_pParentChild == nullptr)
      out_pParentChild = pObject;

    return true;
  }

  return false;
}

static const plDocumentObject* GetPrefabParentOrSelf(const plDocumentObject* pObject)
{
  const plDocumentObject* pParent = pObject;
  const plDocument* pDocument = pObject->GetDocumentObjectManager()->GetDocument();
  const auto& metaData = *pDocument->m_DocumentObjectMetaData;

  while (pParent != nullptr)
  {
    {
      const plDocumentObjectMetaData* pMeta = metaData.BeginReadMetaData(pParent->GetGuid());
      bool bIsPrefab = pMeta->m_CreateFromPrefab.IsValid();
      metaData.EndReadMetaData();

      if (bIsPrefab)
        return pParent;
    }
    pParent = pParent->GetParent();
  }

  return pObject;
}

const plDocumentObject* plSelectionContext::determineObjectToSelect(const plDocumentObject* pickedObject, bool bToggle, bool bDirect) const
{
  auto* pDocument = GetOwnerWindow()->GetDocument();
  const plDeque<const plDocumentObject*> sel = pDocument->GetSelectionManager()->GetSelection();

  const plDocumentObject* pRootObject = pDocument->GetObjectManager()->GetRootObject();

  const plDocumentObject* pParentInSelection = nullptr;
  const plDocumentObject* pParentChild = nullptr;

  if (!IsInSelection(sel, pickedObject, pParentInSelection, pParentChild, pRootObject))
  {
    if (bDirect)
      return pickedObject;

    return GetPrefabParentOrSelf(pickedObject);
  }
  else
  {
    if (bToggle)
    {
      // always toggle the object that is already in the selection
      return pParentInSelection;
    }

    if (bDirect)
      return pickedObject;

    if (sel.GetCount() > 1)
    {
      // multi-selection, but no toggle, so we are about to set the selection
      // -> always use the top-level parent in this case
      return GetPrefabParentOrSelf(pickedObject);
    }

    if (pParentInSelection == pickedObject)
    {
      // object itself is in the selection
      return pickedObject;
    }

    if (pParentChild == nullptr)
    {
      return pParentInSelection;
    }

    return pParentChild;
  }
}

void plSelectionContext::DoFocusLost(bool bCancel)
{
  plEditorInputContext::DoFocusLost(bCancel);

  m_bPressedSpace = false;
  m_Mode = Mode::None;
  m_hMarqueeGizmo.SetVisible(false);

  if (IsActiveInputContext())
    MakeActiveInputContext(false);
}
