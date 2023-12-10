#pragma once

#include <EditorEngineProcessFramework/Gizmos/GizmoHandle.h>
#include <EditorFramework/InputContexts/EditorInputContext.h>

class QWidget;
class plCamera;
struct plObjectPickingResult;
class plDocumentObject;

class PLASMA_EDITORFRAMEWORK_DLL plSelectionContext : public plEditorInputContext
{
public:
  plSelectionContext(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView, const plCamera* pCamera);
  ~plSelectionContext();

  void SetWindowConfig(const plVec2I32& vViewport) { m_vViewport = vViewport; }

  /// \brief Adds a delegate that gets called whenever an object is picked, as long as the override is active.
  ///
  /// It also changes the owner view's cursor to a cross-hair.
  /// If something gets picked, the override is called with a non-null object.
  /// In case the user presses ESC or the view gets destroyed while the override is active,
  /// the delegate is called with nullptr.
  /// This indicates that all picking should be stopped and the registered user should clean up.
  void SetPickObjectOverride(plDelegate<void(const plDocumentObject*)> pickOverride);
  void ResetPickObjectOverride();

protected:
  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) override;
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) override;

  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) override;
  virtual plEditorInput DoKeyPressEvent(QKeyEvent* e) override;
  virtual plEditorInput DoKeyReleaseEvent(QKeyEvent* e) override;

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
  plEngineGizmoHandle m_hMarqueeGizmo;
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
