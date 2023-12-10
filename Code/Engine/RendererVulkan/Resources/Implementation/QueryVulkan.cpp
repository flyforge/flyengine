#include <RendererVulkan/RendererVulkanPCH.h>

#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/QueryVulkan.h>

plGALQueryVulkan::plGALQueryVulkan(const plGALQueryCreationDescription& Description)
  : plGALQuery(Description)
{
}

plGALQueryVulkan::~plGALQueryVulkan() {}

plResult plGALQueryVulkan::InitPlatform(plGALDevice* pDevice)
{
  plGALDeviceVulkan* pVulkanDevice = static_cast<plGALDeviceVulkan*>(pDevice);

  if (true)
  {
    return PLASMA_SUCCESS;
  }
  else
  {
    plLog::Error("Creation of native Vulkan query failed!");
    return PLASMA_FAILURE;
  }
}

plResult plGALQueryVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  // TODO
  return PLASMA_SUCCESS;
}

void plGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  plUInt32 uiLength = plStringUtils::GetStringElementCount(szName);

  // TODO
}

PLASMA_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
