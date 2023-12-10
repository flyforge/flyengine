#pragma once

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <EditorFramework/IPC/IPCObjectMirrorEditor.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>

class QWidget;
class QHBoxLayout;
class QPushButton;
class plQtEngineViewWidget;
class plAssetDocument;
class plEditorEngineDocumentMsg;
struct plObjectPickingResult;
struct plEngineViewConfig;
struct plCommonAssetUiState;


struct PLASMA_EDITORFRAMEWORK_DLL plEngineWindowEvent
{
  enum class Type
  {
    ViewCreated,
    ViewDestroyed,
  };

  Type m_Type;
  plQtEngineViewWidget* m_pView = nullptr;
};

/// \brief Base class for all document windows that need a connection to the engine process, and might want to render 3D content.
///
/// This class has an plEditorEngineConnection object for sending messages between the editor and the engine process.
/// It also allows to embed plQtEngineViewWidget objects into the UI, which enable 3D rendering by the engine process.
class PLASMA_EDITORFRAMEWORK_DLL plQtEngineDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtEngineDocumentWindow(plAssetDocument* pDocument);
  virtual ~plQtEngineDocumentWindow();

  plEditorEngineConnection* GetEditorEngineConnection() const;
  const plObjectPickingResult& PickObject(plUInt16 uiScreenPosX, plUInt16 uiScreenPosY, plQtEngineViewWidget* pView) const;

  plAssetDocument* GetDocument() const;

  /// \brief Returns the plQtEngineViewWidget over which the mouse currently hovers
  plQtEngineViewWidget* GetHoveredViewWidget() const;

  /// \brief Returns the plQtEngineViewWidget that has the input focus
  plQtEngineViewWidget* GetFocusedViewWidget() const;

  plQtEngineViewWidget* GetViewWidgetByID(plUInt32 uiViewID) const;

  plArrayPtr<plQtEngineViewWidget* const> GetViewWidgets() const;

  void AddViewWidget(plQtEngineViewWidget* pView);

public:
  mutable plEvent<const plEngineWindowEvent&> m_EngineWindowEvent;

protected:
  friend class plQtEngineViewWidget;
  plHybridArray<plQtEngineViewWidget*, 4> m_ViewWidgets;

  virtual void CommonAssetUiEventHandler(const plCommonAssetUiState& e);

  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg);
  void RemoveViewWidget(plQtEngineViewWidget* pView);
  void DestroyAllViews();
  virtual void InternalRedraw() override;
};
