#pragma once

#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plProcGenGraphAssetDocument;

class plQtNodeScene;
class plQtNodeView;
struct plCommandHistoryEvent;

class plProcGenGraphAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plProcGenGraphAssetDocumentWindow(plProcGenGraphAssetDocument* pDocument);
  ~plProcGenGraphAssetDocumentWindow();

  plProcGenGraphAssetDocument* GetProcGenGraphDocument();

  virtual const char* GetWindowLayoutGroupName() const override { return "ProcGenAsset"; }

private Q_SLOTS:


private:
  void UpdatePreview();
  void RestoreResource();

  // needed for setting the debug pin
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void TransationEventHandler(const plCommandHistoryEvent& e);

  plQtNodeScene* m_pScene;
  plQtNodeView* m_pView;
};
