#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SphereGizmo.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSphereGizmo, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plSphereGizmo::plSphereGizmo()
{
  m_bInnerEnabled = false;

  m_fRadiusInner = 1.0f;
  m_fRadiusOuter = 2.0f;

  m_ManipulateMode = ManipulateMode::None;

  m_hInnerSphere.ConfigureHandle(this, plEngineGizmoHandleType::Sphere, plColorLinearUB(200, 200, 0, 128), plGizmoFlags::OnTop | plGizmoFlags::Pickable); // this gizmo should be rendered very last so it is always on top
  m_hOuterSphere.ConfigureHandle(this, plEngineGizmoHandleType::Sphere, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plSphereGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hInnerSphere);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOuterSphere);
}

void plSphereGizmo::OnVisibleChanged(bool bVisible)
{
  m_hInnerSphere.SetVisible(bVisible && m_bInnerEnabled);
  m_hOuterSphere.SetVisible(bVisible);
}

void plSphereGizmo::OnTransformationChanged(const plTransform& transform)
{
  plTransform mScaleInner, mScaleOuter;
  mScaleInner.SetIdentity();
  mScaleOuter.SetIdentity();
  mScaleInner.m_vScale = plVec3(m_fRadiusInner);
  mScaleOuter.m_vScale = plVec3(m_fRadiusOuter);

  m_hInnerSphere.SetTransformation(transform * mScaleInner);
  m_hOuterSphere.SetTransformation(transform * mScaleOuter);
}

void plSphereGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hInnerSphere.SetVisible(m_bInnerEnabled);
  m_hOuterSphere.SetVisible(true);

  m_ManipulateMode = ManipulateMode::None;
}

plEditorInput plSphereGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hInnerSphere)
  {
    m_ManipulateMode = ManipulateMode::InnerSphere;
  }
  else if (m_pInteractionGizmoHandle == &m_hOuterSphere)
  {
    m_ManipulateMode = ManipulateMode::OuterSphere;
  }
  else
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  // m_InnerSphere.SetVisible(false);
  // m_OuterSphere.SetVisible(false);

  // m_pInteractionGizmoHandle->SetVisible(true);

  m_LastInteraction = plTime::Now();

  m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  SetActiveInputContext(this);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plSphereGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plSphereGizmo::DoMouseMoveEvent(QMouseEvent* e)
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

  if (m_ManipulateMode == ManipulateMode::InnerSphere)
  {
    m_fRadiusInner += vDiff.x * fSpeed;
    m_fRadiusInner -= vDiff.y * fSpeed;

    m_fRadiusInner = plMath::Max(0.0f, m_fRadiusInner);

    m_fRadiusOuter = plMath::Max(m_fRadiusInner, m_fRadiusOuter);
  }
  else
  {
    m_fRadiusOuter += vDiff.x * fSpeed;
    m_fRadiusOuter -= vDiff.y * fSpeed;

    m_fRadiusOuter = plMath::Max(0.0f, m_fRadiusOuter);

    m_fRadiusInner = plMath::Min(m_fRadiusInner, m_fRadiusOuter);
  }

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

void plSphereGizmo::SetInnerSphere(bool bEnabled, float fRadius)
{
  m_fRadiusInner = fRadius;
  m_bInnerEnabled = bEnabled;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

void plSphereGizmo::SetOuterSphere(float fRadius)
{
  m_fRadiusOuter = fRadius;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
