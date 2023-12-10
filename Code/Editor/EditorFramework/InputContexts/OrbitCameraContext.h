#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class plCamera;

/// \brief A simple orbit camera. Use LMB to rotate, wheel to zoom, Alt to slow down.
class PLASMA_EDITORFRAMEWORK_DLL plOrbitCameraContext : public plEditorInputContext
{
public:
  plOrbitCameraContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView);

  void SetCamera(plCamera* pCamera);
  plCamera* GetCamera() const;

  void SetDefaultCameraRelative(const plVec3& vDirection, float fDistanceScale);
  void SetDefaultCameraFixed(const plVec3& vPosition);

  void MoveCameraToDefaultPosition();

  /// \brief Defines the box in which the user may move the camera around
  void SetOrbitVolume(const plVec3& vCenterPos, const plVec3& vHalfBoxSize);

  /// \brief The center point around which the camera can be moved and rotated.
  plVec3 GetVolumeCenter() const { return m_Volume.GetCenter(); }

  /// \brief The half-size of the volume in which the camera may move around
  plVec3 GetVolumeHalfSize() const { return m_Volume.GetHalfExtents(); }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual plEditorInput DoWheelEvent(QWheelEvent* e) override;
  virtual plEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual plEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  float GetCameraSpeed() const;

  void ResetCursor();
  void SetCurrentMouseMode();

  plVec2I32 m_vLastMousePos;

  enum class Mode
  {
    Off,
    Orbit,
    Free,
    Pan,
  };

  Mode m_Mode = Mode::Off;
  plCamera* m_pCamera;

  plBoundingBox m_Volume;

  bool m_bFixedDefaultCamera = true;
  plVec3 m_vDefaultCamera = plVec3(1, 0, 0);

  bool m_bRun = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;

  plTime m_LastUpdate;
};
