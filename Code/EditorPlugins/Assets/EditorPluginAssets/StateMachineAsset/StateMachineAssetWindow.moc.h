#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtStateMachineAssetScene;
class plQtNodeView;

class plQtStateMachineAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtStateMachineAssetDocumentWindow(plDocument* pDocument);
  ~plQtStateMachineAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "StateMachineAsset"; }

private Q_SLOTS:

private:
  plQtStateMachineAssetScene* m_pScene;
  plQtNodeView* m_pView;
};
