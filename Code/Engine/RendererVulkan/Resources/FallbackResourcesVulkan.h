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

  static const plGALResourceViewVulkan* GetFallbackResourceView(plGALShaderDescriptorType::Enum descriptorType, plGALShaderTextureType::Enum textureType, bool bDepth);
  static const plGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(plGALShaderDescriptorType::Enum descriptorType, plGALShaderTextureType::Enum textureType);

private:
  static void GALDeviceEventHandler(const plGALDeviceEvent& e);

  static plGALDeviceVulkan* s_pDevice;
  static plEventSubscriptionID s_EventID;

  struct Key
  {
    PLASMA_DECLARE_POD_TYPE();
    plEnum<plGALShaderDescriptorType> m_descriptorType;
    plEnum<plGALShaderTextureType> m_plType;
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
