#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/Gizmos/SnapProvider.h>
#include <EditorFramework/InputContexts/OrthoGizmoContext.h>

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plOrthoGizmoContext, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

plOrthoGizmoContext::plOrthoGizmoContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera)
{
  m_pCamera = pCamera;
  m_bCanInteract = false;

  SetOwner(pOwnerWindow, pOwnerView);
}


void plOrthoGizmoContext::FocusLost(bool bCancel)
{
  plGizmoEvent e;
  e.m_pGizmo = this;
  e.m_Type = bCancel ? plGizmoEvent::Type::CancelInteractions : plGizmoEvent::Type::EndInteractions;

  m_GizmoEvents.Broadcast(e);


  m_bCanInteract = false;
  SetActiveInputContext(nullptr);

  PlasmaEditorInputContext::FocusLost(bCancel);
}

PlasmaEditorInput plOrthoGizmoContext::DoMousePressEvent(QMouseEvent* e)
{
  if (!IsViewInOrthoMode())
    return PlasmaEditorInput::MayBeHandledByOthers;
  if (GetOwnerWindow()->GetDocument()->GetSelectionManager()->IsSelectionEmpty())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    m_bCanInteract = true;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plOrthoGizmoContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
  {
    m_bCanInteract = false;
    return PlasmaEditorInput::MayBeHandledByOthers;
  }

  if (e->button() == Qt::MouseButton::LeftButton)
  {
    FocusLost(false);
    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plOrthoGizmoContext::DoMouseMoveEvent(QMouseEvent* e)
{
  if (!e->buttons().testFlag(Qt::MouseButton::LeftButton))
  {
    m_bCanInteract = false;
    return PlasmaEditorInput::MayBeHandledByOthers;
  }

  if (IsActiveInputContext())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == plCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == plCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    const plVec3 vLastTranslationResult = m_vTranslationResult;

    const plVec2I32 diff = plVec2I32(e->globalX(), e->globalY()) - m_vLastMousePos;

    m_vUnsnappedTranslationResult += m_pCamera->GetDirRight() * (float)diff.x * fDistPerPixel;
    m_vUnsnappedTranslationResult -= m_pCamera->GetDirUp() * (float)diff.y * fDistPerPixel;

    m_vTranslationResult = m_vUnsnappedTranslationResult;

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
      plSnapProvider::SnapTranslation(m_vTranslationResult);

    m_vTranslationDiff = m_vTranslationResult - vLastTranslationResult;

    m_UnsnappedRotationResult += plAngle::Degree(-diff.x);

    plAngle snappedRotation = m_UnsnappedRotationResult;

    // disable snapping when ALT is pressed
    if (!e->modifiers().testFlag(Qt::AltModifier))
      plSnapProvider::SnapRotation(snappedRotation);

    m_qRotationResult.SetFromAxisAndAngle(m_pCamera->GetDirForwards(), snappedRotation);

    {
      m_fScaleMouseMove += diff.x;
      m_fUnsnappedScalingResult = 1.0f;

      const float fScaleSpeed = 0.01f;

      if (m_fScaleMouseMove > 0.0f)
        m_fUnsnappedScalingResult = 1.0f + m_fScaleMouseMove * fScaleSpeed;
      if (m_fScaleMouseMove < 0.0f)
        m_fUnsnappedScalingResult = 1.0f / (1.0f - m_fScaleMouseMove * fScaleSpeed);

      m_fScalingResult = m_fUnsnappedScalingResult;

      // disable snapping when ALT is pressed
      if (!e->modifiers().testFlag(Qt::AltModifier))
        plSnapProvider::SnapScale(m_fScalingResult);
    }

    m_vLastMousePos = UpdateMouseMode(e);

    plGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = plGizmoEvent::Type::Interaction;

    m_GizmoEvents.Broadcast(ev);

    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  if (m_bCanInteract)
  {
    m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::WrapAtScreenBorders);
    m_vTranslationResult.SetZero();
    m_vUnsnappedTranslationResult.SetZero();
    m_qRotationResult.SetIdentity();
    m_UnsnappedRotationResult = plAngle::Radian(0.0f);
    m_fScalingResult = 1.0f;
    m_fUnsnappedScalingResult = 1.0f;
    m_fScaleMouseMove = 0.0f;

    m_bCanInteract = false;
    SetActiveInputContext(this);

    plGizmoEvent ev;
    ev.m_pGizmo = this;
    ev.m_Type = plGizmoEvent::Type::BeginInteractions;

    m_GizmoEvents.Broadcast(ev);
    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

bool plOrthoGizmoContext::IsViewInOrthoMode() const
{
  return (GetOwnerView()->m_pViewConfig->m_Perspective != plSceneViewPerspective::Perspective);
}
