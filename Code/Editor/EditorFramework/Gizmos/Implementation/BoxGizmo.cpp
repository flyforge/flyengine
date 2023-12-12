#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/BoxGizmo.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoxGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plBoxGizmo::plBoxGizmo()
{
  m_vSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hCorners.ConfigureHandle(this, PlasmaEngineGizmoHandleType::BoxCorners, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].ConfigureHandle(this, PlasmaEngineGizmoHandleType::BoxEdges, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);
    m_Faces[i].ConfigureHandle(this, PlasmaEngineGizmoHandleType::BoxFaces, plColorLinearUB(200, 200, 200, 128), plGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
}

void plBoxGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hCorners);

  for (int i = 0; i < 3; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Edges[i]);
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Faces[i]);
  }
}

void plBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hCorners.SetVisible(bVisible);

  for (int i = 0; i < 3; ++i)
  {
    m_Edges[i].SetVisible(bVisible);
    m_Faces[i].SetVisible(bVisible);
  }
}

void plBoxGizmo::OnTransformationChanged(const plTransform& transform)
{
  plMat4 scale, rot;
  scale.SetScalingMatrix(m_vSize);
  scale = transform.GetAsMat4() * scale;

  m_hCorners.SetTransformation(scale);

  rot.SetRotationMatrixX(plAngle::Degree(90));
  m_Edges[0].SetTransformation(scale * rot);

  rot.SetRotationMatrixY(plAngle::Degree(90));
  m_Faces[0].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Edges[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixX(plAngle::Degree(90));
  m_Faces[1].SetTransformation(scale * rot);

  rot.SetRotationMatrixZ(plAngle::Degree(90));
  m_Edges[2].SetTransformation(scale * rot);

  rot.SetIdentity();
  m_Faces[2].SetTransformation(scale * rot);
}

void plBoxGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

PlasmaEditorInput plBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hCorners)
  {
    m_ManipulateMode = ManipulateMode::Uniform;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[0])
  {
    m_ManipulateMode = ManipulateMode::AxisX;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[1])
  {
    m_ManipulateMode = ManipulateMode::AxisY;
  }
  else if (m_pInteractionGizmoHandle == &m_Faces[2])
  {
    m_ManipulateMode = ManipulateMode::AxisZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[0])
  {
    m_ManipulateMode = ManipulateMode::PlaneXY;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[1])
  {
    m_ManipulateMode = ManipulateMode::PlaneXZ;
  }
  else if (m_pInteractionGizmoHandle == &m_Edges[2])
  {
    m_ManipulateMode = ManipulateMode::PlaneYZ;
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

PlasmaEditorInput plBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
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
  float fChange = 0.0f;

  {
    fChange += vDiff.x * fSpeed;
    fChange -= vDiff.y * fSpeed;
  }

  plVec3 vChange(0);

  if (m_ManipulateMode == ManipulateMode::Uniform)
    vChange.Set(fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneXY)
    vChange.Set(fChange, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::PlaneXZ)
    vChange.Set(fChange, 0, fChange);
  if (m_ManipulateMode == ManipulateMode::PlaneYZ)
    vChange.Set(0, fChange, fChange);
  if (m_ManipulateMode == ManipulateMode::AxisX)
    vChange.Set(fChange, 0, 0);
  if (m_ManipulateMode == ManipulateMode::AxisY)
    vChange.Set(0, fChange, 0);
  if (m_ManipulateMode == ManipulateMode::AxisZ)
    vChange.Set(0, 0, fChange);

  m_vSize += vChange;
  m_vSize.x = plMath::Max(m_vSize.x, 0.0f);
  m_vSize.y = plMath::Max(m_vSize.y, 0.0f);
  m_vSize.z = plMath::Max(m_vSize.z, 0.0f);

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

void plBoxGizmo::SetSize(const plVec3& size)
{
  m_vSize = size;

  // update the scale
  OnTransformationChanged(GetTransformation());
}
