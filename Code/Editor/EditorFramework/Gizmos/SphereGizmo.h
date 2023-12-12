#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plSphereGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plSphereGizmo, plGizmo);

public:
  plSphereGizmo();

  void SetInnerSphere(bool bEnabled, float fRadius = 0.0f);
  void SetOuterSphere(float fRadius);

  float GetInnerRadius() const { return m_fRadiusInner; }
  float GetOuterRadius() const { return m_fRadiusOuter; }

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

  PlasmaEngineGizmoHandle m_hInnerSphere;
  PlasmaEngineGizmoHandle m_hOuterSphere;

  enum class ManipulateMode
  {
    None,
    InnerSphere,
    OuterSphere
  };

  ManipulateMode m_ManipulateMode;
  bool m_bInnerEnabled;

  float m_fRadiusInner;
  float m_fRadiusOuter;
};
