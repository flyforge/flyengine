#pragma once

#include <EditorPluginParticle/EditorPluginParticleDLL.h>
#include <GuiFoundation/Action/BaseActions.h>
#include <GuiFoundation/GuiFoundationDLL.h>

class plParticleEffectAssetDocument;
struct plParticleEffectAssetEvent;

class plParticleActions
{
public:
  static void RegisterActions();
  static void UnregisterActions();

  static void MapActions(const char* szMapping, const char* szPath);

  static plActionDescriptorHandle s_hCategory;
  static plActionDescriptorHandle s_hPauseEffect;
  static plActionDescriptorHandle s_hRestartEffect;
  static plActionDescriptorHandle s_hAutoRestart;
  static plActionDescriptorHandle s_hSimulationSpeedMenu;
  static plActionDescriptorHandle s_hSimulationSpeed[10];
  static plActionDescriptorHandle s_hRenderVisualizers;
};

class plParticleAction : public plButtonAction
{
  PLASMA_ADD_DYNAMIC_REFLECTION(plParticleAction, plButtonAction);

public:
  enum class ActionType
  {
    PauseEffect,
    RestartEffect,
    AutoRestart,
    SimulationSpeed,
    RenderVisualizers,
  };

  plParticleAction(const plActionContext& context, const char* szName, ActionType type, float fSimSpeed = 1.0f);
  ~plParticleAction();

  virtual void Execute(const plVariant& value) override;

private:
  void EffectEventHandler(const plParticleEffectAssetEvent& e);
  void UpdateState();

  plParticleEffectAssetDocument* m_pEffectDocument;
  ActionType m_Type;
  float m_fSimSpeed;
};
