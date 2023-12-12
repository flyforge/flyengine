#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/SkeletonAsset/SkeletonAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plSelectionContext;

class plQtSkeletonAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtSkeletonAssetDocumentWindow(plSkeletonAssetDocument* pDocument);
  ~plQtSkeletonAssetDocumentWindow();

  plSkeletonAssetDocument* GetSkeletonDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "SkeletonAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const PlasmaEditorEngineDocumentMsg* pMsg) override;

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void SelectionEventHandler(const plSelectionManagerEvent& e);
  void SkeletonAssetEventHandler(const plSkeletonAssetEvent& e);

  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  void CommandEventHandler(const plCommandHistoryEvent&);

  void SendLiveResourcePreview();
  void RestoreResource();

  PlasmaEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget = nullptr;
};

