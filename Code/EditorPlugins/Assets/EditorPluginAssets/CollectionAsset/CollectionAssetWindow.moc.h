#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtCollectionAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtCollectionAssetDocumentWindow(plDocument* pDocument);
  ~plQtCollectionAssetDocumentWindow();

  virtual const char* GetWindowLayoutGroupName() const override { return "CollectionAsset"; }
};

