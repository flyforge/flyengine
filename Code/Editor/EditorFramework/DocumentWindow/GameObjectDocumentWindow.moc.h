#pragma once
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorFramework/EditorFrameworkDLL.h>

class plGameObjectDocument;
class plWorldSettingsMsgToEngine;
class plQtGameObjectViewWidget;
struct plGameObjectEvent;
struct plSnapProviderEvent;

class PLASMA_EDITORFRAMEWORK_DLL plQtGameObjectDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT
public:
  plQtGameObjectDocumentWindow(plGameObjectDocument* pDocument);
  ~plQtGameObjectDocumentWindow();

  plGameObjectDocument* GetGameObjectDocument() const;

protected:
  plWorldSettingsMsgToEngine GetWorldSettings() const;
  plGridSettingsMsgToEngine GetGridSettings() const;
  virtual void ProcessMessageEventHandler(const PlasmaEditorEngineDocumentMsg* pMsg) override;

private:
  void GameObjectEventHandler(const plGameObjectEvent& e);
  void SnapProviderEventHandler(const plSnapProviderEvent& e);

  void FocusOnSelectionAllViews();
  void FocusOnSelectionHoveredView();

  void HandleFocusOnSelection(const plQuerySelectionBBoxResultMsgToEditor* pMsg, plQtGameObjectViewWidget* pSceneView);
};

