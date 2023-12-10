
#pragma once

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/Shader/VertexDeclaration.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

class plGALVertexDeclarationVulkan : public plGALVertexDeclaration
{
public:
  PLASMA_ALWAYS_INLINE plArrayPtr<const vk::VertexInputAttributeDescription> GetAttributes() const;
  PLASMA_ALWAYS_INLINE plArrayPtr<const vk::VertexInputBindingDescription> GetBindings() const;

protected:
  friend class plGALDeviceVulkan;
  friend class plMemoryUtils;

  virtual plResult InitPlatform(plGALDevice* pDevice) override;

  virtual plResult DeInitPlatform(plGALDevice* pDevice) override;

  plGALVertexDeclarationVulkan(const plGALVertexDeclarationCreationDescription& Description);

  virtual ~plGALVertexDeclarationVulkan();

  plHybridArray<vk::VertexInputAttributeDescription, PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT> m_attributes;
  plHybridArray<vk::VertexInputBindingDescription, PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT> m_bindings;
};

#include <RendererVulkan/Shader/Implementation/VertexDeclarationVulkan_inl.h>
