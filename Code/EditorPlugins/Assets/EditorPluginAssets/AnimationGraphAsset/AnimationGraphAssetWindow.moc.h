#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtAnimationGraphAssetScene;
class plQtNodeView;

class plQtAnimationGraphAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtAnimationGraphAssetDocumentWindow(plDocument* pDocument);
  ~plQtAnimationGraphAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "AnimationGraphAsset"; }

private Q_SLOTS:

private:
  void SelectionEventHandler(const plSelectionManagerEvent& e);

  plQtAnimationGraphAssetScene* m_pScene;
  plQtNodeView* m_pView;
};
