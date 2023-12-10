#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/Gizmos/GizmoBase.h>
#include <QPoint>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class PLASMA_EDITORFRAMEWORK_DLL plDrawBoxGizmo : public plGizmo
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plDrawBoxGizmo, plGizmo);

public:
  enum class ManipulateMode
  {
    None,
    DrawBase,
    DrawHeight,
  };

  plDrawBoxGizmo();
  ~plDrawBoxGizmo();

  void GetResult(plVec3& out_vOrigin, float& out_fSizeNegX, float& out_fSizePosX, float& out_fSizeNegY, float& out_fSizePosY, float& out_fSizeNegZ,
    float& out_fSizePosZ) const;

  ManipulateMode GetCurrentMode() const { return m_ManipulateMode; }
  const plVec3& GetStartPosition() const { return m_vFirstCorner; }

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) override;

  bool GetDisplayGrid() const { return m_bDisplayGrid; }

protected:
  virtual void DoFocusLost(bool bCancel) override;

  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual plEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual plEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override;
  virtual void OnVisibleChanged(bool bVisible) override;
  virtual void OnTransformationChanged(const plTransform& transform) override;

private:
  void SwitchMode(bool bCancel);
  void UpdateBox();
  void DisableGrid(bool bControlPressed);
  void UpdateGrid(QMouseEvent* e);
  bool PickPosition(QMouseEvent* e);

  ManipulateMode m_ManipulateMode;
  plEngineGizmoHandle m_hBox;

  plInt32 m_iHeightChange = 0;
  plVec2I32 m_vLastMousePos;
  plVec3 m_vCurrentPosition;
  plVec3 m_vFirstCorner;
  plVec3 m_vSecondCorner;
  plVec3 m_vUpAxis;
  plVec3 m_vLastStartPoint;
  float m_fBoxHeight = 0.5f;
  float m_fOriginalBoxHeight = 0.5f;
  bool m_bDisplayGrid = false;
};
