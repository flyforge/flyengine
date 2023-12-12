#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <EditorPluginRmlUi/RmlUiAsset/RmlUiAsset.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtEngineViewWidget;

class plQtRmlUiAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtRmlUiAssetDocumentWindow(plAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "RmlUiAsset"; }

protected:
  virtual void InternalRedraw() override;

private:
  void SendRedrawMsg();

  PlasmaEngineViewConfig m_ViewConfig;
  plQtEngineViewWidget* m_pViewWidget;
  plRmlUiAssetDocument* m_pAssetDoc;
};

