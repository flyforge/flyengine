#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeLengthGizmo.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plConeLengthGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plConeLengthGizmo::plConeLengthGizmo()
{
  m_fRadius = 1.0f;
  m_fRadiusScale = 0.1f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeRadius.ConfigureHandle(this, plEngineGizmoHandleType::Cone, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable | plGizmoFlags::OnTop); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plConeLengthGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeRadius);
}

void plConeLengthGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeRadius.SetVisible(bVisible);
}

void plConeLengthGizmo::OnTransformationChanged(const plTransform& transform)
{
  plTransform t = transform;
  t.m_vScale *= plVec3(1.0f, m_fRadiusScale, m_fRadiusScale) * m_fRadius;

  m_hConeRadius.SetTransformation(t);
}

void plConeLengthGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeRadius.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

plEditorInput plConeLengthGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
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

plEditorInput plConeLengthGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plConeLengthGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::MakeFromSeconds(1.0 / 25.0))
    return plEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const plVec2I32 vNewMousePos = plVec2I32(mousePosition.x(), mousePosition.y());
  const plVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;
  const plAngle aSpeed = plAngle::MakeFromDegree(1.0f);

  if (m_ManipulateMode == ManipulateMode::Radius)
  {
    m_fRadius += vDiff.x * fSpeed;
    m_fRadius -= vDiff.y * fSpeed;

    m_fRadius = plMath::Max(0.0f, m_fRadius);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

void plConeLengthGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
