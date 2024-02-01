#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/CapsuleGizmo.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plCapsuleGizmo, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plCapsuleGizmo::plCapsuleGizmo()
{
  m_fLength = 1.0f;
  m_fRadius = 0.25f;

  m_ManipulateMode = ManipulateMode::None;

  m_hRadius.ConfigureHandle(this, plEngineGizmoHandleType::CylinderZ, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);
  m_hLengthTop.ConfigureHandle(this, plEngineGizmoHandleType::HalfSphereZ, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);
  m_hLengthBottom.ConfigureHandle(this, plEngineGizmoHandleType::HalfSphereZ, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plCapsuleGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthTop);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hLengthBottom);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hRadius);
}

void plCapsuleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hLengthTop.SetVisible(bVisible);
  m_hLengthBottom.SetVisible(bVisible);
  m_hRadius.SetVisible(bVisible);
}

void plCapsuleGizmo::OnTransformationChanged(const plTransform& transform)
{
  {
    plTransform mScaleCylinder;
    mScaleCylinder.SetIdentity();
    mScaleCylinder.m_vScale = plVec3(m_fRadius, m_fRadius, m_fLength);

    m_hRadius.SetTransformation(transform * mScaleCylinder);
  }

  {
    plTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, m_fLength * 0.5f);
    m_hLengthTop.SetTransformation(transform * mScaleSpheres);
  }

  {
    plTransform mScaleSpheres;
    mScaleSpheres.SetIdentity();
    mScaleSpheres.m_vScale.Set(m_fRadius, -m_fRadius, -m_fRadius);
    mScaleSpheres.m_vPosition.Set(0, 0, -m_fLength * 0.5f);
    m_hLengthBottom.SetTransformation(transform * mScaleSpheres);
  }
}

void plCapsuleGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hLengthTop.SetVisible(true);
  m_hLengthBottom.SetVisible(true);
  m_hRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

plEditorInput plCapsuleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else if (m_pInteractionGizmoHandle == &m_hLengthTop || m_pInteractionGizmoHandle == &m_hLengthBottom)
  {
    m_ManipulateMode = ManipulateMode::Length;
  }
  else
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = plTime::Now();

  m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plCapsuleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plCapsuleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::MakeFromSeconds(1.0 / 25.0))
    return plEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  QPoint mousePosition = e->globalPosition().toPoint();

  const plVec2I32 vNewMousePos = plVec2I32(mousePosition.x(), mousePosition.y());
  const plVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = plMath::Max(0.0f, m_fRadius);
  }
  else
  {
    m_fLength += vDiff.x * fSpeed;
    m_fLength -= vDiff.y * fSpeed;

    m_fLength = plMath::Max(0.0f, m_fLength);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

void plCapsuleGizmo::SetLength(float fRadius)
{
  m_fLength = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void plCapsuleGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
