#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <Foundation/Math/Rect.h>
#include <Foundation/Reflection/Reflection.h>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
class plDocument;
class plQtEngineDocumentWindow;
class plQtEngineViewWidget;

enum class plEditorInput
{
  MayBeHandledByOthers,
  WasExclusivelyHandled,
};

class PL_EDITORFRAMEWORK_DLL plEditorInputContext : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plEditorInputContext, plReflectedClass);

public:
  plEditorInputContext();

  virtual ~plEditorInputContext();

  void FocusLost(bool bCancel);

  plEditorInput KeyPressEvent(QKeyEvent* e) { return DoKeyPressEvent(e); }
  plEditorInput KeyReleaseEvent(QKeyEvent* e) { return DoKeyReleaseEvent(e); }
  plEditorInput MousePressEvent(QMouseEvent* e) { return DoMousePressEvent(e); }
  plEditorInput MouseReleaseEvent(QMouseEvent* e) { return DoMouseReleaseEvent(e); }
  plEditorInput MouseMoveEvent(QMouseEvent* e);
  plEditorInput WheelEvent(QWheelEvent* e) { return DoWheelEvent(e); }

  static void SetActiveInputContext(plEditorInputContext* pContext) { s_pActiveInputContext = pContext; }

  void MakeActiveInputContext(bool bActive = true);

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static plEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

  static void UpdateActiveInputContext();

  bool IsActiveInputContext() const;

  void SetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView);

  plQtEngineDocumentWindow* GetOwnerWindow() const;

  plQtEngineViewWidget* GetOwnerView() const;

  bool GetShortcutsDisabled() const { return m_bDisableShortcuts; }

  /// \brief If set to true, the surrounding window will ensure to block all shortcuts and instead send keypress events to the input context
  void SetShortcutsDisabled(bool bDisabled) { m_bDisableShortcuts = bDisabled; }

  virtual bool IsPickingSelectedAllowed() const { return true; }

  /// \brief How the mouse position is updated when the mouse cursor reaches the screen borders.
  enum class MouseMode
  {
    Normal,                     ///< Nothing happens, the mouse will stop at screen borders as usual
    WrapAtScreenBorders,        ///< The mouse is visibly wrapped at screen borders. When this mode is disabled, the mouse stays where it is.
    HideAndWrapAtScreenBorders, ///< The mouse is wrapped at screen borders, which enables infinite movement, but the cursor is invisible. When this
                                ///< mode is disabled the mouse is restored to the position where it was when it was enabled.
  };

  /// \brief Sets how the mouse will act when it reaches the screen border. UpdateMouseMode() must be called on every mouseMoveEvent to update the
  /// state.
  ///
  /// The return value is the current global mouse position. Can be used to initialize a 'Last Mouse Position' variable.
  plVec2I32 SetMouseMode(MouseMode mode);

  /// \brief Updates the mouse position. Can always be called but will only have an effect if SetMouseMode() was called with one of the wrap modes.
  ///
  /// Returns the new global mouse position, which may change drastically if the mouse cursor needed to be wrapped around the screen.
  /// Should be used to update a "Last Mouse Position" variable.
  plVec2I32 UpdateMouseMode(QMouseEvent* e);

  virtual void UpdateStatusBarText(plQtEngineDocumentWindow* pWindow) {}

protected:
  virtual void DoFocusLost(bool bCancel) {}

  virtual void OnSetOwner(plQtEngineDocumentWindow* pOwnerWindow, plQtEngineViewWidget* pOwnerView) = 0;

  virtual plEditorInput DoKeyPressEvent(QKeyEvent* e);
  virtual plEditorInput DoKeyReleaseEvent(QKeyEvent* e) { return plEditorInput::MayBeHandledByOthers; }
  virtual plEditorInput DoMousePressEvent(QMouseEvent* e) { return plEditorInput::MayBeHandledByOthers; }
  virtual plEditorInput DoMouseReleaseEvent(QMouseEvent* e) { return plEditorInput::MayBeHandledByOthers; }
  virtual plEditorInput DoMouseMoveEvent(QMouseEvent* e) { return plEditorInput::MayBeHandledByOthers; }
  virtual plEditorInput DoWheelEvent(QWheelEvent* e) { return plEditorInput::MayBeHandledByOthers; }

private:
  static plEditorInputContext* s_pActiveInputContext;

  plQtEngineDocumentWindow* m_pOwnerWindow;
  plQtEngineViewWidget* m_pOwnerView;
  bool m_bDisableShortcuts;
  bool m_bJustWrappedMouse;
  MouseMode m_MouseMode;
  plVec2I32 m_vMouseRestorePosition;
  plVec2I32 m_vMousePosBeforeWrap;
  plVec2I32 m_vExpectedMousePosition;
  plRectU32 m_MouseWrapRect;

  virtual void UpdateContext() {}
};
