#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorFramework/Preferences/ScenePreferences.h>
#include <Foundation/Utilities/GraphicsUtils.h>

static const float s_fMoveSpeed[25] = {
  0.5f,
  0.75f,
  1.0f,
  1.5f,
  2.0f,

  3.0f,
  4.0f,
  6.0f,
  8.0f,
  12.0f,

  16.0f,
  24.0f,
  32.0f,
  48.0f,
  64.0f,

  96.0f,
  128.0f,
  192.0f,
  256.0f,
  384.0f,

  512.0f,
  768.0f,
  1024.0f,
  1536.0f,
  2048.0f,
};

plCameraMoveContext::plCameraMoveContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  m_vOrbitPoint.SetZero();
  m_pCamera = nullptr;

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;
  m_bOpenMenuOnMouseUp = false;

  m_LastUpdate = plTime::Now();

  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera = false;
  m_bSlideForwards = false;
  m_bPanOrbitPoint = false;

  m_bRotateLeft = false;
  m_bRotateRight = false;
  m_bRotateUp = false;
  m_bRotateDown = false;

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

float plCameraMoveContext::ConvertCameraSpeed(plUInt32 uiSpeedIdx)
{
  return s_fMoveSpeed[plMath::Clamp<plUInt32>(uiSpeedIdx, 0, PLASMA_ARRAY_SIZE(s_fMoveSpeed) - 1)];
}

void plCameraMoveContext::DoFocusLost(bool bCancel)
{
  m_bRotateCamera = false;
  m_bMoveCamera = false;
  m_bMoveCameraInPlane = false;
  m_bOrbitCamera = false;
  m_bSlideForwards = false;
  m_bOpenMenuOnMouseUp = false;
  m_bPanOrbitPoint = false;

  ResetCursor();

  m_bRun = false;
  m_bSlowDown = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveRight = false;
  m_bMoveLeft = false;
  m_bMoveUp = false;
  m_bMoveDown = false;
  m_bMoveForwardsInPlane = false;
  m_bMoveBackwardsInPlane = false;
  m_bRotateLeft = false;
  m_bRotateRight = false;
  m_bRotateUp = false;
  m_bRotateDown = false;
}

void plCameraMoveContext::LoadState()
{
  const plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  SetMoveSpeed(pPreferences->GetCameraSpeed());
}

void plCameraMoveContext::UpdateContext()
{
  plTime diff = plTime::Now() - m_LastUpdate;
  m_LastUpdate = plTime::Now();

  const double TimeDiff = plMath::Min(diff.GetSeconds(), 0.1);

  plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetOwnerWindow()->GetDocument());
  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;
  if (m_bSlowDown)
    fSpeedFactor *= 0.2f;

  const float fRotateHorizontal = 45 * fSpeedFactor;
  const float fRotateVertical = 45 * fSpeedFactor;

  fSpeedFactor *= ConvertCameraSpeed(pPreferences->GetCameraSpeed());

  if (m_bMoveForwards)
    m_pCamera->MoveLocally(fSpeedFactor, 0, 0);
  if (m_bMoveBackwards)
    m_pCamera->MoveLocally(-fSpeedFactor, 0, 0);
  if (m_bMoveRight)
    m_pCamera->MoveLocally(0, fSpeedFactor, 0);
  if (m_bMoveLeft)
    m_pCamera->MoveLocally(0, -fSpeedFactor, 0);
  if (m_bMoveUp)
    m_pCamera->MoveGlobally(0, 0, 1 * fSpeedFactor);
  if (m_bMoveDown)
    m_pCamera->MoveGlobally(0, 0, -1 * fSpeedFactor);
  if (m_bRotateLeft)
    m_pCamera->RotateGlobally(plAngle::MakeFromRadian(0), plAngle::MakeFromRadian(0), plAngle::MakeFromDegree(-fRotateHorizontal));
  if (m_bRotateRight)
    m_pCamera->RotateGlobally(plAngle::MakeFromRadian(0), plAngle::MakeFromRadian(0), plAngle::MakeFromDegree(fRotateHorizontal));
  if (m_bRotateUp)
    m_pCamera->RotateLocally(plAngle::MakeFromRadian(0), plAngle::MakeFromDegree(fRotateVertical), plAngle::MakeFromRadian(0));
  if (m_bRotateDown)
    m_pCamera->RotateLocally(plAngle::MakeFromRadian(0), plAngle::MakeFromDegree(-fRotateVertical), plAngle::MakeFromRadian(0));

  if (m_bMoveForwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      plVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * fSpeedFactor, vDir.y * fSpeedFactor, vDir.z * fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, fSpeedFactor);
    }
  }

  if (m_bMoveBackwardsInPlane)
  {
    if (m_pCamera->IsPerspective())
    {
      plVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();
      m_pCamera->MoveGlobally(vDir.x * -fSpeedFactor, vDir.y * -fSpeedFactor, vDir.z * -fSpeedFactor);
    }
    else
    {
      m_pCamera->MoveLocally(0, 0, -fSpeedFactor);
    }
  }
}

void plCameraMoveContext::DeactivateIfLast()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards || m_bPanOrbitPoint || m_bMoveForwards || m_bMoveBackwards || m_bMoveRight || m_bMoveLeft || m_bMoveUp || m_bMoveDown || m_bMoveForwardsInPlane || m_bMoveBackwardsInPlane || m_bRotateLeft || m_bRotateRight || m_bRotateUp || m_bRotateDown)
    return;

  FocusLost(false);
}

plEditorInput plCameraMoveContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return plEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;
  m_bSlowDown = false;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = false;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Left:
      m_bMoveLeft = false;
      m_bRotateLeft = false;
      DeactivateIfLast();
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      m_bMoveRight = false;
      m_bRotateRight = false;
      DeactivateIfLast();
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      m_bMoveForwards = false;
      m_bRotateUp = false;
      DeactivateIfLast();
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      m_bMoveBackwards = false;
      m_bRotateDown = false;
      DeactivateIfLast();
      return plEditorInput::WasExclusivelyHandled;
  }

  return plEditorInput::MayBeHandledByOthers;
}

plEditorInput plCameraMoveContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (m_pCamera == nullptr)
    return plEditorInput::MayBeHandledByOthers;

  //if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
  //  return plEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_Left:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateLeft = true;
      else
        m_bMoveLeft = true;
      SetActiveInputContext(this);
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Right:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateRight = true;
      else
        m_bMoveRight = true;
      SetActiveInputContext(this);
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Up:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateUp = true;
      else
        m_bMoveForwards = true;
      SetActiveInputContext(this);
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Down:
      if (e->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
        m_bRotateDown = true;
      else
        m_bMoveBackwards = true;
      SetActiveInputContext(this);
      return plEditorInput::WasExclusivelyHandled;
  }

  if (!m_bRotateCamera)
    return plEditorInput::MayBeHandledByOthers;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = true;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = true;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = true;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = true;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = true;
      return plEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = true;
      return plEditorInput::WasExclusivelyHandled;
  }

  return plEditorInput::MayBeHandledByOthers;
}

plEditorInput plCameraMoveContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);
      m_bMoveCamera = true;
      m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[1] = false;
      MakeActiveInputContext();
      return plEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bSlideForwards = false;
      m_bRotateCamera = false;
      m_bOpenMenuOnMouseUp = (e->buttons() == Qt::MouseButton::RightButton);

      m_fSlideForwardsDistance = (m_vOrbitPoint - m_pCamera->GetPosition()).GetLength();

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bSlideForwards = true;
      else
        m_bRotateCamera = true;

      m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[1] = false;
      MakeActiveInputContext();
      return plEditorInput::WasExclusivelyHandled;
    }
    else
    {
      m_bOpenMenuOnMouseUp = false;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bOrbitCamera = false;
      m_bMoveCamera = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
        m_bOrbitCamera = true;
      else
        m_bMoveCamera = true;

      m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[0] = false;
      MakeActiveInputContext();
      return plEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint = false;

      if ((e->modifiers() & Qt::KeyboardModifier::AltModifier) != 0)
      {
        m_bPanOrbitPoint = true;
      }
      else
        m_bMoveCameraInPlane = true;

      m_vLastMousePos = SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
      m_bDidMoveMouse[2] = false;
      MakeActiveInputContext();
      return plEditorInput::WasExclusivelyHandled;
    }
  }

  return plEditorInput::MayBeHandledByOthers;
}

void plCameraMoveContext::ResetCursor()
{
  if (!m_bRotateCamera && !m_bMoveCamera && !m_bMoveCameraInPlane && !m_bOrbitCamera && !m_bSlideForwards)
  {
    SetMouseMode(plEditorInputContext::MouseMode::Normal);

    MakeActiveInputContext(false);
  }
}

void plCameraMoveContext::SetCurrentMouseMode()
{
  if (m_bRotateCamera || m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bSlideForwards)
  {
    SetMouseMode(plEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(plEditorInputContext::MouseMode::Normal);
  }
}

plEditorInput plCameraMoveContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return plEditorInput::MayBeHandledByOthers;

  if (m_pCamera->IsOrthographic())
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bMoveCamera = false;

      ResetCursor();

      if (!m_bDidMoveMouse[1] && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }
      return plEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_bRotateCamera = false;
      m_bSlideForwards = false;

      m_bMoveForwards = false;
      m_bMoveBackwards = false;
      m_bMoveLeft = false;
      m_bMoveRight = false;
      m_bMoveUp = false;
      m_bMoveDown = false;
      m_bRotateLeft = false;
      m_bRotateRight = false;
      m_bRotateUp = false;
      m_bRotateDown = false;

      ResetCursor();

      if (!m_bDidMoveMouse[1] && m_bOpenMenuOnMouseUp)
      {
        GetOwnerView()->OpenContextMenu(e->globalPosition().toPoint());
      }

      return plEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_bMoveCamera = false;
      m_bOrbitCamera = false;
      ResetCursor();

      if (!m_bDidMoveMouse[0])
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return plEditorInput::MayBeHandledByOthers;
      }

      return plEditorInput::WasExclusivelyHandled;
    }

    if (e->button() == Qt::MouseButton::MiddleButton)
    {
      m_bRotateCamera = false;
      m_bMoveCamera = false;
      m_bMoveCameraInPlane = false;
      m_bPanOrbitPoint = false;

      ResetCursor();

      if (!m_bDidMoveMouse[2])
      {
        // not really handled, so make this context inactive and tell the surrounding code that it may pass
        // the event to the next handler
        return plEditorInput::MayBeHandledByOthers;
      }

      return plEditorInput::WasExclusivelyHandled;
    }
  }

  return plEditorInput::MayBeHandledByOthers;
}

const plVec3& plCameraMoveContext::GetOrbitPoint() const
{
  return m_vOrbitPoint;
}

void plCameraMoveContext::SetOrbitPoint(const plVec3& vPos)
{
  m_vOrbitPoint = vPos;
}

plEditorInput plCameraMoveContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return plEditorInput::MayBeHandledByOthers;

  // store that the mouse has been moved since the last click
  for (plInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_bDidMoveMouse); ++i)
    m_bDidMoveMouse[i] = true;

  // send a message to clear any highlight
  plViewHighlightMsgToEngine msg;
  GetOwnerWindow()->GetEditorEngineConnection()->SendHighlightObjectMessage(&msg);

  if (m_pCamera == nullptr)
    return plEditorInput::MayBeHandledByOthers;

  const plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  float fBoost = 1.0f;
  float fRotateBoost = 1.0f;

  if (m_bRun)
    fBoost = 5.0f;
  if (m_bSlowDown)
  {
    fBoost = 0.1f;
    fRotateBoost = 0.2f;
  }

  const plVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const plVec2I32 diff = CurMousePos - m_vLastMousePos;

  if (m_pCamera->IsOrthographic())
  {
    float fDistPerPixel = 0;

    if (m_pCamera->GetCameraMode() == plCameraMode::OrthoFixedHeight)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().height();

    if (m_pCamera->GetCameraMode() == plCameraMode::OrthoFixedWidth)
      fDistPerPixel = m_pCamera->GetFovOrDim() / (float)GetOwnerView()->size().width();

    if (m_bMoveCamera)
    {
      m_vLastMousePos = UpdateMouseMode(e);

      float fMoveUp = diff.y * fDistPerPixel;
      float fMoveRight = -diff.x * fDistPerPixel;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      return plEditorInput::WasExclusivelyHandled;
    }
  }
  else
  {
    SetCurrentMouseMode();

    // correct the up vector, if it got messed up
    m_pCamera->LookAt(m_pCamera->GetCenterPosition(), m_pCamera->GetCenterPosition() + m_pCamera->GetCenterDirForwards(), plVec3(0, 0, 1));

    const float fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const plAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const plAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    const float fMouseScale = 4.0f;

    const float fMouseMoveSensitivity = 0.002f * ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fRotateBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fRotateBoost * fMouseScale;

    if (m_bRotateCamera && m_bMoveCamera) // left & right mouse button -> pan
    {
      float fMoveUp = -diff.y * fMouseMoveSensitivity;
      float fMoveRight = diff.x * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);

      m_vLastMousePos = UpdateMouseMode(e);
      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_bRotateCamera || m_bOrbitCamera)
    {
      float fDistToOrbit = 0.0f;

      if (m_bOrbitCamera)
      {
        fDistToOrbit = plMath::Max(0.01f, (m_vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength());
      }

      float fRotateHorizontal = diff.x * fMouseRotateSensitivityX;
      float fRotateVertical = -diff.y * fMouseRotateSensitivityY;

      m_pCamera->RotateLocally(plAngle::MakeFromRadian(0), plAngle::MakeFromRadian(fRotateVertical), plAngle::MakeFromRadian(0));
      m_pCamera->RotateGlobally(plAngle::MakeFromRadian(0), plAngle::MakeFromRadian(0), plAngle::MakeFromRadian(fRotateHorizontal));

      if (m_bOrbitCamera)
      {
        const plVec3 vDirection = m_pCamera->GetDirForwards();
        const plVec3 vNewCamPos = m_vOrbitPoint - vDirection * fDistToOrbit;

        m_pCamera->LookAt(vNewCamPos, m_vOrbitPoint, m_pCamera->GetDirUp());
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCamera)
    {
      float fMoveRight = diff.x * fMouseMoveSensitivity;
      float fMoveForward = -diff.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(fMoveForward, fMoveRight, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_bMoveCameraInPlane)
    {
      float fMoveRight = diff.x * fMouseMoveSensitivity;
      float fMoveForward = -diff.y * fMouseMoveSensitivity;

      m_pCamera->MoveLocally(0, fMoveRight, 0);

      plVec3 vDir = m_pCamera->GetCenterDirForwards();
      vDir.z = 0.0f;
      vDir.NormalizeIfNotZero(plVec3::MakeZero()).IgnoreResult();

      m_vOrbitPoint += vDir * fMoveForward;
      m_pCamera->MoveGlobally(vDir.x * fMoveForward, vDir.y * fMoveForward, vDir.z * fMoveForward);

      m_vLastMousePos = UpdateMouseMode(e);

      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_bSlideForwards)
    {
      float fMove = diff.y * fMouseMoveSensitivity * m_fSlideForwardsDistance * 0.1f;

      m_pCamera->MoveLocally(fMove, 0, 0);

      m_vLastMousePos = UpdateMouseMode(e);

      return plEditorInput::WasExclusivelyHandled;
    }

    if (m_bPanOrbitPoint)
    {
      plMat4 viewMatrix, projectionMatrix;
      GetOwnerView()->GetCameraMatrices(viewMatrix, projectionMatrix);

      plMat4 mvp = projectionMatrix * viewMatrix;

      plVec3 vScreenPos(0);
      if (plGraphicsUtils::ConvertWorldPosToScreenPos(mvp, 0, 0, GetOwnerView()->width(), GetOwnerView()->height(), m_vOrbitPoint, vScreenPos).Succeeded())
      {
        plMat4 invMvp = mvp.GetInverse();

        vScreenPos.x -= diff.x;
        vScreenPos.y += diff.y;

        plVec3 vNewPoint(0);
        if (plGraphicsUtils::ConvertScreenPosToWorldPos(invMvp, 0, 0, GetOwnerView()->width(), GetOwnerView()->height(), vScreenPos, vNewPoint).Succeeded())
        {
          const plVec3 vDiff = vNewPoint - m_vOrbitPoint;

          m_vOrbitPoint = vNewPoint;
          m_pCamera->MoveGlobally(vDiff.x, vDiff.y, vDiff.z);
        }
      }

      m_vLastMousePos = UpdateMouseMode(e);
      return plEditorInput::WasExclusivelyHandled;
    }
  }

  return plEditorInput::MayBeHandledByOthers;
}

void plCameraMoveContext::SetMoveSpeed(plInt32 iSpeed)
{
  if (GetOwnerWindow()->GetDocument() != nullptr)
  {
    plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetOwnerWindow()->GetDocument());
    pPreferences->SetCameraSpeed(iSpeed);
  }
}

plEditorInput plCameraMoveContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_bMoveCamera || m_bMoveCameraInPlane || m_bOrbitCamera || m_bRotateCamera)
    return plEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  const plScenePreferencesUser* pPreferences = plPreferences::QueryPreferences<plScenePreferencesUser>(GetOwnerWindow()->GetDocument());

  if (m_pCamera->IsOrthographic())
  {
    float fBoost = 1.0f;
    const float fTick = 1.4f;

    float fNewDim = 20.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    if (e->angleDelta().y() > 0)
#else
    if (e->delta() > 0)
#endif
      fNewDim = m_pCamera->GetFovOrDim() * plMath::Pow(1.0f / fTick, fBoost);
    else
      fNewDim = m_pCamera->GetFovOrDim() * plMath::Pow(fTick, fBoost);

    fNewDim = plMath::Clamp(fNewDim, 1.0f, 2000.0f);

    m_pCamera->SetCameraMode(m_pCamera->GetCameraMode(), fNewDim, m_pCamera->GetNearPlane(), m_pCamera->GetFarPlane());

    // handled, independent of whether we are the active context or not
    return plEditorInput::WasExclusivelyHandled;
  }
  else
  {
    if (e->modifiers() == Qt::KeyboardModifier::ControlModifier)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() + 1);
      }
      else
      {
        SetMoveSpeed(pPreferences->GetCameraSpeed() - 1);
      }

      // handled, independent of whether we are the active context or not
      return plEditorInput::WasExclusivelyHandled;
    }

    {
      float fBoost = 0.25f;

      if (e->modifiers() == Qt::KeyboardModifier::ShiftModifier)
        fBoost *= 5.0f;

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
      if (e->angleDelta().y() > 0)
#else
      if (e->delta() > 0)
#endif
      {
        m_pCamera->MoveLocally(ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }
      else
      {
        m_pCamera->MoveLocally(-ConvertCameraSpeed(pPreferences->GetCameraSpeed()) * fBoost, 0, 0);
      }

      // handled, independent of whether we are the active context or not
      return plEditorInput::WasExclusivelyHandled;
    }
  }
}

void plCameraMoveContext::SetCamera(plCamera* pCamera)
{
  if (m_pCamera == pCamera)
    return;

  m_pCamera = pCamera;
}
