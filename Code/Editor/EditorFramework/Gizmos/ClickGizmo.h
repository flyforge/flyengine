#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

/// \brief The click gizmo displays a simple shape that can be clicked.
///
/// This can be used to provide the user with a way to select which part to edit further.
class PLASMA_EDITORFRAMEWORK_DLL plClickGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plClickGizmo, plGizmo);

public:
  plClickGizmo();

  void SetColor(const plColor& color);

protected:
  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual void DoFocusLost(bool bCancel) override;
  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

private:
  plEngineGizmoHandle m_hShape;
};
