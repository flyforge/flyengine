#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <RendererFoundation/Shader/ShaderUtils.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class plGALBufferVulkan;
class plGALTextureVulkan;
class plGALRenderTargetViewVulkan;
class plGALResourceViewVulkan;
class plGALUnorderedAccessViewVulkan;


/// \brief
class PLASMA_RENDERERVULKAN_DLL plImageCopyVulkan
{
public:
  plImageCopyVulkan(plGALDeviceVulkan& GALDeviceVulkan);
  ~plImageCopyVulkan();
  void Init(const plGALTextureVulkan* pSource, const plGALTextureVulkan* pTarget, plShaderUtils::plBuiltinShaderType type);

  void Copy(const plVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const plVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const plVec3U32& extends);

  static void Initialize(plGALDeviceVulkan& GALDeviceVulkan);
  static void DeInitialize(plGALDeviceVulkan& GALDeviceVulkan);

  struct RenderPassCacheKey
  {
    PLASMA_DECLARE_POD_TYPE();

    vk::Format targetFormat;
    vk::SampleCountFlagBits targetSamples;
  };

  struct FramebufferCacheKey
  {
    PLASMA_DECLARE_POD_TYPE();

    vk::RenderPass m_renderpass;
    vk::ImageView m_targetView;
    plVec3U32 m_extends;
    uint32_t m_layerCount;
  };

  struct ImageViewCacheKey
  {
    PLASMA_DECLARE_POD_TYPE();

    vk::Image m_image;
    vk::ImageSubresourceLayers m_subresourceLayers;
  };

  struct ImageViewCacheValue
  {
    PLASMA_DECLARE_POD_TYPE();

    vk::ImageSubresourceLayers m_subresourceLayers;
    vk::ImageView m_imageView;
  };

private:
  void RenderInternal(const plVec3U32& sourceOffset, const vk::ImageSubresourceLayers& sourceLayers, const plVec3U32& targetOffset, const vk::ImageSubresourceLayers& targetLayers, const plVec3U32& extends);

  static void OnBeforeImageDestroyed(plGALDeviceVulkan::OnBeforeImageDestroyedData data);


private:
  plGALDeviceVulkan& m_GALDeviceVulkan;

  // Init input
  const plGALTextureVulkan* m_pSource = nullptr;
  const plGALTextureVulkan* m_pTarget = nullptr;
  plShaderUtils::plBuiltinShaderType m_type = plShaderUtils::plBuiltinShaderType::CopyImage;

  // Init derived Vulkan objects
  vk::RenderPass m_renderPass;
  plShaderUtils::plBuiltinShader m_shader;
  plGALVertexDeclarationHandle m_hVertexDecl;
  plResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  plResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  vk::Pipeline m_pipeline;

  // Cache to keep important resources alive
  // This avoids recreating them every frame
  struct Cache
  {
    Cache(plAllocatorBase* pAllocator);
    ~Cache();

    plHashTable<plGALShaderHandle, plGALVertexDeclarationHandle> m_vertexDeclarations;
    plHashTable<RenderPassCacheKey, vk::RenderPass> m_renderPasses;
    plHashTable<ImageViewCacheKey, vk::ImageView> m_sourceImageViews;
    plHashTable<vk::Image, ImageViewCacheValue> m_imageToSourceImageViewCacheKey;
    plHashTable<ImageViewCacheKey, vk::ImageView> m_targetImageViews;
    plHashTable<vk::Image, ImageViewCacheValue> m_imageToTargetImageViewCacheKey;
    plHashTable<FramebufferCacheKey, vk::Framebuffer> m_framebuffers;
    plHashTable<plShaderUtils::plBuiltinShaderType, plShaderUtils::plBuiltinShader> m_shaders;

    plEventSubscriptionID m_onBeforeImageDeletedSubscription;
  };

  static plUniquePtr<Cache> s_cache;
};
