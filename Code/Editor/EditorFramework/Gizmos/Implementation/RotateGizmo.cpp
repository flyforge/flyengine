#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plRotateGizmo, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;

plRotateGizmo::plRotateGizmo()
{
  const plColor colr = plColorScheme::LightUI(plColorScheme::Red);
  const plColor colg = plColorScheme::LightUI(plColorScheme::Green);
  const plColor colb = plColorScheme::LightUI(plColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colr, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneX.obj");
  m_hAxisY.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colg, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneY.obj");
  m_hAxisZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colb, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/RotatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());
}

void plRotateGizmo::UpdateStatusBarText(plQtEngineDocumentWindow* pWindow)
{
  GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Rotation: {}", plAngle()));
}

void plRotateGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);
}

void plRotateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);
}

void plRotateGizmo::OnTransformationChanged(const plTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
}

void plRotateGizmo::DoFocusLost(bool bCancel)
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
}

plEditorInput plRotateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;

  const plQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vRotationAxis = gizmoRot * plVec3(1, 0, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vRotationAxis = gizmoRot * plVec3(0, 1, 0);
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vRotationAxis = gizmoRot * plVec3(0, 0, 1);
  }
  else
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_Rotation = plAngle();

  m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);

  m_qStartRotation = GetTransformation().m_qRotation;

  plMat4 mView = m_pCamera->GetViewMatrix();
  plMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  plMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();

  // compute screen space tangent for rotation
  {
    const plVec3 vAxisWS = m_vRotationAxis.GetNormalized();
    const plVec3 vMousePos(e->pos().x(), m_vViewport.y - e->pos().y(), 0);
    const plVec3 vGizmoPosWS = GetTransformation().m_vPosition;

    plVec3 vPosOnNearPlane, vRayDir;
    plGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vMousePos, vPosOnNearPlane, &vRayDir).IgnoreResult();

    plPlane plane;
    plane = plPlane::MakeFromNormalAndPoint(vAxisWS, vGizmoPosWS);

    plVec3 vPointOnGizmoWS;
    if (!plane.GetRayIntersection(vPosOnNearPlane, vRayDir, nullptr, &vPointOnGizmoWS))
    {
      // fallback at grazing angles, will result in fallback vDirWS during normalization
      vPointOnGizmoWS = vGizmoPosWS;
    }

    plVec3 vDirWS = vPointOnGizmoWS - vGizmoPosWS;
    vDirWS.NormalizeIfNotZero(plVec3(1, 0, 0)).IgnoreResult();

    plVec3 vTangentWS = vAxisWS.CrossRH(vDirWS);
    vTangentWS.Normalize();

    const plVec3 vTangentEndWS = vPointOnGizmoWS + vTangentWS;

    // compute the screen space position of the end point of the tangent vector, so that we can then compute the tangent in screen space
    plVec3 vTangentEndSS;
    plGraphicsUtils::ConvertWorldPosToScreenPos(mViewProj, 0, 0, m_vViewport.x, m_vViewport.y, vTangentEndWS, vTangentEndSS).IgnoreResult();
    vTangentEndSS.z = 0;

    const plVec3 vTangentSS = vTangentEndSS - vMousePos;
    m_vScreenTangent.Set(vTangentSS.x, vTangentSS.y);
    m_vScreenTangent.NormalizeIfNotZero(plVec2(1, 0)).IgnoreResult();

    // because window coordinates are flipped along Y
    m_vScreenTangent.y = -m_vScreenTangent.y;
  }

  m_LastInteraction = plTime::Now();

  SetActiveInputContext(this);

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plRotateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plRotateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::MakeFromSeconds(1.0 / 25.0))
    return plEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const plVec2 vNewMousePos = plVec2(mousePosition.x(), mousePosition.y());
  plVec2 vDiff = vNewMousePos - plVec2(m_vLastMousePos.x, m_vLastMousePos.y);

  m_vLastMousePos = UpdateMouseMode(e);

  const float dv = m_vScreenTangent.Dot(vDiff);
  m_Rotation += plAngle::MakeFromDegree(dv);

  plAngle rot = m_Rotation;

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
    plSnapProvider::SnapRotation(rot);

  m_qCurrentRotation = plQuat::MakeFromAxisAndAngle(m_vRotationAxis, rot);

  plTransform mTrans = GetTransformation();
  mTrans.m_qRotation = m_qCurrentRotation * m_qStartRotation;

  SetTransformation(mTrans);

  GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Rotation: {}", rot));

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}
