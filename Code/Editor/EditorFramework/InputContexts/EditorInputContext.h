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

enum class PlasmaEditorInput
{
  MayBeHandledByOthers,
  WasExclusivelyHandled,
};

class PLASMA_EDITORFRAMEWORK_DLL PlasmaEditorInputContext : public plReflectedClass
{
  PLASMA_ADD_DYNAMIC_REFLECTION(PlasmaEditorInputContext, plReflectedClass);

public:
  PlasmaEditorInputContext();

  virtual ~PlasmaEditorInputContext();

  void FocusLost(bool bCancel);

  PlasmaEditorInput KeyPressEvent(QKeyEvent* e) { return DoKeyPressEvent(e); }
  PlasmaEditorInput KeyReleaseEvent(QKeyEvent* e) { return DoKeyReleaseEvent(e); }
  PlasmaEditorInput MousePressEvent(QMouseEvent* e) { return DoMousePressEvent(e); }
  PlasmaEditorInput MouseReleaseEvent(QMouseEvent* e) { return DoMouseReleaseEvent(e); }
  PlasmaEditorInput MouseMoveEvent(QMouseEvent* e);
  PlasmaEditorInput WheelEvent(QWheelEvent* e) { return DoWheelEvent(e); }

  static void SetActiveInputContext(PlasmaEditorInputContext* pContext) { s_pActiveInputContext = pContext; }

  void MakeActiveInputContext(bool bActive = true);

  static bool IsAnyInputContextActive() { return s_pActiveInputContext != nullptr; }

  static PlasmaEditorInputContext* GetActiveInputContext() { return s_pActiveInputContext; }

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

  virtual PlasmaEditorInput DoKeyPressEvent(QKeyEvent* e);
  virtual PlasmaEditorInput DoKeyReleaseEvent(QKeyEvent* e) { return PlasmaEditorInput::MayBeHandledByOthers; }
  virtual PlasmaEditorInput DoMousePressEvent(QMouseEvent* e) { return PlasmaEditorInput::MayBeHandledByOthers; }
  virtual PlasmaEditorInput DoMouseReleaseEvent(QMouseEvent* e) { return PlasmaEditorInput::MayBeHandledByOthers; }
  virtual PlasmaEditorInput DoMouseMoveEvent(QMouseEvent* e) { return PlasmaEditorInput::MayBeHandledByOthers; }
  virtual PlasmaEditorInput DoWheelEvent(QWheelEvent* e) { return PlasmaEditorInput::MayBeHandledByOthers; }

private:
  static PlasmaEditorInputContext* s_pActiveInputContext;

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
