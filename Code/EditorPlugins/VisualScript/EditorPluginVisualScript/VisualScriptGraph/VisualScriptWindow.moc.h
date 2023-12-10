#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtVisualScriptNodeScene;
class plQtNodeView;

class plQtVisualScriptWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtVisualScriptWindow(plDocument* pDocument);
  ~plQtVisualScriptWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "VisualScriptGraph"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  plQtVisualScriptNodeScene* m_pScene;
  plQtNodeView* m_pView;
};
