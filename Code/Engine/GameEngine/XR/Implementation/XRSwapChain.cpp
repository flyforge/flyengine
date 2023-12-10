#include <GameEngine/GameEnginePCH.h>

#include <Foundation/Reflection/Reflection.h>
#include <GameEngine/XR/XRSwapChain.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALXRSwapChain, plGALSwapChain, 1, plRTTINoAllocator)
{
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plGALXRSwapChain::Functor plGALXRSwapChain::s_Factory;

plGALXRSwapChain::plGALXRSwapChain(plXRInterface* pXrInterface)
  : plGALSwapChain(plGetStaticRTTI<plGALXRSwapChain>())
  , m_pXrInterface(pXrInterface)
{
}

plResult plGALXRSwapChain::UpdateSwapChain(plGALDevice* pDevice, plEnum<plGALPresentMode> newPresentMode)
{
  return PLASMA_FAILURE;
}

void plGALXRSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

plGALSwapChainHandle plGALXRSwapChain::Create(plXRInterface* pXrInterface)
{
  PLASMA_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for plGALXRSwapChain.");
  return s_Factory(pXrInterface);
}
