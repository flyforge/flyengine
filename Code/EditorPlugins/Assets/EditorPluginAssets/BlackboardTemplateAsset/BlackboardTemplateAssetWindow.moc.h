#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtBlackboardTemplateAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtBlackboardTemplateAssetDocumentWindow(plDocument* pDocument);
  ~plQtBlackboardTemplateAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "BlackboardTemplateAsset"; }
};
