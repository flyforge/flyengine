#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plScaleGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plScaleGizmo, plGizmo);

public:
  plScaleGizmo();

  const plVec3& GetScalingResult() const { return m_vScalingResult; }

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

protected:
  PlasmaEngineGizmoHandle m_hAxisX;
  PlasmaEngineGizmoHandle m_hAxisY;
  PlasmaEngineGizmoHandle m_hAxisZ;
  PlasmaEngineGizmoHandle m_hAxisXYZ;

private:
  plVec3 m_vScalingResult;
  plVec3 m_vScaleMouseMove;

  plVec2I32 m_vLastMousePos;

  plTime m_LastInteraction;
  plVec3 m_vMoveAxis;
  plMat4 m_mInvViewProj;

  bool m_bUseExperimentalGizmo = false;
};

/// \brief Scale gizmo version that only uses boxes that can be composited with
/// rotate and translate gizmos without major overlap.
/// Used by the plTransformManipulatorAdapter.
class PLASMA_EDITORFRAMEWORK_DLL plManipulatorScaleGizmo : public plScaleGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plManipulatorScaleGizmo, plScaleGizmo);

public:
  plManipulatorScaleGizmo();

protected:
  virtual void OnTransformationChanged(const plTransform& transform) override;
};
