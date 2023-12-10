#pragma once

#include <Foundation/Basics.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtSurfaceAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtSurfaceAssetDocumentWindow(plDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "SurfaceAsset"; }
};

