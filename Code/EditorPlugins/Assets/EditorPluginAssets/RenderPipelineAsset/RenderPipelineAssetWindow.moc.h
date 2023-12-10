#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtRenderPipelineAssetScene;
class plQtNodeView;

class plQtRenderPipelineAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtRenderPipelineAssetDocumentWindow(plDocument* pDocument);
  ~plQtRenderPipelineAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "RenderPipelineAsset"; }

private Q_SLOTS:

private:
  plQtRenderPipelineAssetScene* m_pScene;
  plQtNodeView* m_pView;
};

