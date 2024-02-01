#include <RendererFoundation/RendererFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/ImmutableSamplers.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererFoundation, ImmutableSamplers)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plGALImmutableSamplers::OnEngineStartup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plGALImmutableSamplers::OnEngineShutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

bool plGALImmutableSamplers::s_bInitialized = false;
plGALImmutableSamplers::ImmutableSamplers plGALImmutableSamplers::s_ImmutableSamplers;
plHashTable<plHashedString, plGALSamplerStateCreationDescription> plGALImmutableSamplers::s_ImmutableSamplerDesc;

void plGALImmutableSamplers::OnEngineStartup()
{
  plGALDevice::s_Events.AddEventHandler(plMakeDelegate(&plGALImmutableSamplers::GALDeviceEventHandler));
}

void plGALImmutableSamplers::OnEngineShutdown()
{
  plGALDevice::s_Events.RemoveEventHandler(plMakeDelegate(&plGALImmutableSamplers::GALDeviceEventHandler));

  PL_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "plGALDeviceEvent::BeforeShutdown should have been fired before engine shutdown");
  s_ImmutableSamplers.Clear();
  s_ImmutableSamplerDesc.Clear();
}

plResult plGALImmutableSamplers::RegisterImmutableSampler(plHashedString sSamplerName, const plGALSamplerStateCreationDescription& desc)
{
  PL_ASSERT_DEBUG(!s_bInitialized, "Registering immutable samplers is only allowed at sub-system startup");
  if (s_ImmutableSamplerDesc.Contains(sSamplerName))
    return PL_FAILURE;

  s_ImmutableSamplerDesc.Insert(sSamplerName, desc);
  return PL_SUCCESS;
}

void plGALImmutableSamplers::GALDeviceEventHandler(const plGALDeviceEvent& e)
{
  switch (e.m_Type)
  {
    case plGALDeviceEvent::AfterInit:
      CreateSamplers(e.m_pDevice);
      break;
    case plGALDeviceEvent::BeforeShutdown:
      DestroySamplers(e.m_pDevice);
      break;
    case plGALDeviceEvent::BeforeBeginFrame:
    case plGALDeviceEvent::AfterBeginFrame:
    case plGALDeviceEvent::BeforeEndFrame:
    case plGALDeviceEvent::AfterEndFrame:
      break;
  }
}

void plGALImmutableSamplers::CreateSamplers(plGALDevice* pDevice)
{
  PL_ASSERT_DEBUG(s_ImmutableSamplers.IsEmpty(), "Creating more than one GAL device is not supported");
  for (auto it : s_ImmutableSamplerDesc)
  {
    plGALSamplerStateHandle hSampler = pDevice->CreateSamplerState(it.Value());
    s_ImmutableSamplers.Insert(it.Key(), hSampler);
  }
  s_bInitialized = true;
}

void plGALImmutableSamplers::DestroySamplers(plGALDevice* pDevice)
{
  for (auto it : s_ImmutableSamplers)
  {
    pDevice->DestroySamplerState(it.Value());
  }
  s_ImmutableSamplers.Clear();
  s_bInitialized = false;
}

const plGALImmutableSamplers::ImmutableSamplers& plGALImmutableSamplers::GetImmutableSamplers()
{
  return s_ImmutableSamplers;
}
