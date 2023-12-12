#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plCapsuleGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCapsuleGizmo, plGizmo);

public:
  plCapsuleGizmo();

  void SetLength(float fRadius);
  void SetRadius(float fLength);

  float GetLength() const { return m_fLength; }
  float GetRadius() const { return m_fRadius; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;


private:
  plTime m_LastInteraction;

  plVec2I32 m_vLastMousePos;

  PlasmaEngineGizmoHandle m_hLengthTop;
  PlasmaEngineGizmoHandle m_hLengthBottom;
  PlasmaEngineGizmoHandle m_hRadius;

  enum class ManipulateMode
  {
    None,
    Length,
    Radius,
  };

  ManipulateMode m_ManipulateMode;

  float m_fRadius;
  float m_fLength;
};
