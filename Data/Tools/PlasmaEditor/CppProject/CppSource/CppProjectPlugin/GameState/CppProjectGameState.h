#pragma once

#include <Core/Input/Declarations.h>
#include <Core/World/Declarations.h>
#include <CppProjectPlugin/CppProjectPluginDLL.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <GameEngine/GameState/FallbackGameState.h>
#include <GameEngine/GameState/GameState.h>

class CppProjectGameState : public plFallbackGameState
{
  PLASMA_ADD_DYNAMIC_REFLECTION(CppProjectGameState, plFallbackGameState);

public:
  CppProjectGameState();
  ~CppProjectGameState();

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

  plDeque<plGameObjectHandle> m_SpawnedObjects;
};
