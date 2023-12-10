#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/SwapChain.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALSwapChain, plNoBase, 1, plRTTINoAllocator)
{
}
PLASMA_END_STATIC_REFLECTED_TYPE;

PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plGALWindowSwapChain, plGALSwapChain, 1, plRTTINoAllocator)
{
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plGALSwapChainCreationDescription CreateSwapChainCreationDescription(const plRTTI* pType)
{
  plGALSwapChainCreationDescription desc;
  desc.m_pSwapChainType = pType;
  return desc;
}

plGALSwapChain::plGALSwapChain(const plRTTI* pSwapChainType)
  : plGALObject(CreateSwapChainCreationDescription(pSwapChainType))
{
}

plGALSwapChain::~plGALSwapChain() = default;

//////////////////////////////////////////////////////////////////////////

plGALWindowSwapChain::Functor plGALWindowSwapChain::s_Factory;


plGALWindowSwapChain::plGALWindowSwapChain(const plGALWindowSwapChainCreationDescription& Description)
  : plGALSwapChain(plGetStaticRTTI<plGALWindowSwapChain>())
  , m_WindowDesc(Description)
{
}

void plGALWindowSwapChain::SetFactoryMethod(Functor factory)
{
  s_Factory = factory;
}

plGALSwapChainHandle plGALWindowSwapChain::Create(const plGALWindowSwapChainCreationDescription& desc)
{
  PLASMA_ASSERT_DEV(s_Factory.IsValid(), "No factory method assigned for plGALWindowSwapChain.");
  return s_Factory(desc);
}

PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_Device_Implementation_SwapChain);
