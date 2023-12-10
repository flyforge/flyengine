#pragma once

#include <EditorFramework/EditorFrameworkDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plPreferences;
struct plGameObjectEvent;
class plGameObjectDocument;
///
class PLASMA_EDITORFRAMEWORK_DLL plGameObjectDocumentActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapMenuActions(plStringView sMapping);
  static void MapMenuSimulationSpeed(plStringView sMapping);

  static void MapToolbarActions(plStringView sMapping);

  static plActionDescriptorHandle s_hGameObjectCategory;
  static plActionDescriptorHandle s_hRenderSelectionOverlay;
  static plActionDescriptorHandle s_hRenderVisualizers;
  static plActionDescriptorHandle s_hRenderShapeIcons;
  static plActionDescriptorHandle s_hRenderGrid;
  static plActionDescriptorHandle s_hAddAmbientLight;
  static plActionDescriptorHandle s_hSimulationSpeedMenu;
  static plActionDescriptorHandle s_hSimulationSpeed[10];
  static plActionDescriptorHandle s_hCameraSpeed;
  static plActionDescriptorHandle s_hPickTransparent;
};

///
class PLASMA_EDITORFRAMEWORK_DLL plGameObjectDocumentAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plGameObjectDocumentAction, plButtonAction);

public:
  enum class ActionType
  {
    RenderSelectionOverlay,
    RenderVisualizers,
    RenderShapeIcons,
    RenderGrid,
    AddAmbientLight,
    SimulationSpeed,
    PickTransparent,
  };

  plGameObjectDocumentAction(const plActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~plGameObjectDocumentAction();

  virtual void Execute(const plVariant& value) override;

private:
  void SceneEventHandler(const plGameObjectEvent& e);
  void OnPreferenceChange(plPreferences* pref);

  float m_fSimSpeed;
  plGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};


class PLASMA_EDITORFRAMEWORK_DLL plCameraSpeedSliderAction : public plSliderAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plCameraSpeedSliderAction, plSliderAction);

public:
  enum class ActionType
  {
    CameraSpeed,
  };

  plCameraSpeedSliderAction(const plActionContext& context, const char* szName, ActionType type);
  ~plCameraSpeedSliderAction();

  virtual void Execute(const plVariant& value) override;

private:
  void OnPreferenceChange(plPreferences* pref);
  void UpdateState();

  plGameObjectDocument* m_pGameObjectDocument;
  ActionType m_Type;
};
