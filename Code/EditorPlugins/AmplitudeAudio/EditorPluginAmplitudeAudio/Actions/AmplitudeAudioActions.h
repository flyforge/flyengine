#pragma once

#include <EditorPluginAmplitudeAudio/EditorPluginAmplitudeAudioDLL.h>

#include <Foundation/Configuration/CVar.h>

#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plPreferences;

class PL_EDITORPLUGINAMPLITUDEAUDIO_DLL plAmplitudeAudioActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hCategoryAudioSystem;
  static plActionDescriptorHandle s_hProjectSettings;
  static plActionDescriptorHandle s_hReloadControls;
};


class PL_EDITORPLUGINAMPLITUDEAUDIO_DLL plAmplitudeAudioAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plAmplitudeAudioAction, plButtonAction);

public:
  enum class ActionType
  {
    ProjectSettings,
    ReloadControls
  };

  plAmplitudeAudioAction(const plActionContext& context, const char* szName, ActionType type);
  ~plAmplitudeAudioAction() override;

  virtual void Execute(const plVariant& value) override;

private:
  void OnPreferenceChange(plPreferences* pref);

  ActionType m_Type;
};
