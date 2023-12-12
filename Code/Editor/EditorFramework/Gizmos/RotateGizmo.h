#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plRotateGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plRotateGizmo, plGizmo);

public:
  plRotateGizmo();

  const plQuat& GetRotationResult() const { return m_qCurrentRotation; }

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

private:
  PlasmaEngineGizmoHandle m_hAxisX;
  PlasmaEngineGizmoHandle m_hAxisY;
  PlasmaEngineGizmoHandle m_hAxisZ;

  plQuat m_qStartRotation;
  plQuat m_qCurrentRotation;
  plAngle m_Rotation;

  plVec2I32 m_vLastMousePos;

  plTime m_LastInteraction;
  plVec3 m_vRotationAxis;
  plMat4 m_mInvViewProj;
  plVec2 m_vScreenTangent;

  bool m_bUseExperimentalGizmo = false;
};
