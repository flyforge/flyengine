#pragma once

#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plAssetDocument;

class PL_EDITORFRAMEWORK_DLL plCommonAssetActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapToolbarActions(plStringView sMapping, plUInt32 uiStateMask);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hPause;
  static plActionDescriptorHandle s_hRestart;
  static plActionDescriptorHandle s_hLoop;
  static plActionDescriptorHandle s_hSimulationSpeedMenu;
  static plActionDescriptorHandle s_hSimulationSpeed[10];
  static plActionDescriptorHandle s_hGrid;
  static plActionDescriptorHandle s_hVisualizers;
};

class PL_EDITORFRAMEWORK_DLL plCommonAssetAction : public plButtonAction
{
  PL_ADD_DYNAMIC_REFLECTION(plCommonAssetAction, plButtonAction);

public:
  enum class ActionType
  {
    Pause,
    Restart,
    Loop,
    SimulationSpeed,
    Grid,
    Visualizers,
  };

  plCommonAssetAction(const plActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~plCommonAssetAction();

  virtual void Execute(const plVariant& value) override;

private:
  void CommonUiEventHandler(const plCommonAssetUiState& e);
  void UpdateState();

  plAssetDocument* m_pAssetDocument = nullptr;
  ActionType m_Type;
  float m_fSimSpeed;
};
