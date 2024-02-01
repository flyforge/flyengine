#pragma once

#include <EditorEngineProcessFramework/EngineProcess/ViewRenderSettings.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>

/// \brief Actions for configuring the engine view light settings.
class PL_EDITORFRAMEWORK_DLL plViewLightActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hLightMenu;
  static plActionDescriptorHandle s_hSkyBox;
  static plActionDescriptorHandle s_hSkyLight;
  static plActionDescriptorHandle s_hSkyLightCubeMap;
  static plActionDescriptorHandle s_hSkyLightIntensity;
  static plActionDescriptorHandle s_hDirLight;
  static plActionDescriptorHandle s_hDirLightAngle;
  static plActionDescriptorHandle s_hDirLightShadows;
  static plActionDescriptorHandle s_hDirLightIntensity;
  static plActionDescriptorHandle s_hFog;
  static plActionDescriptorHandle s_hSetAsDefault;
};

class PL_EDITORFRAMEWORK_DLL plViewLightButtonAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plViewLightButtonAction, plButtonAction);

public:
  plViewLightButtonAction(const plActionContext& context, const char* szName, plEngineViewLightSettingsEvent::Type button);
  ~plViewLightButtonAction();

  virtual void Execute(const plVariant& value) override;
  void LightSettingsEventHandler(const plEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  plEngineViewLightSettingsEvent::Type m_ButtonType;
  plEngineViewLightSettings* m_pSettings = nullptr;
  plEventSubscriptionID m_SettingsID;
};

class PL_EDITORFRAMEWORK_DLL plViewLightSliderAction : public plSliderAction
{
  PL_ADD_DYNAMIC_REFLECTION(plViewLightSliderAction, plSliderAction);

public:
  plViewLightSliderAction(const plActionContext& context, const char* szName, plEngineViewLightSettingsEvent::Type button);
  ~plViewLightSliderAction();

  virtual void Execute(const plVariant& value) override;
  void LightSettingsEventHandler(const plEngineViewLightSettingsEvent& e);
  void UpdateAction();

private:
  plEngineViewLightSettingsEvent::Type m_ButtonType;
  plEngineViewLightSettings* m_pSettings = nullptr;
  plEventSubscriptionID m_SettingsID;
};
