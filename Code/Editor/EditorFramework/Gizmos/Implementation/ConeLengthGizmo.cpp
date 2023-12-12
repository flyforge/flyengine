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

  m_hConeRadius.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Cone, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable | plGizmoFlags::OnTop); // this gizmo should be rendered very last so it is always on top

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
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

PlasmaEditorInput plConeLengthGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeRadius)
  {
    m_ManipulateMode = ManipulateMode::Radius;
  }
  else
    return PlasmaEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = plTime::Now();

  m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plConeLengthGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plConeLengthGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::Seconds(1.0 / 25.0))
    return PlasmaEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const plVec2I32 vNewMousePos = plVec2I32(e->globalPos().x(), e->globalPos().y());
  const plVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vLastMousePos = UpdateMouseMode(e);

  const float fSpeed = 0.02f;
  const plAngle aSpeed = plAngle::Degree(1.0f);

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

  return PlasmaEditorInput::WasExclusivelyHandled;
}

void plConeLengthGizmo::SetRadius(float fRadius)
{
  m_fRadius = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
