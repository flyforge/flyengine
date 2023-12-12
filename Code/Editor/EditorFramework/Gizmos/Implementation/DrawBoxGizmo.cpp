#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DrawBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDrawBoxGizmo, 1, plRTTINoAllocator)
  ;
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plDrawBoxGizmo::plDrawBoxGizmo()
{
  m_ManipulateMode = ManipulateMode::None;

  m_vLastStartPoint.SetZero();
  m_hBox.ConfigureHandle(this, PlasmaEngineGizmoHandleType::LineBox, plColorLinearUB(255, 100, 0), plGizmoFlags::ShowInOrtho);

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
}

plDrawBoxGizmo::~plDrawBoxGizmo() {}

void plDrawBoxGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBox);
}

void plDrawBoxGizmo::OnVisibleChanged(bool bVisible) {}

void plDrawBoxGizmo::OnTransformationChanged(const plTransform& transform) {}

void plDrawBoxGizmo::DoFocusLost(bool bCancel)
{
  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
  UpdateBox();

  if (IsActiveInputContext())
    SetActiveInputContext(nullptr);
}

bool plDrawBoxGizmo::PickPosition(QMouseEvent* e)
{
  const QPoint mousePos = GetOwnerWindow()->mapFromGlobal(QCursor::pos());

  const plObjectPickingResult& res = GetOwnerView()->PickObject(mousePos.x(), mousePos.y());

  m_vUpAxis = GetOwnerView()->GetFallbackPickingPlane().m_vNormal;
  m_vUpAxis.x = plMath::Abs(m_vUpAxis.x);
  m_vUpAxis.y = plMath::Abs(m_vUpAxis.y);
  m_vUpAxis.z = plMath::Abs(m_vUpAxis.z);

  if (res.m_PickedObject.IsValid() && !e->modifiers().testFlag(Qt::ShiftModifier))
  {
    m_vCurrentPosition = res.m_vPickedPosition;
  }
  else
  {
    if (GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), GetOwnerView()->GetFallbackPickingPlane(m_vLastStartPoint), m_vCurrentPosition).Failed())
    {
      return false;
    }
  }

  plSnapProvider::SnapTranslation(m_vCurrentPosition);
  return true;
}

PlasmaEditorInput plDrawBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (e->buttons() == Qt::LeftButton && e->modifiers().testFlag(Qt::ControlModifier))
  {
    if (m_ManipulateMode == ManipulateMode::None)
    {
      if (!PickPosition(e))
      {
        return PlasmaEditorInput::WasExclusivelyHandled; // failed to pick anything
      }

      m_vLastStartPoint = m_vCurrentPosition;
      SwitchMode(false);
      return PlasmaEditorInput::WasExclusivelyHandled;
    }
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plDrawBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::LeftButton)
  {
    if (m_ManipulateMode == ManipulateMode::DrawBase || m_ManipulateMode == ManipulateMode::DrawHeight)
    {
      SwitchMode(m_vFirstCorner == m_vSecondCorner);
      return PlasmaEditorInput::WasExclusivelyHandled;
    }
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plDrawBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  UpdateGrid(e);

  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    const plVec2I32 vMouseMove = plVec2I32(e->globalPos().x(), e->globalPos().y()) - m_vLastMousePos;
    m_iHeightChange -= vMouseMove.y;

    m_vLastMousePos = UpdateMouseMode(e);
  }
  else
  {
    plPlane plane;
    plane.SetFromNormalAndPoint(m_vUpAxis, m_vFirstCorner);

    GetOwnerView()->PickPlane(e->pos().x(), e->pos().y(), plane, m_vCurrentPosition).IgnoreResult();

    plSnapProvider::SnapTranslation(m_vCurrentPosition);
  }

  UpdateBox();

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plDrawBoxGizmo::DoKeyPressEvent(QKeyEvent* e)
{
  // is the gizmo in general visible == is it active
  if (!IsVisible())
    return PlasmaEditorInput::MayBeHandledByOthers;

  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  if (e->key() == Qt::Key_Escape)
  {
    if (m_ManipulateMode != ManipulateMode::None)
    {
      SwitchMode(true);
      return PlasmaEditorInput::WasExclusivelyHandled;
    }
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plDrawBoxGizmo::DoKeyReleaseEvent(QKeyEvent* e)
{
  DisableGrid(e->modifiers().testFlag(Qt::ControlModifier));

  return PlasmaEditorInput::MayBeHandledByOthers;
}

void plDrawBoxGizmo::SwitchMode(bool bCancel)
{
  plGizmoEvent e;
  e.m_pGizmo = this;

  if (bCancel)
  {
    FocusLost(true);

    e.m_Type = plGizmoEvent::Type::CancelInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::None)
  {
    m_ManipulateMode = ManipulateMode::DrawBase;
    m_vFirstCorner = m_vCurrentPosition;
    m_vSecondCorner = m_vFirstCorner;

    SetActiveInputContext(this);
    UpdateBox();

    e.m_Type = plGizmoEvent::Type::BeginInteractions;
    m_GizmoEvents.Broadcast(e);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_ManipulateMode = ManipulateMode::DrawHeight;
    m_iHeightChange = 0;
    m_fOriginalBoxHeight = m_fBoxHeight;
    m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
    UpdateBox();
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    e.m_Type = plGizmoEvent::Type::EndInteractions;
    m_GizmoEvents.Broadcast(e);

    UpdateBox();
    FocusLost(false);
    return;
  }
}

void plDrawBoxGizmo::UpdateBox()
{
  UpdateStatusBarText(GetOwnerWindow());

  if (m_ManipulateMode == ManipulateMode::DrawBase)
  {
    m_vSecondCorner = m_vCurrentPosition;
    m_vSecondCorner.x = plMath::Lerp(m_vSecondCorner.x, m_vFirstCorner.x, m_vUpAxis.x);
    m_vSecondCorner.y = plMath::Lerp(m_vSecondCorner.y, m_vFirstCorner.y, m_vUpAxis.y);
    m_vSecondCorner.z = plMath::Lerp(m_vSecondCorner.z, m_vFirstCorner.z, m_vUpAxis.z);
  }

  if (m_ManipulateMode == ManipulateMode::None || m_vFirstCorner == m_vSecondCorner)
  {
    m_hBox.SetTransformation(plTransform(plVec3(0), plQuat::IdentityQuaternion(), plVec3(0)));
    m_hBox.SetVisible(false);
    return;
  }

  if (m_ManipulateMode == ManipulateMode::DrawHeight)
  {
    m_fBoxHeight = m_fOriginalBoxHeight + ((float)m_iHeightChange * 0.1f * plSnapProvider::GetTranslationSnapValue());
    plVec3 snapDummy(m_fBoxHeight);
    plSnapProvider::SnapTranslation(snapDummy);
    m_fBoxHeight = m_vUpAxis.Dot(snapDummy);
  }

  plVec3 vCenter = plMath::Lerp(m_vFirstCorner, m_vSecondCorner, 0.5f);
  vCenter.x += m_fBoxHeight * 0.5f * m_vUpAxis.x;
  vCenter.y += m_fBoxHeight * 0.5f * m_vUpAxis.y;
  vCenter.z += m_fBoxHeight * 0.5f * m_vUpAxis.z;

  plVec3 vSize;

  if (m_vUpAxis.z != 0)
  {
    vSize.x = plMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.y = plMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.z = m_fBoxHeight;
  }
  else if (m_vUpAxis.x != 0)
  {
    vSize.z = plMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = plMath::Abs(m_vSecondCorner.y - m_vFirstCorner.y);
    vSize.x = m_fBoxHeight;
  }
  else if (m_vUpAxis.y != 0)
  {
    vSize.x = plMath::Abs(m_vSecondCorner.x - m_vFirstCorner.x);
    vSize.z = plMath::Abs(m_vSecondCorner.z - m_vFirstCorner.z);
    vSize.y = m_fBoxHeight;
  }

  m_hBox.SetTransformation(plTransform(vCenter, plQuat::IdentityQuaternion(), vSize));
  m_hBox.SetVisible(true);
}

void plDrawBoxGizmo::DisableGrid(bool bControlPressed)
{
  if (!bControlPressed)
  {
    m_bDisplayGrid = false;
  }
}

void plDrawBoxGizmo::UpdateGrid(QMouseEvent* e)
{
  m_bDisplayGrid = false;

  if (m_ManipulateMode == ManipulateMode::None && e->modifiers().testFlag(Qt::ControlModifier))
  {
    if (e != nullptr && PickPosition(e))
    {
      m_vFirstCorner = m_vCurrentPosition;
      m_bDisplayGrid = true;
    }
  }
}

void plDrawBoxGizmo::GetResult(plVec3& out_Origin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ, float& out_fSizePosZ) const
{
  out_Origin = m_vFirstCorner;

  float fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
  float fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
  float fBoxZ = m_fBoxHeight;

  if (m_vUpAxis.x != 0)
  {
    fBoxY = m_vSecondCorner.y - m_vFirstCorner.y;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxX = m_fBoxHeight;
  }

  if (m_vUpAxis.y != 0)
  {
    fBoxX = m_vSecondCorner.x - m_vFirstCorner.x;
    fBoxZ = m_vSecondCorner.z - m_vFirstCorner.z;
    fBoxY = m_fBoxHeight;
  }

  if (fBoxX > 0)
  {
    out_fSizeNegX = 0;
    out_fSizePosX = fBoxX;
  }
  else
  {
    out_fSizeNegX = -fBoxX;
    out_fSizePosX = 0;
  }

  if (fBoxY > 0)
  {
    out_fSizeNegY = 0;
    out_fSizePosY = fBoxY;
  }
  else
  {
    out_fSizeNegY = -fBoxY;
    out_fSizePosY = 0;
  }

  if (fBoxZ > 0)
  {
    out_fSizeNegZ = 0;
    out_fSizePosZ = fBoxZ;
  }
  else
  {
    out_fSizeNegZ = -fBoxZ;
    out_fSizePosZ = 0;
  }
}

void plDrawBoxGizmo::UpdateStatusBarText(plQtEngineDocumentWindow* pWindow)
{
  switch (m_ManipulateMode)
  {
    case ManipulateMode::None:
    {
      pWindow->SetPermanentStatusBarMsg("Greyboxing: Hold CTRL and click-drag to draw a box. Hold SHIFT to reuse the previous plane height.");
      break;
    }

    case ManipulateMode::DrawBase:
    {
      plVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x = plMath::Abs(diff.x);
      diff.y = plMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(plFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Release the mouse to finish the base. ESC to cancel.", plArgF(diff.y, 2, false, 2), plArgF(diff.x, 2, false, 2), plArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }

    case ManipulateMode::DrawHeight:
    {
      plVec3 diff = m_vSecondCorner - m_vFirstCorner;
      diff.x = plMath::Abs(diff.x);
      diff.y = plMath::Abs(diff.y);

      pWindow->SetPermanentStatusBarMsg(plFmt("Greyboxing: [Width: {}, Depth: {}, Height: {}] Draw up/down to specify the box height. Click to finish, ESC to cancel.", plArgF(diff.y, 2, false, 2), plArgF(diff.x, 2, false, 2), plArgF(m_fBoxHeight, 2, false, 2)));
      break;
    }
  }
}
