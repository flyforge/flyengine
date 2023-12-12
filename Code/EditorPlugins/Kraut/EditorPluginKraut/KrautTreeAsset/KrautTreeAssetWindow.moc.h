#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginKraut/KrautTreeAsset/KrautTreeAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;

class plQtKrautTreeAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtKrautTreeAssetDocumentWindow(plAssetDocument* pDocument);
  ~plQtKrautTreeAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "KrautTreeAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const PlasmaEditorEngineDocumentMsg* pMsg) override;
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);

  PlasmaEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
  plKrautTreeAssetDocument* m_pAssetDoc;
};
