#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plScaleGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plScaleGizmo::plScaleGizmo()
{
  PlasmaEditorPreferencesUser* pPreferences = plPreferences::QueryPreferences<PlasmaEditorPreferencesUser>();
  m_bUseExperimentalGizmo = !pPreferences->m_bOldGizmos;

  if (m_bUseExperimentalGizmo)
  {
    const plColor colr = plColorScheme::LightUI(plColorScheme::Red);
    const plColor colg = plColorScheme::LightUI(plColorScheme::Green);
    const plColor colb = plColorScheme::LightUI(plColorScheme::Blue);
    const plColor coly = plColorScheme::LightUI(plColorScheme::Gray);

    m_hAxisX.ConfigureHandle(this, PlasmaEngineGizmoHandleType::FromFile, colr, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowX.obj");
    m_hAxisY.ConfigureHandle(this, PlasmaEngineGizmoHandleType::FromFile, colg, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowY.obj");
    m_hAxisZ.ConfigureHandle(this, PlasmaEngineGizmoHandleType::FromFile, colb, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/ScaleArrowZ.obj");
    m_hAxisXYZ.ConfigureHandle(this, PlasmaEngineGizmoHandleType::FromFile, coly, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/ScaleXYZ.obj");
  }
  else
  {
    m_hAxisX.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Piston, plColorLinearUB(128, 0, 0), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
    m_hAxisY.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Piston, plColorLinearUB(0, 128, 0), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
    m_hAxisZ.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Piston, plColorLinearUB(0, 0, 128), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
    m_hAxisXYZ.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Box, plColorLinearUB(128, 128, 0), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
}

void plScaleGizmo::UpdateStatusBarText(plQtEngineDocumentWindow* pWindow)
{
  const plVec3 scale(1.0f);
  GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Scale: {}, {}, {}", plArgF(scale.x, 2), plArgF(scale.y, 2), plArgF(scale.z, 2)));
}

void plScaleGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisXYZ);
}

void plScaleGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);
  m_hAxisXYZ.SetVisible(bVisible);
}

void plScaleGizmo::OnTransformationChanged(const plTransform& transform)
{
  if (m_bUseExperimentalGizmo)
  {
    m_hAxisX.SetTransformation(transform);
    m_hAxisY.SetTransformation(transform);
    m_hAxisZ.SetTransformation(transform);
    m_hAxisXYZ.SetTransformation(transform);
  }
  else
  {
    plTransform t;
    t.SetIdentity();

    t.m_vScale.Set(2.0f);
    m_hAxisX.SetTransformation(transform * t);

    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));
    m_hAxisY.SetTransformation(transform * t);

    t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(-90));
    m_hAxisZ.SetTransformation(transform * t);

    t.SetIdentity();
    t.m_vScale = plVec3(0.2f);
    m_hAxisXYZ.SetTransformation(transform * t);
  }
}

void plScaleGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_hAxisX.SetVisible(true);
  m_hAxisY.SetVisible(true);
  m_hAxisZ.SetVisible(true);
  m_hAxisXYZ.SetVisible(true);
}

PlasmaEditorInput plScaleGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis.Set(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis.Set(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis.Set(0, 0, 1);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisXYZ)
  {
    m_vMoveAxis.Set(1, 1, 1);
  }
  else
    return PlasmaEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_vScalingResult.Set(1.0f);
  m_vScaleMouseMove.SetZero();

  plMat4 mView = m_pCamera->GetViewMatrix();
  plMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  plMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

  m_LastInteraction = plTime::Now();

  SetActiveInputContext(this);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plScaleGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plScaleGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::Seconds(1.0 / 25.0))
    return PlasmaEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const plVec2I32 vNewMousePos = plVec2I32(e->globalPos().x(), e->globalPos().y());
  plVec2I32 vDiff = (vNewMousePos - m_vLastMousePos);

  m_vLastMousePos = UpdateMouseMode(e);

  m_vScaleMouseMove += m_vMoveAxis * (float)vDiff.x;
  m_vScaleMouseMove -= m_vMoveAxis * (float)vDiff.y;

  m_vScalingResult.Set(1.0f);

  const float fScaleSpeed = 0.01f;

  if (m_vScaleMouseMove.x > 0.0f)
    m_vScalingResult.x = 1.0f + m_vScaleMouseMove.x * fScaleSpeed;
  if (m_vScaleMouseMove.x < 0.0f)
    m_vScalingResult.x = 1.0f / (1.0f - m_vScaleMouseMove.x * fScaleSpeed);

  if (m_vScaleMouseMove.y > 0.0f)
    m_vScalingResult.y = 1.0f + m_vScaleMouseMove.y * fScaleSpeed;
  if (m_vScaleMouseMove.y < 0.0f)
    m_vScalingResult.y = 1.0f / (1.0f - m_vScaleMouseMove.y * fScaleSpeed);

  if (m_vScaleMouseMove.z > 0.0f)
    m_vScalingResult.z = 1.0f + m_vScaleMouseMove.z * fScaleSpeed;
  if (m_vScaleMouseMove.z < 0.0f)
    m_vScalingResult.z = 1.0f / (1.0f - m_vScaleMouseMove.z * fScaleSpeed);

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    plSnapProvider::SnapScale(m_vScalingResult);

  GetOwnerWindow()->SetPermanentStatusBarMsg(
    plFmt("Scale: {}, {}, {}", plArgF(m_vScalingResult.x, 2), plArgF(m_vScalingResult.y, 2), plArgF(m_vScalingResult.z, 2)));

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plManipulatorScaleGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plManipulatorScaleGizmo::plManipulatorScaleGizmo()
{
  // Overwrite axis to be boxes.
  m_hAxisX.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Box, plColorLinearUB(128, 0, 0), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
  m_hAxisY.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Box, plColorLinearUB(0, 128, 0), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
  m_hAxisZ.ConfigureHandle(this, PlasmaEngineGizmoHandleType::Box, plColorLinearUB(0, 0, 128), plGizmoFlags::ConstantSize | plGizmoFlags::Pickable);
}

void plManipulatorScaleGizmo::OnTransformationChanged(const plTransform& transform)
{
  const float fOffset = 0.8f;
  plTransform t;
  t.SetIdentity();
  t.m_vPosition = plVec3(fOffset, 0, 0);
  t.m_vScale = plVec3(0.2f);

  m_hAxisX.SetTransformation(transform * t);

  t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 0, 1), plAngle::Degree(90));
  t.m_vPosition = plVec3(0, fOffset, 0);
  m_hAxisY.SetTransformation(transform * t);

  t.m_qRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(-90));
  t.m_vPosition = plVec3(0, 0, fOffset);
  m_hAxisZ.SetTransformation(transform * t);

  t.SetIdentity();
  t.m_vScale = plVec3(0.3f);
  m_hAxisXYZ.SetTransformation(transform * t);
}
