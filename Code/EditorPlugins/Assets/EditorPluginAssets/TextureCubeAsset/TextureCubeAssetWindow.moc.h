#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plTextureCubeAssetDocument;

class plQtTextureCubeAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtTextureCubeAssetDocumentWindow(plTextureCubeAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureCubeAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
};
