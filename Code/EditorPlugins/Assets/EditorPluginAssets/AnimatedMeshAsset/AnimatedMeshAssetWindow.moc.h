#pragma once

#include <EditorPluginAssets/EditorPluginAssetsDLL.h>

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginAssets/AnimatedMeshAsset/AnimatedMeshAsset.h>
#include <Foundation/Basics.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

#include <QPointer>

class plQtOrbitCamViewWidget;

class plQtAnimatedMeshAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtAnimatedMeshAssetDocumentWindow(plAnimatedMeshAssetDocument* pDocument);
  ~plQtAnimatedMeshAssetDocumentWindow();

  plAnimatedMeshAssetDocument* GetMeshDocument();
  virtual const char* GetWindowLayoutGroupName() const override { return "AnimatedMeshAsset"; }

protected:
  virtual void InternalRedraw() override;
  virtual void ProcessMessageEventHandler(const plEditorEngineDocumentMsg* pMsg) override;

protected Q_SLOTS:
  void HighlightTimer();

private:
  void SendRedrawMsg();
  void QueryObjectBBox(plInt32 iPurpose = 0);
  void PropertyEventHandler(const plDocumentObjectPropertyEvent& e);
  bool UpdatePreview();

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
  plUInt32 m_uiHighlightSlots = 0;
  QPointer<QTimer> m_pHighlightTimer;
};
