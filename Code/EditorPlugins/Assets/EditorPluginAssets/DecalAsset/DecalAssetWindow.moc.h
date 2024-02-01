#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plDecalAssetDocument;
class plQtOrbitCamViewWidget;

class plQtDecalAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtDecalAssetDocumentWindow(plDecalAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "DecalAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
};

