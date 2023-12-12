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

  PlasmaEngineViewConfig m_ViewConfig;
  plQtOrbitCamViewWidget* m_pViewWidget;
};

class plTextureCubeChannelModeAction : public plEnumerationMenuAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeChannelModeAction, plEnumerationMenuAction);

public:
  plTextureCubeChannelModeAction(const plActionContext& context, const char* szName, const char* szIconPath);
  virtual plInt64 GetValue() const override;
  virtual void Execute(const plVariant& value) override;
};

class plTextureCubeLodSliderAction : public plSliderAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plTextureCubeLodSliderAction, plSliderAction);

public:
  plTextureCubeLodSliderAction(const plActionContext& context, const char* szName);

  virtual void Execute(const plVariant& value) override;

private:
  plTextureCubeAssetDocument* m_pDocument;
};

class plTextureCubeAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hTextureChannelMode;
  static plActionDescriptorHandle s_hLodSlider;
};

