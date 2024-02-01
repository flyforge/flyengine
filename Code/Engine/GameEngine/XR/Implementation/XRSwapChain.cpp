#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRSwapChain.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plGALXRSwapChain, plGALSwapChain, 1, plRTTINoAllocator)
{
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plGALXRSwapChain::Functor plGALXRSwapChain::s_Factory;

plGALXRSwapChain::plGALXRSwapChain(plXRInterface* pXrInterface)
  : plGALSwapChain(plGetStaticRTTI<plGALXRSwapChain>())
  , m_pXrInterface(pXrInterface)
{
}

plResult plGALXRSwapChain::UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode)
{
  return PL_FAILURE;
}

void plGALXRSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

plGALSwapChainHandle plGALXRSwapChain::Create(plXRInterface* pXrInterface)
{
  PL_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for plGALXRSwapChain.");
  return s_Factory(pXrInterface);
}


PL_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_XRSwapChain);
