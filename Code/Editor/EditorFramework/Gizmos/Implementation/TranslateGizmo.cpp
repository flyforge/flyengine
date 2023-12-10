#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/Preferences/EditorPreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plTranslateGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plTranslateGizmo::plTranslateGizmo()
{
  m_vStartPosition.SetZero();
  m_fCameraSpeed = 0.2f;

  const plColor colr = plColorScheme::LightUI(plColorScheme::Red);
  const plColor colg = plColorScheme::LightUI(plColorScheme::Green);
  const plColor colb = plColorScheme::LightUI(plColorScheme::Blue);

  m_hAxisX.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colr, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowX.obj");
  m_hAxisY.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colg, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowY.obj");
  m_hAxisZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colb, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable, "Editor/Meshes/TranslateArrowZ.obj");

  m_hPlaneYZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colr, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable | plGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneX.obj");
  m_hPlaneXZ.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colg, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable | plGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneY.obj");
  m_hPlaneXY.ConfigureHandle(this, plEngineGizmoHandleType::FromFile, colb, plGizmoFlags::ConstantSize | plGizmoFlags::Pickable | plGizmoFlags::FaceCamera, "Editor/Meshes/TranslatePlaneZ.obj");

  SetVisible(false);
  SetTransformation(plTransform::MakeIdentity());

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
}

void plTranslateGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisX);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hAxisZ);

  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXY);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneXZ);
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hPlaneYZ);
}

void plTranslateGizmo::OnVisibleChanged(bool bVisible)
{
  m_hAxisX.SetVisible(bVisible);
  m_hAxisY.SetVisible(bVisible);
  m_hAxisZ.SetVisible(bVisible);

  m_hPlaneXY.SetVisible(bVisible);
  m_hPlaneXZ.SetVisible(bVisible);
  m_hPlaneYZ.SetVisible(bVisible);
}

void plTranslateGizmo::OnTransformationChanged(const plTransform& transform)
{
  m_hAxisX.SetTransformation(transform);
  m_hAxisY.SetTransformation(transform);
  m_hAxisZ.SetTransformation(transform);
  m_hPlaneXY.SetTransformation(transform);
  m_hPlaneYZ.SetTransformation(transform);
  m_hPlaneXZ.SetTransformation(transform);

  if (!IsActiveInputContext())
  {
    // if the gizmo is currently not being dragged, copy the translation into the start position
    m_vStartPosition = GetTransformation().m_vPosition;
  }
}

void plTranslateGizmo::DoFocusLost(bool bCancel)
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

  m_hPlaneXY.SetVisible(true);
  m_hPlaneXZ.SetVisible(true);
  m_hPlaneYZ.SetVisible(true);

  m_Mode = TranslateMode::None;
  m_MovementMode = MovementMode::ScreenProjection;
  m_vLastMoveDiff.SetZero();

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  GetOwnerWindow()->SetPermanentStatusBarMsg("");
}

plEditorInput plTranslateGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return plEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::MayBeHandledByOthers;

  m_vLastMoveDiff.SetZero();

  const plQuat gizmoRot = GetTransformation().m_qRotation;

  if (m_pInteractionGizmoHandle == &m_hAxisX)
  {
    m_vMoveAxis = gizmoRot * plVec3(1, 0, 0);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisY)
  {
    m_vMoveAxis = gizmoRot * plVec3(0, 1, 0);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_hAxisZ)
  {
    m_vMoveAxis = gizmoRot * plVec3(0, 0, 1);
    m_Mode = TranslateMode::Axis;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXY)
  {
    m_vMoveAxis = gizmoRot * plVec3(0, 0, 1);
    m_vPlaneAxis[0] = gizmoRot * plVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * plVec3(0, 1, 0);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneZ;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneXZ)
  {
    m_vMoveAxis = gizmoRot * plVec3(0, 1, 0);
    m_vPlaneAxis[0] = gizmoRot * plVec3(1, 0, 0);
    m_vPlaneAxis[1] = gizmoRot * plVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneY;
  }
  else if (m_pInteractionGizmoHandle == &m_hPlaneYZ)
  {
    m_vMoveAxis = gizmoRot * plVec3(1, 0, 0);
    m_vPlaneAxis[0] = gizmoRot * plVec3(0, 1, 0);
    m_vPlaneAxis[1] = gizmoRot * plVec3(0, 0, 1);
    m_Mode = TranslateMode::Plane;
    m_LastPlaneInteraction = PlaneInteraction::PlaneX;
  }
  else
    return plEditorInput::MayBeHandledByOthers;

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_vStartPosition = GetTransformation().m_vPosition;
  m_vTotalMouseDiff.SetZero();

  plMat4 mView = m_pCamera->GetViewMatrix();
  plMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  plMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();


  m_LastInteraction = plTime::Now();

  m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::WrapAtScreenBorders);
  SetActiveInputContext(this);

  if (m_Mode == TranslateMode::Axis)
  {
    GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }
  else if (m_Mode == TranslateMode::Plane)
  {
    GetPointOnPlane(e->pos().x(), m_vViewport.y - e->pos().y(), m_vInteractionPivot).IgnoreResult();
  }

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return plEditorInput::WasExclusivelyHandled;
}

plEditorInput plTranslateGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return plEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return plEditorInput::WasExclusivelyHandled;
}

plResult plTranslateGizmo::GetPointOnPlane(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  plVec3 vPos, vRayDir;
  if (plGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, plVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return PLASMA_FAILURE;

  plPlane Plane;
  Plane = plPlane::MakeFromNormalAndPoint(m_vMoveAxis, m_vStartPosition);

  plVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return PLASMA_FAILURE;

  out_Result = vIntersection;
  return PLASMA_SUCCESS;
}

plResult plTranslateGizmo::GetPointOnAxis(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  plVec3 vPos, vRayDir;
  if (plGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, plVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return PLASMA_FAILURE;

  const plVec3 vPlaneTangent = m_vMoveAxis.CrossRH(m_pCamera->GetDirForwards()).GetNormalized();
  const plVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

  plPlane Plane;
  Plane = plPlane::MakeFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  plVec3 vIntersection;
  if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
    return PLASMA_FAILURE;

  const plVec3 vDirAlongRay = vIntersection - m_vStartPosition;
  const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return PLASMA_SUCCESS;
}

plEditorInput plTranslateGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::MakeFromSeconds(1.0 / 25.0))
    return plEditorInput::WasExclusivelyHandled;

  const QPoint mousePosition = e->globalPosition().toPoint();

  const plVec2I32 CurMousePos(mousePosition.x(), mousePosition.y());

  m_LastInteraction = tNow;

  plTransform mTrans = GetTransformation();
  plVec3 vTranslate(0);

  if (m_MovementMode == MovementMode::ScreenProjection)
  {
    plVec3 vCurrentInteractionPoint;

    if (m_Mode == TranslateMode::Axis)
    {
      if (GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return plEditorInput::WasExclusivelyHandled;
      }
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      if (GetPointOnPlane(e->pos().x(), m_vViewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
      {
        m_vLastMousePos = UpdateMouseMode(e);
        return plEditorInput::WasExclusivelyHandled;
      }
    }


    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const plVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const plVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    vTranslate = vNewPos - m_vStartPosition;
  }
  else
  {
    const float fSpeed = m_fCameraSpeed * 0.01f;

    m_vTotalMouseDiff += plVec2((float)(CurMousePos.x - m_vLastMousePos.x), (float)(CurMousePos.y - m_vLastMousePos.y));
    const plVec3 vMouseDir = m_pCamera->GetDirRight() * m_vTotalMouseDiff.x + -m_pCamera->GetDirUp() * m_vTotalMouseDiff.y;

    if (m_Mode == TranslateMode::Axis)
    {
      vTranslate = m_vMoveAxis * (m_vMoveAxis.Dot(vMouseDir)) * fSpeed;
    }
    else if (m_Mode == TranslateMode::Plane)
    {
      vTranslate = m_vPlaneAxis[0] * (m_vPlaneAxis[0].Dot(vMouseDir)) * fSpeed + m_vPlaneAxis[1] * (m_vPlaneAxis[1].Dot(vMouseDir)) * fSpeed;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // disable snapping when ALT is pressed
  if (!e->modifiers().testFlag(Qt::AltModifier))
  {
    plSnapProvider::SnapTranslationInLocalSpace(mTrans.m_qRotation, vTranslate);
  }

  const plVec3 vLastPos = mTrans.m_vPosition;

  mTrans.m_vPosition = m_vStartPosition + vTranslate;

  m_vLastMoveDiff = mTrans.m_vPosition - vLastPos;

  SetTransformation(mTrans);

  // set statusbar message
  {
    const plVec3 diff = GetTransformation().m_qRotation.GetInverse() * GetTranslationResult();
    GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Translation: {}, {}, {}", plArgF(diff.x, 2), plArgF(diff.y, 2), plArgF(diff.z, 2)));
  }

  if (!m_vLastMoveDiff.IsZero())
  {
    plGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = plGizmoEvent::Type::Interaction;
    m_GizmoEvents.Broadcast(ev);
  }

  return plEditorInput::WasExclusivelyHandled;
}

void plTranslateGizmo::SetMovementMode(MovementMode mode)
{
  if (m_MovementMode == mode)
    return;

  m_MovementMode = mode;

  if (m_MovementMode == MovementMode::MouseDiff)
  {
    m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::WrapAtScreenBorders);
  }
}

void plTranslateGizmo::SetCameraSpeed(float fSpeed)
{
  m_fCameraSpeed = fSpeed;
}

void plTranslateGizmo::UpdateStatusBarText(plQtEngineDocumentWindow* pWindow)
{
  const plVec3 diff = plVec3::MakeZero();
  GetOwnerWindow()->SetPermanentStatusBarMsg(plFmt("Translation: {}, {}, {}", plArgF(diff.x, 2), plArgF(diff.y, 2), plArgF(diff.z, 2)));
}
