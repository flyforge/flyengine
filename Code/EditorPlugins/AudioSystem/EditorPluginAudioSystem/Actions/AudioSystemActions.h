#pragma once

#include <EditorPluginAudioSystem/EditorPluginAudioSystemDLL.h>

#include <Foundation/Configuration/CVar.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plPreferences;

class PL_EDITORPLUGINAUDIOSYSTEM_DLL plAudioSystemActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(const char* szMapping);
  static void MapToolbarActions(const char* szMapping);

  static plActionDescriptorHandle s_hCatAudioSettings;
  static plActionDescriptorHandle s_hCategoryAudioSystem;
  static plActionDescriptorHandle s_hMuteSound;
  static plActionDescriptorHandle s_hMasterVolume;
};


class PL_EDITORPLUGINAUDIOSYSTEM_DLL plAudioSystemAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemAction, plButtonAction);

public:
  enum class ActionType
  {
    MuteSound,
  };

  plAudioSystemAction(const plActionContext& context, const char* szName, ActionType type);
  ~plAudioSystemAction() override;

  virtual void Execute(const plVariant& value) override;

private:
  void OnPreferenceChange(plPreferences* pref);

  ActionType m_Type;
};

class PL_EDITORPLUGINAUDIOSYSTEM_DLL plAudioSystemSliderAction : public plSliderAction
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioSystemSliderAction, plSliderAction);

public:
  enum class ActionType
  {
    MasterVolume,
  };

  plAudioSystemSliderAction(const plActionContext& context, const char* szName, ActionType type);
  ~plAudioSystemSliderAction() override;

  virtual void Execute(const plVariant& value) override;

private:
  void OnPreferenceChange(plPreferences* pref);
  void UpdateState();

  ActionType m_Type;
};
