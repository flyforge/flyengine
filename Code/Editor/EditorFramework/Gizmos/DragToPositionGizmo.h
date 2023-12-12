#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Math/Quat.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plDragToPositionGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDragToPositionGizmo, plGizmo);

public:
  plDragToPositionGizmo();

  const plVec3 GetTranslationResult() const { return GetTransformation().m_vPosition - m_vStartPosition; }
  const plQuat GetRotationResult() const { return GetTransformation().m_qRotation; }

  virtual bool IsPickingSelectedAllowed() const override { return false; }

  /// \brief Returns true if any of the 'align with' handles is selected, and thus the rotation of the dragged object should be modified as well
  bool ModifiesRotation() const { return m_bModifiesRotation; }

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) override;

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

  PlasmaEngineGizmoHandle m_hBobble;
  PlasmaEngineGizmoHandle m_hAlignPX;
  PlasmaEngineGizmoHandle m_hAlignNX;
  PlasmaEngineGizmoHandle m_hAlignPY;
  PlasmaEngineGizmoHandle m_hAlignNY;
  PlasmaEngineGizmoHandle m_hAlignPZ;
  PlasmaEngineGizmoHandle m_hAlignNZ;

  bool m_bUseExperimentalGizmo = false;
  bool m_bModifiesRotation;
  plTime m_LastInteraction;
  plVec3 m_vStartPosition;
  plQuat m_qStartOrientation;
};
