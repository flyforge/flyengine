#pragma once

#include <GameEngine/GameEngineDLL.h>

#include <RendererFoundation/Device/SwapChain.h>

class plXRInterface;

class PLASMA_GAMEENGINE_DLL plGALXRSwapChain : public plGALSwapChain
{
public:
  using Functor = plDelegate<plGALSwapChainHandle(plXRInterface*)>;
  static void SetFactoryMethod(Functor factory);

  static plGALSwapChainHandle Create(plXRInterface* pXrInterface);

public:
  plGALXRSwapChain(plXRInterface* pXrInterface);
  virtual plResult UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode) override;

protected:
  static Functor s_Factory;

protected:
  plXRInterface* m_pXrInterface = nullptr;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_GAMEENGINE_DLL, plGALXRSwapChain);