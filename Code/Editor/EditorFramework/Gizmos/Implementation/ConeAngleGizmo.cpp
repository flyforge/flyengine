#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ConeAngleGizmo.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plConeAngleGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plConeAngleGizmo::plConeAngleGizmo()
{
  m_Angle = plAngle::Degree(1.0f);
  m_fAngleScale = 1.0f;
  m_fRadius = 1.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hConeAngle.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Cone, plColorLinearUB(200, 200, 0, 128), plGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
}

void plConeAngleGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hConeAngle);
}

void plConeAngleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hConeAngle.SetVisible(bVisible);
}

void plConeAngleGizmo::OnTransformationChanged(const plTransform& transform)
{
  plTransform t = transform;

  t.m_vScale *= plVec3(1.0f, m_fAngleScale, m_fAngleScale) * (plMath::Sin(m_Angle) * m_fRadius);
  m_hConeAngle.SetTransformation(t);
}

void plConeAngleGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hConeAngle.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

PlasmaEditorInput plConeAngleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hConeAngle)
  {
    m_ManipulateMode = ManipulateMode::Angle;
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

PlasmaEditorInput plConeAngleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plConeAngleGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  {
    m_Angle += vDiff.x * aSpeed;
    m_Angle -= vDiff.y * aSpeed;

    m_Angle = plMath::Clamp(m_Angle, plAngle(), plAngle::Degree(179.0f));

    m_fAngleScale = plMath::Tan(m_Angle * 0.5f);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

void plConeAngleGizmo::SetAngle(plAngle angle)
{
  m_Angle = angle;
  m_fAngleScale = plMath::Tan(m_Angle * 0.5f);

  // update the scale
  OnTransformationChanged(GetTransformation());
}
