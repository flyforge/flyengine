#pragma once

#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/DocumentWindow/EngineViewWidget.moc.h>
#include <EditorFramework/DocumentWindow/GameObjectDocumentWindow.moc.h>
#include <EditorFramework/EditTools/EditTool.h>
#include <EditorFramework/Gizmos/DragToPositionGizmo.h>
#include <EditorFramework/Gizmos/RotateGizmo.h>
#include <EditorFramework/Gizmos/ScaleGizmo.h>
#include <EditorFramework/Gizmos/TranslateGizmo.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/InputContexts/CameraMoveContext.h>
#include <EditorPluginScene/Actions/GizmoActions.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/PropertyGrid/Declarations.h>

struct plEngineViewPreferences;
class QGridLayout;
class plQtViewWidgetContainer;
class plQtSceneViewWidget;
class QSettings;
struct plManipulatorManagerEvent;
class plPreferences;
class plQtQuadViewWidget;
struct plEngineWindowEvent;
class plSceneDocument;
class QMenu;

Q_DECLARE_OPAQUE_POINTER(plQtSceneViewWidget*);

class plQtSceneDocumentWindowBase : public plQtGameObjectDocumentWindow, public plGameObjectGizmoInterface
{
  Q_OBJECT

public:
  plQtSceneDocumentWindowBase(plSceneDocument* pDocument);
  ~plQtSceneDocumentWindowBase();

  plSceneDocument* GetSceneDocument() const;

  virtual void CreateImageCapture(const char* szOutputPath) override;

public Q_SLOTS:
  void ToggleViews(QWidget* pView);

public:
  /// \name plGameObjectGizmoInterface implementation
  ///@{
  virtual plObjectAccessorBase* GetObjectAccessor() override;
  virtual bool CanDuplicateSelection() const override;
  virtual void DuplicateSelection() override;
  ///@}

protected:
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;
  virtual void InternalRedraw() override;

  void GameObjectEventHandler(const plGameObjectEvent& e);
  void SnapSelectionToPosition(bool bSnapEachObject);
  void SendRedrawMsg();
  void ExtendPropertyGridContextMenu(QMenu& menu, const plHybridArray<plPropertySelection, 8>& items, const plAbstractProperty* pProp);

protected:
  plQtQuadViewWidget* m_pQuadViewWidget = nullptr;
};

class plQtSceneDocumentWindow : public plQtSceneDocumentWindowBase
{
  Q_OBJECT

public:
  plQtSceneDocumentWindow(plSceneDocument* pDocument);
  ~plQtSceneDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "Scene"; }
};
