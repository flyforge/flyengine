#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Algorithm/HashStream.h>
#include <Foundation/Math/Size.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Device/DeviceVulkan.h>
#include <RendererVulkan/Resources/RenderTargetViewVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>

#include <vulkan/vulkan.hpp>

class plGALRasterizerStateVulkan;
class plGALBlendStateVulkan;
class plGALDepthStencilStateVulkan;
class plGALShaderVulkan;
class plGALVertexDeclarationVulkan;
class plRefCounted;

PL_DEFINE_AS_POD_TYPE(vk::DynamicState);

/// \brief Creates and caches persistent Vulkan resources. Resources are never freed until the device is shut down.
class PL_RENDERERVULKAN_DLL plResourceCacheVulkan
{
public:
  static void Initialize(plGALDeviceVulkan* pDevice, vk::Device device);
  static void DeInitialize();

  static vk::RenderPass RequestRenderPass(const plGALRenderingSetup& renderingSetup);
  static vk::Framebuffer RequestFrameBuffer(vk::RenderPass renderPass, const plGALRenderTargetSetup& renderTargetSetup, plSizeU32& out_Size, plEnum<plGALMSAASampleCount>& out_msaa, plUInt32& out_uiLayers);

  struct PipelineLayoutDesc
  {
    plHybridArray<vk::DescriptorSetLayout, 4> m_layout;
    vk::PushConstantRange m_pushConstants;
  };

  struct GraphicsPipelineDesc
  {
    PL_DECLARE_POD_TYPE();
    vk::RenderPass m_renderPass; // Created from plGALRenderingSetup
    vk::PipelineLayout m_layout; // Created from shader
    plEnum<plGALPrimitiveTopology> m_topology;
    plEnum<plGALMSAASampleCount> m_msaa;
    plUInt8 m_uiAttachmentCount = 0; // DX12 requires format list for RT and DT
    const plGALRasterizerStateVulkan* m_pCurrentRasterizerState = nullptr;
    const plGALBlendStateVulkan* m_pCurrentBlendState = nullptr;
    const plGALDepthStencilStateVulkan* m_pCurrentDepthStencilState = nullptr;
    const plGALShaderVulkan* m_pCurrentShader = nullptr;
    const plGALVertexDeclarationVulkan* m_pCurrentVertexDecl = nullptr;
    plUInt32 m_VertexBufferStrides[PL_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  };

  struct ComputePipelineDesc
  {
    PL_DECLARE_POD_TYPE();
    vk::PipelineLayout m_layout;
    const plGALShaderVulkan* m_pCurrentShader = nullptr;
  };

  static vk::PipelineLayout RequestPipelineLayout(const PipelineLayoutDesc& desc);
  static vk::Pipeline RequestGraphicsPipeline(const GraphicsPipelineDesc& desc);
  static vk::Pipeline RequestComputePipeline(const ComputePipelineDesc& desc);

  struct DescriptorSetLayoutDesc
  {
    mutable plUInt32 m_uiHash = 0;
    plHybridArray<vk::DescriptorSetLayoutBinding, 6> m_bindings;
  };
  static vk::DescriptorSetLayout RequestDescriptorSetLayout(const plGALShaderVulkan::DescriptorSetLayoutDesc& desc);

  /// \brief Invalidates any caches that use this resource. Basically all pointer types in GraphicsPipelineDesc except for plGALShaderVulkan.
  static void ResourceDeleted(const plRefCounted* pResource);
  /// \brief Invalidates any caches that use this shader resource.
  static void ShaderDeleted(const plGALShaderVulkan* pShader);

private:
  struct FramebufferKey
  {
    vk::RenderPass m_renderPass;
    plGALRenderTargetSetup m_renderTargetSetup;
  };

  /// \brief Hashable version without pointers of vk::FramebufferCreateInfo
  struct FramebufferDesc
  {
    VkRenderPass renderPass;
    plSizeU32 m_size = {0, 0};
    uint32_t layers = 1;
    plHybridArray<vk::ImageView, PL_GAL_MAX_RENDERTARGET_COUNT + 1> attachments;
    plEnum<plGALMSAASampleCount> m_msaa;
  };

  /// \brief Hashable version without pointers or redundant data of vk::AttachmentDescription
  struct AttachmentDesc
  {
    PL_DECLARE_POD_TYPE();
    vk::Format format = vk::Format::eUndefined;
    vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    // Not set at all right now
    vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eSampled;
    // Not set at all right now
    vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined;
    // No support for eDontCare in PL right now
    vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore;
    vk::AttachmentLoadOp stencilLoadOp = vk::AttachmentLoadOp::eClear;
    vk::AttachmentStoreOp stencilStoreOp = vk::AttachmentStoreOp::eStore;
  };

  /// \brief Hashable version without pointers of vk::RenderPassCreateInfo
  struct RenderPassDesc
  {
    plHybridArray<AttachmentDesc, PL_GAL_MAX_RENDERTARGET_COUNT> attachments;
  };

  struct ResourceCacheHash
  {
    static plUInt32 Hash(const RenderPassDesc& renderingSetup);
    static bool Equal(const RenderPassDesc& a, const RenderPassDesc& b);

    static plUInt32 Hash(const plGALRenderingSetup& renderingSetup);
    static bool Equal(const plGALRenderingSetup& a, const plGALRenderingSetup& b);

    static plUInt32 Hash(const FramebufferKey& renderTargetSetup);
    static bool Equal(const FramebufferKey& a, const FramebufferKey& b);

    static plUInt32 Hash(const PipelineLayoutDesc& desc);
    static bool Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b);

    static bool Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);
    static plUInt32 Hash(const GraphicsPipelineDesc& desc);
    static bool Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b);

    static bool Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b);
    static bool Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b);

    static plUInt32 Hash(const plGALShaderVulkan::DescriptorSetLayoutDesc& desc) { return desc.m_uiHash; }
    static bool Equal(const plGALShaderVulkan::DescriptorSetLayoutDesc& a, const plGALShaderVulkan::DescriptorSetLayoutDesc& b);
  };

  struct FrameBufferCache
  {
    vk::Framebuffer m_frameBuffer;
    plSizeU32 m_size;
    plEnum<plGALMSAASampleCount> m_msaa;
    plUInt32 m_layers = 0;
    PL_DECLARE_POD_TYPE();
  };

  static vk::RenderPass RequestRenderPassInternal(const RenderPassDesc& desc);
  static void GetRenderPassDesc(const plGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc);
  static void GetFrameBufferDesc(vk::RenderPass renderPass, const plGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc);

public:
  using GraphicsPipelineMap = plMap<plResourceCacheVulkan::GraphicsPipelineDesc, vk::Pipeline, plResourceCacheVulkan::ResourceCacheHash>;
  using ComputePipelineMap = plMap<plResourceCacheVulkan::ComputePipelineDesc, vk::Pipeline, plResourceCacheVulkan::ResourceCacheHash>;


private:
  static plGALDeviceVulkan* s_pDevice;
  static vk::Device s_device;
  // We have a N to 1 mapping for plGALRenderingSetup to vk::RenderPass as multiple plGALRenderingSetup can share the same RenderPassDesc.
  // Thus, we have a two stage resolve to the vk::RenderPass. If a plGALRenderingSetup is not present in s_shallowRenderPasses we create the RenderPassDesc which has a 1 to 1 relationship with vk::RenderPass and look that one up in s_renderPasses. Finally we add the entry to s_shallowRenderPasses to make sure a shallow lookup will work on the next query.
  static plHashTable<plGALRenderingSetup, vk::RenderPass, ResourceCacheHash> s_shallowRenderPasses; //#TODO_VULKAN cache invalidation
  static plHashTable<RenderPassDesc, vk::RenderPass, ResourceCacheHash> s_renderPasses;
  static plHashTable<FramebufferKey, FrameBufferCache, ResourceCacheHash> s_frameBuffers; //#TODO_VULKAN cache invalidation

  static plHashTable<PipelineLayoutDesc, vk::PipelineLayout, ResourceCacheHash> s_pipelineLayouts;
  static GraphicsPipelineMap s_graphicsPipelines;
  static ComputePipelineMap s_computePipelines;
  static plMap<const plRefCounted*, plHybridArray<GraphicsPipelineMap::Iterator, 1>> s_graphicsPipelineUsedBy;
  static plMap<const plRefCounted*, plHybridArray<ComputePipelineMap::Iterator, 1>> s_computePipelineUsedBy;

  static plHashTable<plGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, ResourceCacheHash> s_descriptorSetLayouts;
};
