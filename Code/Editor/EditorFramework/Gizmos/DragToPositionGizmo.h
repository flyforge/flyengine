#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <Foundation/Math/Quat.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PL_EDITORFRAMEWORK_DLL plDragToPositionGizmo : public plGizmo
{
  PL_ADD_DYNAMIC_REFLECTION(plDragToPositionGizmo, plGizmo);

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

  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

  plEngineGizmoHandle m_hBobble;
  plEngineGizmoHandle m_hAlignPX;
  plEngineGizmoHandle m_hAlignNX;
  plEngineGizmoHandle m_hAlignPY;
  plEngineGizmoHandle m_hAlignNY;
  plEngineGizmoHandle m_hAlignPZ;
  plEngineGizmoHandle m_hAlignNZ;

  bool m_bModifiesRotation;
  plTime m_LastInteraction;
  plVec3 m_vStartPosition;
  plQuat m_qStartOrientation;
};
