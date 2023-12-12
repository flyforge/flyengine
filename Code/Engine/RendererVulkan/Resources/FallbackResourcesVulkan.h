#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class plGALDeviceVulkan;
class plGALResourceViewVulkan;
class plGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
class plFallbackResourcesVulkan
{
public:
  static void Initialize(plGALDeviceVulkan* pDevice);
  static void DeInitialize();

  static const plGALResourceViewVulkan* GetFallbackResourceView(vk::DescriptorType descriptorType, plShaderResourceType::Enum plType, bool bDepth);
  static const plGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(vk::DescriptorType descriptorType, plShaderResourceType::Enum plType);

private:
  static void GALDeviceEventHandler(const plGALDeviceEvent& e);

  static plGALDeviceVulkan* s_pDevice;
  static plEventSubscriptionID s_EventID;

  struct Key
  {
    PLASMA_DECLARE_POD_TYPE();
    vk::DescriptorType m_descriptorType;
    plShaderResourceType::Enum m_plType;
    bool m_bDepth = false;
  };

  struct KeyHash
  {
    static plUInt32 Hash(const Key& a);
    static bool Equal(const Key& a, const Key& b);
  };

  static plHashTable<Key, plGALResourceViewHandle, KeyHash> m_ResourceViews;
  static plHashTable<Key, plGALUnorderedAccessViewHandle, KeyHash> m_UAVs;

  static plDynamicArray<plGALBufferHandle> m_Buffers;
  static plDynamicArray<plGALTextureHandle> m_Textures;
};
