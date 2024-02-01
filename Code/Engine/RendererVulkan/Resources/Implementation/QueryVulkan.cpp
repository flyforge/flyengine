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
    return PL_SUCCESS;
  }
  else
  {
    plLog::Error("Creation of native Vulkan query failed!");
    return PL_FAILURE;
  }
}

plResult plGALQueryVulkan::DeInitPlatform(plGALDevice* pDevice)
{
  // TODO
  return PL_SUCCESS;
}

void plGALQueryVulkan::SetDebugNamePlatform(const char* szName) const
{
  plUInt32 uiLength = plStringUtils::GetStringElementCount(szName);

  // TODO
}

PL_STATICLINK_FILE(RendererVulkan, RendererVulkan_Resources_Implementation_QueryVulkan);
