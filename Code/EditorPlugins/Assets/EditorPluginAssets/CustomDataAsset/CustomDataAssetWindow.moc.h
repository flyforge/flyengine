#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtCustomDataAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtCustomDataAssetDocumentWindow(plDocument* pDocument);
  ~plQtCustomDataAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "CustomDataAsset"; }
};