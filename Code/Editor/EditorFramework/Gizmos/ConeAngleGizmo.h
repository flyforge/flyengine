#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plConeAngleGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plConeAngleGizmo, plGizmo);

public:
  plConeAngleGizmo();

  void SetAngle(plAngle angle);
  plAngle GetAngle() const { return m_Angle; }

  void SetRadius(float fRadius) { m_fRadius = fRadius; }

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

  plEngineGizmoHandle m_hConeAngle;

  enum class ManipulateMode
  {
    None,
    Angle,
  };

  ManipulateMode m_ManipulateMode;

  plAngle m_Angle;
  float m_fRadius;
  float m_fAngleScale;
};
