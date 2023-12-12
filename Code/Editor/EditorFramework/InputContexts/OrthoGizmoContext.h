#pragma once

#include <EditorFramework/Gizmos/GizmoBase.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>
#include <QPoint>

class QWidget;
class plCamera;

class PLASMA_EDITORFRAMEWORK_DLL plOrthoGizmoContext : public PlasmaEditorInputContext
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plOrthoGizmoContext, PlasmaEditorInputContext);

public:
  plOrthoGizmoContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera);

  void SetWindowConfig(const plVec2I32& viewport) { m_vViewport = viewport; }

  virtual void FocusLost(bool bCancel);

  plEvent<const plGizmoEvent&> m_GizmoEvents;

  const plVec3& GetTranslationResult() const { return m_vTranslationResult; }
  const plVec3& GetTranslationDiff() const { return m_vTranslationDiff; }
  const plQuat& GetRotationResult() const { return m_qRotationResult; }
  float GetScalingResult() const { return m_fScalingResult; }

protected:
  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override {}

private:
  bool IsViewInOrthoMode() const;

  plVec2I32 m_vLastMousePos;
  plVec3 m_vUnsnappedTranslationResult;
  plVec3 m_vTranslationResult;
  plVec3 m_vTranslationDiff;
  plAngle m_UnsnappedRotationResult;
  plQuat m_qRotationResult;
  float m_fScaleMouseMove;
  float m_fScalingResult;
  float m_fUnsnappedScalingResult;
  bool m_bCanInteract;
  const plCamera* m_pCamera;
  plVec2I32 m_vViewport;
};
