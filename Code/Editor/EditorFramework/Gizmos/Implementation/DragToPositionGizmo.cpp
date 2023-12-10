#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plDragToPositionGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plDragToPositionGizmo::plDragToPositionGizmo()
{
  m_bModifiesRotation = false;

  // TODO: adjust colors for +/- axis
  const plColor colr1 = plColorGammaUB(206, 0, 46);
  const plColor colr2 = plColorGammaUB(206, 0, 46);
  const plColor colg1 = plColorGammaUB(101, 206, 0);
  const plColor colg2 = plColorGammaUB(101, 206, 0);
  const plColor colb1 = plColorGammaUB(0, 125, 206);
  const plColor colb2 = plColorGammaUB(0, 125, 206);
  const plColor coly = plColorGammaUB(128, 128, 0);

  m_hBobble.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, coly, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragCenter.obj");
  m_hAlignPX.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colr1, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowPX.obj");
  m_hAlignNX.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colr2, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowNX.obj");
  m_hAlignPY.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colg1, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowPY.obj");
  m_hAlignNY.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colg2, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowNY.obj");
  m_hAlignPZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colb1, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowPZ.obj");
  m_hAlignNZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colb2, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/DragArrowNZ.obj");

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plDragToPositionGizmo::UpdateStatusBarText(plQtEngineDocumentWindow* pWindow)
{
  if (m_pInteractionGizmoHandle != nullptr)
  {
    if (m_pInteractionGizmoHandle == &m_hBobble)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: Center"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: +X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNX)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: -X"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: +Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNY)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: -Y"));
    else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: +Z"));
    else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
      GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position: -Z"));
  }
  else
  {
    GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Drag to Position"));
  }
}

void plDragToPositionGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hBobble);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignPZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAlignNZ);
}

void plDragToPositionGizmo::OnVisibleChanged(bool bVisible)
{
  m_hBobble.SetVisible(bVisible);
  m_hAlignPX.SetVisible(bVisible);
  m_hAlignNX.SetVisible(bVisible);
  m_hAlignPY.SetVisible(bVisible);
  m_hAlignNY.SetVisible(bVisible);
  m_hAlignPZ.SetVisible(bVisible);
  m_hAlignNZ.SetVisible(bVisible);
}

void plDragToPositionGizmo::OnTransformationChanged(const plTransform& transform)
{
  m_hBobble.SetTransformation(transform);
  m_hAlignPX.SetTransformation(transform);
  m_hAlignNX.SetTransformation(transform);
  m_hAlignPY.SetTransformation(transform);
  m_hAlignNY.SetTransformation(transform);
  m_hAlignPZ.SetTransformation(transform);
  m_hAlignNZ.SetTransformation(transform);
}

void plDragToPositionGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hBobble.SetVisible(true);
  m_hAlignPX.SetVisible(true);
  m_hAlignNX.SetVisible(true);
  m_hAlignPY.SetVisible(true);
  m_hAlignNY.SetVisible(true);
  m_hAlignPZ.SetVisible(true);
  m_hAlignNZ.SetVisible(true);

  m_pInteractionGizmoHandle = nullptr;
}

plEditorInput plDragToPositionGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // The gizmo is actually "hidden" somewhere else during dragging,
  // because it musn't be rendered into the picking buffer, to avoid picking against the gizmo
  // m_Bobble.SetVisible(false);
  // m_AlignPX.SetVisible(false);
  // m_AlignNX.SetVisible(false);
  // m_AlignPY.SetVisible(false);
  // m_AlignNY.SetVisible(false);
  // m_AlignPZ.SetVisible(false);
  // m_AlignNZ.SetVisible(false);
  // m_pInteractionGizmoHandle->SetVisible(true);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_qStartOrientation = GetTransformation().m_qRotation;

  m_LastInteraction = plTime::Now();

  SetActiveInputContext(this);

  UpdateStatusBarText(nullptr);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plDragToPositionGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plDragToPositionGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::MakeFromSeconds(1.0 / 25.0))
    return plEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const plObjectPickingResult& res = GetOwnerView()->PickObject(e->pos().x(), e->pos().y());

  if (!res.m_PickedObject.IsValid())
    return plEditorInput::WasExclusivelyHandled;

  if (res.m_vPickedPosition.IsNaN() || res.m_vPickedNormal.IsNaN() || res.m_vPickedNormal.IsZero())
    return plEditorInput::WasExclusivelyHandled;

  plVec3 vSnappedPosition = res.m_vPickedPosition;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    plSnapProvider::SnapTranslation(vSnappedPosition);

  plTransform mTrans = GetTransformation();
  mTrans.m_vPosition = vSnappedPosition;

  plQuat rot;
  plVec3 alignAxis, orthoAxis;

  if (m_pInteractionGizmoHandle == &m_hAlignPX)
  {
    alignAxis.Set(1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNX)
  {
    alignAxis.Set(-1, 0, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPY)
  {
    alignAxis.Set(0, 1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNY)
  {
    alignAxis.Set(0, -1, 0);
    orthoAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignPZ)
  {
    alignAxis.Set(0, 0, 1);
    orthoAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAlignNZ)
  {
    alignAxis.Set(0, 0, -1);
    orthoAxis.Set(1, 0, 0);
  }
  else
  {
    m_bModifiesRotation = false;
    rot.SetIdentity();
  }

  if (m_pInteractionGizmoHandle != &m_hBobble)
  {
    m_bModifiesRotation = true;

    alignAxis = m_qStartOrientation * alignAxis;
    alignAxis.Normalize();

    if (alignAxis.GetAngleBetween(res.m_vPickedNormal) > plAngle::MakeFromDegree(179))
    {
      rot = plQuat::MakeFromAxisAndAngle(m_qStartOrientation * orthoAxis, plAngle::MakeFromDegree(180));
    }
    else
    {
      rot = plQuat::MakeShortestRotation(alignAxis, res.m_vPickedNormal);
    }
  }

  mTrans.m_qRotation = rot * m_qStartOrientation;
  SetTransformation(mTrans);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}
