
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/Shader.h>

#include <RendererCore/Shader/ShaderStageBinary.h>
#include <vulkan/vulkan.hpp>

class PLASMA_RENDERERVULKAN_DLL plGALShaderVulkan : public plGALShader
{
public:
  /// \brief Used as input to plResourceCacheVulkan::RequestDescriptorSetLayout to create a vk::DescriptorSetLayout.
  struct DescriptorSetLayoutDesc
  {
    mutable plUInt32 m_uiHash = 0;
    plHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
    void ComputeHash();
  };

  void SetDebugName(const char* szName) const override;

  PLASMA_ALWAYS_INLINE vk::ShaderModule GetShader(plGALShaderStage::Enum stage) const;
  PLASMA_ALWAYS_INLINE plUInt32 GetSetCount() const;
  PLASMA_ALWAYS_INLINE const DescriptorSetLayoutDesc& GetDescriptorSetLayout(plUInt32 uiSet = 0) const;
  PLASMA_ALWAYS_INLINE plArrayPtr<const plShaderResourceBinding> GetBindings(plUInt32 uiSet = 0) const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  plGALShaderVulkan(const plGALShaderCreationDescription& description);
  virtual ~plGALShaderVulkan();

  virtual plResult InitPlatform(plGALDevice* pDevice) override;
  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

private:
  plHybridArray<DescriptorSetLayoutDesc, 4> m_descriptorSetLayoutDesc;
  plHybridArray<plHybridArray<plShaderResourceBinding, 16>, 4> m_SetBindings;
  vk::ShaderModule m_Shaders[plGALShaderStage::ENUM_COUNT];
};

#include <RendererVulkan/Shader/Implementation/ShaderVulkan_inl.h>
