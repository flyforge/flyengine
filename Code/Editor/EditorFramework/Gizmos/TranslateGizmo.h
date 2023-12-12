#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plTranslateGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTranslateGizmo, plGizmo);

public:
  plTranslateGizmo();

  const plVec3 GetStartPosition() const { return m_vStartPosition; }
  const plVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const plVec3 GetTranslationDiff() const { return m_vLastMoveDiff; }

  enum class MovementMode
  {
    ScreenProjection,
    MouseDiff
  };

  enum class PlaneInteraction
  {
    PlaneX,
    PlaneY,
    PlaneZ
  };

  enum class TranslateMode
  {
    None,
    Axis,
    Plane
  };

  void SetMovementMode(MovementMode mode);
  PlaneInteraction GetLastPlaneInteraction() const { return m_LastPlaneInteraction; }
  TranslateMode GetTranslateMode() const { return m_Mode; }

  /// \brief Used when CTRL+drag moves the object AND the camera
  void SetCameraSpeed(float fSpeed);

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

  plResult GetPointOnAxis(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const;
  plResult GetPointOnPlane(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const;

private:
  plVec2I32 m_vLastMousePos;
  plVec2 m_vTotalMouseDiff;

  plVec3 m_vLastMoveDiff;

  MovementMode m_MovementMode;
  PlasmaEngineGizmoHandle m_hAxisX;
  PlasmaEngineGizmoHandle m_hAxisY;
  PlasmaEngineGizmoHandle m_hAxisZ;

  PlasmaEngineGizmoHandle m_hPlaneXY;
  PlasmaEngineGizmoHandle m_hPlaneXZ;
  PlasmaEngineGizmoHandle m_hPlaneYZ;

  TranslateMode m_Mode;
  PlaneInteraction m_LastPlaneInteraction;

  float m_fStartScale;
  float m_fCameraSpeed;

  plTime m_LastInteraction;
  plVec3 m_vMoveAxis;
  plVec3 m_vPlaneAxis[2];
  plVec3 m_vStartPosition;
  plMat4 m_mInvViewProj;

  bool m_bUseExperimentalGizmo = false;
};
