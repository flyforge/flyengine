#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/DocumentWindow/EngineDocumentWindow.moc.h>
#include <Foundation/Basics.h>
#include <GuiFoundation/Action/Action.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/DocumentWindow/DocumentWindow.moc.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>

class plQtOrbitCamViewWidget;
class plTextureAssetDocument;

class plQtTextureAssetDocumentWindow : public plQtEngineDocumentWindow
{
  Q_OBJECT

public:
  plQtTextureAssetDocumentWindow(plTextureAssetDocument* pDocument);

  virtual const char* GetWindowLayoutGroupName() const override { return "TextureAsset"; }

private:
  virtual void InternalRedraw() override;
  void SendRedrawMsg();

  plEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
};

class plTextureChannelModeAction : public plEnumerationMenuAction
{
  PL_ADD_DYNAMIC_REFLECTION(plTextureChannelModeAction, plEnumerationMenuAction);

public:
  plTextureChannelModeAction(const plActionContext& context, const char* szName, const char* szIconPath);
  virtual plInt64 GetValue() const override;
  virtual void Execute(const plVariant& value) override;
};

class plTextureLodSliderAction : public plSliderAction
{
  PL_ADD_DYNAMIC_REFLECTION(plTextureLodSliderAction, plSliderAction);

public:
  plTextureLodSliderAction(const plActionContext& context, const char* szName);

  virtual void Execute(const plVariant& value) override;

private:
  plTextureAssetDocument* m_pDocument;
};

class plTextureAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hTextureChannelMode;
  static plActionDescriptorHandle s_hLodSlider;
};

