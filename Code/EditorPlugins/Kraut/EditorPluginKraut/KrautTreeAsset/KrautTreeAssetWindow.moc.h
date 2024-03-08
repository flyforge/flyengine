#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plQtPropertyGridWidget;

class plQtKrautTreeAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtKrautTreeAssetDocumentWindow(plAssetDocument* pDocument);
  ~plQtKrautTreeAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "KrautTreeAsset"; }

  plKrautTreeAssetDocument* GetKrautDocument() const
  {
    return static_cast<plKrautTreeAssetDocument*>(GetDocument());
  }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);

private Q_SLOTS:
  void onBranchTypeSelected(int index);

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void UpdatePreview();
  void RestoreResource();

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget = nullptr;
  plKrautTreeAssetDocument* m_pAssetDoc = nullptr;
  plQtPropertyGridWidget* m_pBranchProps = nullptr;
};
