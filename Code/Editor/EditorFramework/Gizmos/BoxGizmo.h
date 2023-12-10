#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plBoxGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plBoxGizmo, plGizmo);

public:
  plBoxGizmo();

  void SetSize(const plVec3& vSize);

  const plVec3& GetSize() const { return m_vSize; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;


private:
  plTime m_LastInteraction;

  plVec2I32 m_vLastMousePos;

  plEngineGizmoHandle m_hCorners;
  plEngineGizmoHandle m_Edges[3];
  plEngineGizmoHandle m_Faces[3];

  enum class ManipulateMode
  {
    None,
    Uniform,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXY,
    PlaneXZ,
    PlaneYZ,
  };

  ManipulateMode m_ManipulateMode;

  plVec3 m_vSize;
};
