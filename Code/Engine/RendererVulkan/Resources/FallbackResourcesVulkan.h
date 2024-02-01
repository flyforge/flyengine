#pragma once
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/RendererFoundationDLL.h>

#include <vulkan/vulkan.hpp>

class plGALDeviceVulkan;
class plGALResourceViewVulkan;
class plGALUnorderedAccessViewVulkan;

/// \brief Creates fallback resources in case the high-level renderer did not map a resource to a descriptor slot.
/// #TODO_VULKAN: Although the class has 'Vulkan' in the name, it could be made GAL agnostic by just returning the base class of the resource views and then it will work for any device type so it could be moved to RendererFoundation if needed for another GAL implementation.
class plFallbackResourcesVulkan
{
public:
  /// Returns a fallback resource for the given shader resource type.
  /// \param descriptorType The shader resource descriptor for which a compatible fallback resource is requested.
  /// \param textureType In case descriptorType is a texture, this specifies the texture type.
  /// \param bDepth Whether the shader resource is using a depth sampler.
  /// \return
  static const plGALResourceViewVulkan* GetFallbackResourceView(plGALShaderResourceType::Enum descriptorType, plGALShaderTextureType::Enum textureType, bool bDepth);
  static const plGALUnorderedAccessViewVulkan* GetFallbackUnorderedAccessView(plGALShaderResourceType::Enum descriptorType, plGALShaderTextureType::Enum textureType);

private:
  PL_MAKE_SUBSYSTEM_STARTUP_FRIEND(RendererVulkan, FallbackResourcesVulkan)
  static void GALDeviceEventHandler(const plGALDeviceEvent& e);
  static void Initialize();
  static void DeInitialize();

  static plGALDevice* s_pDevice;
  static plEventSubscriptionID s_EventID;

  struct Key
  {
    PL_DECLARE_POD_TYPE();
    plEnum<plGALShaderResourceType> m_ResourceType;
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
