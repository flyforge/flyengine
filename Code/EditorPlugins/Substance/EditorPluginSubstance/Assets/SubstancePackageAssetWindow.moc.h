#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtSubstancePackageAssetWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtSubstancePackageAssetWindow(plDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "SubstancePackageAsset"; }
};

