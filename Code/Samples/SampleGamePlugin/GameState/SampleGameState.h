#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>
#include <SampleGamePlugin/SampleGamePluginDLL.h>

class PLASMA_SAMPLEGAMEPLUGIN_DLL SampleGameState : public plFallbackGameState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(SampleGameState, plFallbackGameState);

public:
  SampleGameState();

  virtual plGameStatePriority DeterminePriority(plWorld* pWorld) const override;

  virtual void ProcessInput() override;

protected:
  virtual void ConfigureMainWindowInputDevices(plWindow* pWindow) override;
  virtual void ConfigureInputActions() override;
  virtual void ConfigureMainCamera() override;

private:
  virtual void OnActivation(plWorld* pWorld, const plTransform* pStartPosition) override;
  virtual void OnDeactivation() override;
  virtual void BeforeWorldUpdate() override;
  virtual void AfterWorldUpdate() override;

  // BEGIN-DOCS-CODE-SNIPPET: confunc-decl
  void ConFunc_Print(plString sText);
  plConsoleFunction<void(plString)> m_ConFunc_Print;
  // END-DOCS-CODE-SNIPPET

  plDeque<plGameObjectHandle> m_SpawnedObjects;
};
