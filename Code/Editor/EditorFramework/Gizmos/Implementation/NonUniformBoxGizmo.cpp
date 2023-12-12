#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/Gizmos/NonUniformBoxGizmo.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <Foundation/Utilities/GraphicsUtils.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plNonUniformBoxGizmo, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plNonUniformBoxGizmo::plNonUniformBoxGizmo()
{
  m_vNegSize.Set(1.0f);
  m_vPosSize.Set(1.0f);

  m_ManipulateMode = ManipulateMode::None;

  m_hOutline.ConfigureHandle(this, PlasmaEngineGizmoHandleType::LineBox, plColorScheme::LightUI(plColorScheme::Gray), plGizmoFlags::ShowInOrtho);

  plColor cols[6] = {
    plColorScheme::LightUI(plColorScheme::Red),
    plColorScheme::LightUI(plColorScheme::Red),
    plColorScheme::LightUI(plColorScheme::Green),
    plColorScheme::LightUI(plColorScheme::Green),
    plColorScheme::LightUI(plColorScheme::Blue),
    plColorScheme::LightUI(plColorScheme::Blue),
  };

  for (plUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].ConfigureHandle(this, PlasmaEngineGizmoHandleType::Box, cols[i], plGizmoFlags::ConstantSize | plGizmoFlags::OnTop | plGizmoFlags::ShowInOrtho | plGizmoFlags::Pickable);
  }

  SetVisible(false);
  SetTransformation(plTransform::IdentityTransform());
}

void plNonUniformBoxGizmo::OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  pOwnerWindow->GetDocument()->AddSyncObject(&m_hOutline);

  for (plUInt32 i = 0; i < 6; ++i)
  {
    pOwnerWindow->GetDocument()->AddSyncObject(&m_Nobs[i]);
  }
}

void plNonUniformBoxGizmo::OnVisibleChanged(bool bVisible)
{
  m_hOutline.SetVisible(bVisible);

  for (plUInt32 i = 0; i < 6; ++i)
  {
    m_Nobs[i].SetVisible(bVisible);
  }
}

void plNonUniformBoxGizmo::OnTransformationChanged(const plTransform& transform)
{
  plMat4 scale, rot;
  scale.SetScalingMatrix(m_vNegSize + m_vPosSize);

  const plVec3 center = plMath::Lerp(-m_vNegSize, m_vPosSize, 0.5f);

  scale.SetTranslationVector(center);
  scale = transform.GetAsMat4() * scale;

  m_hOutline.SetTransformation(scale);

  for (plUInt32 i = 0; i < 6; ++i)
  {
    plVec3 pos = center;
    plVec3 dir;

    switch (i)
    {
      case ManipulateMode::DragNegX:
        pos.x = -m_vNegSize.x;
        dir.Set(-1, 0, 0);
        break;
      case ManipulateMode::DragPosX:
        pos.x = m_vPosSize.x;
        dir.Set(+1, 0, 0);
        break;
      case ManipulateMode::DragNegY:
        pos.y = -m_vNegSize.y;
        dir.Set(0, -1, 0);
        break;
      case ManipulateMode::DragPosY:
        pos.y = m_vPosSize.y;
        dir.Set(0, +1, 0);
        break;
      case ManipulateMode::DragNegZ:
        pos.z = -m_vNegSize.z;
        dir.Set(0, 0, -1);
        break;
      case ManipulateMode::DragPosZ:
        pos.z = m_vPosSize.z;
        dir.Set(0, 0, +1);
        break;
    }
    pos = pos.CompMul(transform.m_vScale);

    plTransform t;
    t.m_qRotation = transform.m_qRotation;
    t.m_vPosition = transform.m_vPosition + t.m_qRotation * pos;
    t.m_vScale.Set(0.15f);

    m_vMainAxis[i] = t.m_qRotation * dir;

    m_Nobs[i].SetTransformation(t);
  }
}

void plNonUniformBoxGizmo::DoFocusLost(bool bCancel)
{
  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;
  m_GizmoEvents.Broadcast(ev);

  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_ManipulateMode = ManipulateMode::None;
}

PlasmaEditorInput plNonUniformBoxGizmo::DoMousePressEvent(QMouseEvent* e)
{
  if (IsActiveInputContext())
    return PlasmaEditorInput::WasExclusivelyHandled;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::MayBeHandledByOthers;
  if (e->modifiers() != 0)
    return PlasmaEditorInput::MayBeHandledByOthers;

  for (plUInt32 i = 0; i < 6; ++i)
  {
    if (m_pInteractionGizmoHandle == &m_Nobs[i])
    {
      m_ManipulateMode = (ManipulateMode)i;
      m_vMoveAxis = m_vMainAxis[i];
      m_vStartPosition = m_pInteractionGizmoHandle->GetTransformation().m_vPosition;
      goto modify;
    }
  }

  return PlasmaEditorInput::MayBeHandledByOthers;

modify:

{
  plMat4 mView = m_pCamera->GetViewMatrix();
  plMat4 mProj;
  m_pCamera->GetProjectionMatrix((float)m_vViewport.x / (float)m_vViewport.y, mProj);
  plMat4 mViewProj = mProj * mView;
  m_mInvViewProj = mViewProj.GetInverse();
}

  m_vStartNegSize = m_vNegSize;
  m_vStartPosSize = m_vPosSize;

  GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), m_vInteractionPivot).IgnoreResult();

  plViewHighlightMsgToEngine msg;
  msg.m_HighlightObject = m_pInteractionGizmoHandle->GetGuid();
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  m_LastInteraction = plTime::Now();

  m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::Normal);

  SetActiveInputContext(this);

  m_fStartScale = (m_vInteractionPivot - m_pCamera->GetPosition()).GetLength() * 0.125;

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::BeginInteractions;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plNonUniformBoxGizmo::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() != Qt::MouseButton::LeftButton)
    return PlasmaEditorInput::WasExclusivelyHandled;

  FocusLost(false);

  SetActiveInputContext(nullptr);
  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plNonUniformBoxGizmo::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  const plTime tNow = plTime::Now();

  if (tNow - m_LastInteraction < plTime::Seconds(1.0 / 25.0))
    return PlasmaEditorInput::WasExclusivelyHandled;

  m_LastInteraction = tNow;

  const plVec2I32 vNewMousePos = plVec2I32(e->globalPos().x(), e->globalPos().y());
  const plVec2I32 vDiff = vNewMousePos - m_vLastMousePos;

  m_vNegSize = m_vStartNegSize;
  m_vPosSize = m_vStartPosSize;

  {
    plVec3 vCurrentInteractionPoint;

    if (GetPointOnAxis(e->pos().x(), m_vViewport.y - e->pos().y(), vCurrentInteractionPoint).Failed())
    {
      m_vLastMousePos = UpdateMouseMode(e);
      return PlasmaEditorInput::WasExclusivelyHandled;
    }

    const float fPerspectiveScale = (vCurrentInteractionPoint - m_pCamera->GetPosition()).GetLength() * 0.125;
    const plVec3 vOffset = (m_vInteractionPivot - m_vStartPosition);

    const plVec3 vNewPos = vCurrentInteractionPoint - vOffset * fPerspectiveScale / m_fStartScale;

    plVec3 vTranslate = -GetTransformation().m_qRotation * (vNewPos - m_vStartPosition);

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
    {
      plSnapProvider::SnapTranslation(vTranslate);
    }

    switch (m_ManipulateMode)
    {
      case None:
        break;
      case DragNegX:
        m_vNegSize.x -= vTranslate.x;
        if (m_bLinkAxis)
          m_vPosSize.x -= vTranslate.x;
        break;
      case DragPosX:
        m_vPosSize.x += vTranslate.x;
        if (m_bLinkAxis)
          m_vNegSize.x += vTranslate.x;
        break;
      case DragNegY:
        m_vNegSize.y -= vTranslate.y;
        if (m_bLinkAxis)
          m_vPosSize.y -= vTranslate.y;
        break;
      case DragPosY:
        m_vPosSize.y += vTranslate.y;
        if (m_bLinkAxis)
          m_vNegSize.y += vTranslate.y;
        break;
      case DragNegZ:
        m_vNegSize.z -= vTranslate.z;
        if (m_bLinkAxis)
          m_vPosSize.z -= vTranslate.z;
        break;
      case DragPosZ:
        m_vPosSize.z += vTranslate.z;
        if (m_bLinkAxis)
          m_vNegSize.z += vTranslate.z;
        break;
    }
  }

  m_vLastMousePos = UpdateMouseMode(e);

  // update the scale
  OnTransformationChanged(GetTransformation());

  plGizmoEvent ev;
  ev.m_pGizmo = this;
  ev.m_Type = plGizmoEvent::Type::Interaction;
  m_GizmoEvents.Broadcast(ev);

  return PlasmaEditorInput::WasExclusivelyHandled;
}

void plNonUniformBoxGizmo::SetSize(const plVec3& negSize, const plVec3& posSize, bool bLinkAxis)
{
  m_vNegSize = negSize;
  m_vPosSize = posSize;
  m_bLinkAxis = bLinkAxis;

  // update the scale
  OnTransformationChanged(GetTransformation());
}

plResult plNonUniformBoxGizmo::GetPointOnAxis(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const
{
  out_Result = m_vStartPosition;

  plVec3 vPos, vRayDir;
  if (plGraphicsUtils::ConvertScreenPosToWorldPos(m_mInvViewProj, 0, 0, m_vViewport.x, m_vViewport.y, plVec3(iScreenPosX, iScreenPosY, 0), vPos, &vRayDir).Failed())
    return PLASMA_FAILURE;

  const plVec3 vDir = m_pCamera->GetDirForwards();

  if (plMath::Abs(vDir.Dot(m_vMoveAxis)) > 0.999f)
    return PLASMA_FAILURE;

  const plVec3 vPlaneTangent = m_vMoveAxis.CrossRH(vDir).GetNormalized();
  const plVec3 vPlaneNormal = m_vMoveAxis.CrossRH(vPlaneTangent);

  plPlane Plane;
  Plane.SetFromNormalAndPoint(vPlaneNormal, m_vStartPosition);

  plVec3 vIntersection;
  if (m_pCamera->IsPerspective())
  {
    if (!Plane.GetRayIntersection(m_pCamera->GetPosition(), vRayDir, nullptr, &vIntersection))
      return PLASMA_FAILURE;
  }
  else
  {
    if (!Plane.GetRayIntersectionBiDirectional(vPos - vRayDir, vRayDir, nullptr, &vIntersection))
      return PLASMA_FAILURE;
  }

  const plVec3 vDirAlongRay = vIntersection - m_vStartPosition;
  const float fProjectedLength = vDirAlongRay.Dot(m_vMoveAxis);

  out_Result = m_vStartPosition + fProjectedLength * m_vMoveAxis;
  return PLASMA_SUCCESS;
}
