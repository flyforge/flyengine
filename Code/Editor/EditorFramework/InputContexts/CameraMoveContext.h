#pragma once

#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <Foundation/Time/Time.h>
#include <QPoint>

class plCamera;

class PL_EDITORFRAMEWORK_DLL plCameraMoveContext : public plEditorInputContext
{
public:
  plCameraMoveContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView);

  void LoadState();

  void SetCamera(plCamera* pCamera);

  const plVec3& GetOrbitPoint() const;
  void SetOrbitPoint(const plVec3& vPos);

  static float ConvertCameraSpeed(plUInt32 uiSpeedIdx);

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual plEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual plEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;
  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual plEditorInput DoWheelEvent(QWheelEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override {}

private:
  virtual void UpdateContext() override;

  void SetMoveSpeed(plInt32 iSpeed);
  void ResetCursor();
  void SetCurrentMouseMode();
  void DeactivateIfLast();

  plVec3 m_vOrbitPoint;

  plVec2I32 m_vLastMousePos;

  bool m_bRotateCamera;
  bool m_bMoveCamera;
  bool m_bMoveCameraInPlane;
  bool m_bOrbitCamera;
  bool m_bSlideForwards;
  bool m_bPanOrbitPoint;
  float m_fSlideForwardsDistance;
  bool m_bOpenMenuOnMouseUp;

  plCamera* m_pCamera;

  bool m_bRun = false;
  bool m_bSlowDown = false;
  bool m_bMoveForwards = false;
  bool m_bMoveBackwards = false;
  bool m_bMoveRight = false;
  bool m_bMoveLeft = false;
  bool m_bMoveUp = false;
  bool m_bMoveDown = false;
  bool m_bMoveForwardsInPlane = false;
  bool m_bMoveBackwardsInPlane = false;
  bool m_bDidMoveMouse[3] = {false, false, false}; // Left Click, Right Click, Middle Click

  bool m_bRotateLeft = false;
  bool m_bRotateRight = false;
  bool m_bRotateUp = false;
  bool m_bRotateDown = false;

  plTime m_LastUpdate;
};
