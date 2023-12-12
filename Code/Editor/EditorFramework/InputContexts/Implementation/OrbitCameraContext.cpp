#include <EditorFramework/EditorFrameworkPCH.h>

#include <Core/Graphics/Camera.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/InputContexts/OrbitCameraContext.h>

plOrbitCameraContext::plOrbitCameraContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView)
{
  m_Volume.SetCenterAndHalfExtents(plVec3::ZeroVector(), plVec3::ZeroVector());
  m_pCamera = nullptr;

  m_LastUpdate = plTime::Now();

  // while the camera moves, ignore all other shortcuts
  SetShortcutsDisabled(true);

  SetOwner(pOwnerWindow, pOwnerView);
}

void plOrbitCameraContext::SetCamera(plCamera* pCamera)
{
  m_pCamera = pCamera;
}

plCamera* plOrbitCameraContext::GetCamera() const
{
  return m_pCamera;
}


void plOrbitCameraContext::SetDefaultCameraRelative(const plVec3& vDirection, float fDistanceScale)
{
  m_bFixedDefaultCamera = false;

  m_vDefaultCamera = vDirection;
  m_vDefaultCamera.NormalizeIfNotZero(plVec3::UnitXAxis()).IgnoreResult();
  m_vDefaultCamera *= plMath::Max(0.01f, fDistanceScale);
}

void plOrbitCameraContext::SetDefaultCameraFixed(const plVec3& vPosition)
{
  m_bFixedDefaultCamera = true;
  m_vDefaultCamera = vPosition;
}

void plOrbitCameraContext::MoveCameraToDefaultPosition()
{
  if (!m_pCamera)
    return;

  const plVec3 vCenterPos = m_Volume.GetCenter();
  plVec3 vCamPos = m_vDefaultCamera;

  if (!m_bFixedDefaultCamera)
  {
    const plVec3 ext = m_Volume.GetHalfExtents();

    vCamPos = vCenterPos + m_vDefaultCamera * plMath::Max(0.1f, plMath::Max(ext.x, ext.y, ext.z));
  }

  m_pCamera->LookAt(vCamPos, vCenterPos, plVec3(0, 0, 1));
}

void plOrbitCameraContext::SetOrbitVolume(const plVec3& vCenterPos, const plVec3& vHalfBoxSize)
{
  bool bSetCamLookAt = false;

  if (m_Volume.GetHalfExtents().IsZero() && !vHalfBoxSize.IsZero())
  {
    bSetCamLookAt = true;
  }

  m_Volume.SetCenterAndHalfExtents(vCenterPos, vHalfBoxSize);

  if (bSetCamLookAt)
  {
    MoveCameraToDefaultPosition();
  }
}

void plOrbitCameraContext::DoFocusLost(bool bCancel)
{
  m_Mode = Mode::Off;

  m_bRun = false;
  m_bMoveForwards = false;
  m_bMoveBackwards = false;
  m_bMoveLeft = false;
  m_bMoveRight = false;
  m_bMoveUp = false;
  m_bMoveDown = false;

  ResetCursor();
}

PlasmaEditorInput plOrbitCameraContext::DoMousePressEvent(QMouseEvent* e)
{
  if (m_pCamera == nullptr)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
    {
      m_Mode = Mode::Orbit;
      goto activate;
    }

    if (e->button() == Qt::MouseButton::RightButton)
    {
      m_Mode = Mode::Free;
      goto activate;
    }
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Pan;

    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Pan;

    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;

activate:
{
  m_vLastMousePos = SetMouseMode(PlasmaEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  MakeActiveInputContext();
  return PlasmaEditorInput::WasExclusivelyHandled;
}
}

PlasmaEditorInput plOrbitCameraContext::DoMouseReleaseEvent(QMouseEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return PlasmaEditorInput::MayBeHandledByOthers;


  if (m_Mode == Mode::Orbit)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Free)
  {
    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  if (m_Mode == Mode::Pan)
  {
    if (e->button() == Qt::MouseButton::LeftButton)
      m_Mode = Mode::Free;

    if (e->button() == Qt::MouseButton::RightButton)
      m_Mode = Mode::Off;
  }

  // just to be save
  if (e->buttons() == Qt::NoButton || m_Mode == Mode::Off)
  {
    m_Mode = Mode::Off;
    m_bRun = false;
    m_bMoveForwards = false;
    m_bMoveBackwards = false;
    m_bMoveLeft = false;
    m_bMoveRight = false;
    m_bMoveUp = false;
    m_bMoveDown = false;
    ResetCursor();
  }

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plOrbitCameraContext::DoMouseMoveEvent(QMouseEvent* e)
{
  // do nothing, unless this is an active context
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (!m_pCamera->IsPerspective())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_Mode == Mode::Off)
    return PlasmaEditorInput::MayBeHandledByOthers;

  const plVec2I32 CurMousePos(QCursor::pos().x(), QCursor::pos().y());
  const plVec2I32 diff = CurMousePos - m_vLastMousePos;
  m_vLastMousePos = UpdateMouseMode(e);

  SetCurrentMouseMode();

  const float fMouseMoveSensitivity = 0.002f;

  const plVec3 vHalfExtents = m_Volume.GetHalfExtents();
  const float fMaxExtent = plMath::Max(vHalfExtents.x, vHalfExtents.y, vHalfExtents.z);
  const float fBoost = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 5.0f : 1.0f;
  const float fSensitivity = fBoost * 0.0001f * fMaxExtent;

  if (m_Mode == Mode::Orbit)
  {
    const float fMoveRight = diff.x * fMouseMoveSensitivity;
    const float fMoveUp = -diff.y * fMouseMoveSensitivity;

    const plVec3 vOrbitPoint = m_Volume.GetCenter();

    const float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

    if (fDistance > 0.01f)
    {
      // first force the camera to rotate towards the orbit point
      // this way the camera position doesn't jump around
      m_pCamera->LookAt(m_pCamera->GetCenterPosition(), vOrbitPoint, plVec3(0.0f, 0.0f, 1.0f));
    }

    // then rotate the camera, and adjust its position to again point at the orbit point

    m_pCamera->RotateLocally(plAngle::Radian(0.0f), plAngle::Radian(fMoveUp), plAngle::Radian(0.0f));
    m_pCamera->RotateGlobally(plAngle::Radian(0.0f), plAngle::Radian(0.0f), plAngle::Radian(fMoveRight));

    plVec3 vDir = m_pCamera->GetDirForwards();
    if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
    {
      vDir.Set(1.0f, 0, 0);
    }

    m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, plVec3(0.0f, 0.0f, 1.0f));
  }

  if (m_Mode == Mode::Free)
  {
    const float fAspectRatio = (float)GetOwnerView()->size().width() / (float)GetOwnerView()->size().height();
    const plAngle fFovX = m_pCamera->GetFovX(fAspectRatio);
    const plAngle fFovY = m_pCamera->GetFovY(fAspectRatio);

    float fRotateBoost = 1.0f;

    const float fMouseScale = 4.0f;
    const float fMouseRotateSensitivityX = (fFovX.GetRadian() / (float)GetOwnerView()->size().width()) * fRotateBoost * fMouseScale;
    const float fMouseRotateSensitivityY = (fFovY.GetRadian() / (float)GetOwnerView()->size().height()) * fRotateBoost * fMouseScale;

    float fRotateHorizontal = diff.x * fMouseRotateSensitivityX;
    float fRotateVertical = -diff.y * fMouseRotateSensitivityY;

    m_pCamera->RotateLocally(plAngle::Radian(0), plAngle::Radian(fRotateVertical), plAngle::Radian(0));
    m_pCamera->RotateGlobally(plAngle::Radian(0), plAngle::Radian(0), plAngle::Radian(fRotateHorizontal));
  }

  if (m_Mode == Mode::Pan)
  {
    const float fSpeedFactor = GetCameraSpeed();

    const float fMoveUp = -diff.y * fMouseMoveSensitivity * fSpeedFactor;
    const float fMoveRight = diff.x * fMouseMoveSensitivity * fSpeedFactor;

    m_pCamera->MoveLocally(0, fMoveRight, fMoveUp);
  }

  return PlasmaEditorInput::WasExclusivelyHandled;
}

PlasmaEditorInput plOrbitCameraContext::DoWheelEvent(QWheelEvent* e)
{
  if (m_Mode != Mode::Off)
    return PlasmaEditorInput::WasExclusivelyHandled; // ignore it, but others should not handle it either

  if (!m_pCamera->IsPerspective())
    return PlasmaEditorInput::MayBeHandledByOthers;

  const float fScale = e->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier) ? 1.4f : 1.1f;

  const plVec3 vOrbitPoint = m_Volume.GetCenter();

  float fDistance = (vOrbitPoint - m_pCamera->GetCenterPosition()).GetLength();

#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
  if (e->angleDelta().y() > 0)
#else
  if (e->delta() > 0)
#endif
  {
    fDistance /= fScale;
  }
  else
  {
    fDistance *= fScale;
  }

  plVec3 vDir = m_pCamera->GetDirForwards();
  if (fDistance == 0.0f || vDir.SetLength(fDistance).Failed())
  {
    vDir.Set(1.0f, 0, 0);
  }

  m_pCamera->LookAt(vOrbitPoint - vDir, vOrbitPoint, plVec3(0.0f, 0.0f, 1.0f));

  // handled, independent of whether we are the active context or not
  return PlasmaEditorInput::WasExclusivelyHandled;
}


PlasmaEditorInput plOrbitCameraContext::DoKeyPressEvent(QKeyEvent* e)
{
  if (e->key() == Qt::Key_F)
  {
    MoveCameraToDefaultPosition();
    return PlasmaEditorInput::WasExclusivelyHandled;
  }

  if (m_Mode != Mode::Free)
    return PlasmaEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = true;
      return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

PlasmaEditorInput plOrbitCameraContext::DoKeyReleaseEvent(QKeyEvent* e)
{
  if (!IsActiveInputContext())
    return PlasmaEditorInput::MayBeHandledByOthers;

  if (m_pCamera == nullptr)
    return PlasmaEditorInput::MayBeHandledByOthers;

  m_bRun = (e->modifiers() & Qt::KeyboardModifier::ShiftModifier) != 0;

  switch (e->key())
  {
    case Qt::Key_W:
      m_bMoveForwards = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_S:
      m_bMoveBackwards = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_A:
      m_bMoveLeft = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_D:
      m_bMoveRight = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_Q:
      m_bMoveDown = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
    case Qt::Key_E:
      m_bMoveUp = false;
      return PlasmaEditorInput::WasExclusivelyHandled;
  }

  return PlasmaEditorInput::MayBeHandledByOthers;
}

void plOrbitCameraContext::UpdateContext()
{
  plTime diff = plTime::Now() - m_LastUpdate;
  m_LastUpdate = plTime::Now();

  const double TimeDiff = plMath::Min(diff.GetSeconds(), 0.1);

  float fSpeedFactor = TimeDiff;

  if (m_bRun)
    fSpeedFactor *= 5.0f;

  const float fRotateHorizontal = 45 * fSpeedFactor;
  const float fRotateVertical = 45 * fSpeedFactor;

  fSpeedFactor *= GetCameraSpeed();

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
}

float plOrbitCameraContext::GetCameraSpeed() const
{
  const plVec3 ext = m_Volume.GetHalfExtents();
  float fSize = plMath::Max(0.1f, ext.x, ext.y, ext.z);

  return fSize;
}

void plOrbitCameraContext::ResetCursor()
{
  if (m_Mode == Mode::Off)
  {
    SetMouseMode(PlasmaEditorInputContext::MouseMode::Normal);
    MakeActiveInputContext(false);
  }
}

void plOrbitCameraContext::SetCurrentMouseMode()
{
  if (m_Mode != Mode::Off)
  {
    SetMouseMode(PlasmaEditorInputContext::MouseMode::HideAndWrapAtScreenBorders);
  }
  else
  {
    SetMouseMode(PlasmaEditorInputContext::MouseMode::Normal);
  }
}
