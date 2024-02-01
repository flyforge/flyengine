#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plLUTAssetDocument;

class plQtLUTAssetDocumentWindow : public plQtDocumentWindow
{
  Q_OBJECT

public:
  plQtLUTAssetDocumentWindow(plLUTAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "LUTAsset"; }
};

class plLUTAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(plStringView sMapping);
};
