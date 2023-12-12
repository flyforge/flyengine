#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class QWidget;
class plCamera;
struct plObjectPickingResult;
class plDocumentObject;

class PLASMA_EDITORFRAMEWORK_DLL plSelectionContext : public PlasmaEditorInputContext
{
public:
  plSelectionContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera);

  void SetWindowConfig(const plVec2I32& viewport) { m_vViewport = viewport; }

  void SetPickObjectOverride(plDelegate<void(const plDocumentObject*)> pickOverride);
  void ResetPickObjectOverride();

protected:
  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual PlasmaEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual PlasmaEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) override {}

  const plDocumentObject* determineObjectToSelect(const plDocumentObject* pickedObject, bool bToggle, bool bDirect) const;

  virtual void DoFocusLost(bool bCancel) override;

  virtual void OpenDocumentForPickedObject(const plObjectPickingResult& res) const;
  virtual void SelectPickedObject(const plObjectPickingResult& res, bool bToggle, bool bDirect) const;

protected:
  void SendMarqueeMsg(QMouseEvent* e, plUInt8 uiWhatToDo);

  plDelegate<void(const plDocumentObject*)> m_PickObjectOverride;
  const plCamera* m_pCamera;
  plVec2I32 m_vViewport;
  PlasmaEngineGizmoHandle m_hMarqueeGizmo;
  plVec3 m_vMarqueeStartPos;
  plUInt32 m_uiMarqueeID;
  bool m_bPressedSpace = false;

  enum class Mode
  {
    None,
    Single,
    MarqueeAdd,
    MarqueeRemove
  };

  Mode m_Mode = Mode::None;
};
