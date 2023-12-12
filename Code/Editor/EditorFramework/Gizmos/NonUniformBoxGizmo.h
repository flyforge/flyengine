#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plNonUniformBoxGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plNonUniformBoxGizmo, plGizmo);

public:
  plNonUniformBoxGizmo();

  void SetSize(const plVec3& negSize, const plVec3& posSize, bool bLinkAxis = false);

  const plVec3& GetNegSize() const { return m_vNegSize; }
  const plVec3& GetPosSize() const { return m_vPosSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

private:
  plResult GetPointOnAxis(plInt32 iScreenPosX, plInt32 iScreenPosY, plVec3& out_Result) const;

  plTime m_LastInteraction;
  plMat4 m_mInvViewProj;

  plVec2I32 m_vLastMousePos;

  PlasmaEngineGizmoHandle m_hOutline;
  PlasmaEngineGizmoHandle m_Nobs[6];
  plVec3 m_vMainAxis[6];

  enum ManipulateMode
  {
    None = -1,
    DragNegX,
    DragPosX,
    DragNegY,
    DragPosY,
    DragNegZ,
    DragPosZ,
  };

  ManipulateMode m_ManipulateMode = ManipulateMode::None;

  plVec3 m_vNegSize;
  plVec3 m_vPosSize;
  plVec3 m_vStartNegSize;
  plVec3 m_vStartPosSize;
  plVec3 m_vMoveAxis;
  plVec3 m_vStartPosition;
  plVec3 m_vInteractionPivot;
  float m_fStartScale = 1.0f;
  bool m_bLinkAxis = false;
};
