#include <RendererVulkan/RendererVulkanPCH.h>

#include <Foundation/Basics.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>
#include <RendererVulkan/Resources/TextureVulkan.h>
#include <RendererVulkan/Shader/ShaderVulkan.h>
#include <RendererVulkan/Shader/VertexDeclarationVulkan.h>
#include <RendererVulkan/State/StateVulkan.h>
#include <RendererVulkan/Utils/ConversionUtilsVulkan.h>

plGALDeviceVulkan* plResourceCacheVulkan::s_pDevice;
vk::Device plResourceCacheVulkan::s_device;

plHashTable<plGALRenderingSetup, vk::RenderPass, plResourceCacheVulkan::ResourceCacheHash> plResourceCacheVulkan::s_shallowRenderPasses;
plHashTable<plResourceCacheVulkan::RenderPassDesc, vk::RenderPass, plResourceCacheVulkan::ResourceCacheHash> plResourceCacheVulkan::s_renderPasses;
plHashTable<plResourceCacheVulkan::FramebufferKey, plResourceCacheVulkan::FrameBufferCache, plResourceCacheVulkan::ResourceCacheHash> plResourceCacheVulkan::s_frameBuffers;
plHashTable<plResourceCacheVulkan::PipelineLayoutDesc, vk::PipelineLayout, plResourceCacheVulkan::ResourceCacheHash> plResourceCacheVulkan::s_pipelineLayouts;
plResourceCacheVulkan::GraphicsPipelineMap plResourceCacheVulkan::s_graphicsPipelines;
plResourceCacheVulkan::ComputePipelineMap plResourceCacheVulkan::s_computePipelines;
plMap<const plRefCounted*, plHybridArray<plResourceCacheVulkan::GraphicsPipelineMap::Iterator, 1>> plResourceCacheVulkan::s_graphicsPipelineUsedBy;
plMap<const plRefCounted*, plHybridArray<plResourceCacheVulkan::ComputePipelineMap::Iterator, 1>> plResourceCacheVulkan::s_computePipelineUsedBy;

plHashTable<plGALShaderVulkan::DescriptorSetLayoutDesc, vk::DescriptorSetLayout, plResourceCacheVulkan::ResourceCacheHash> plResourceCacheVulkan::s_descriptorSetLayouts;

//#define PL_LOG_VULKAN_RESOURCES

PL_CHECK_AT_COMPILETIME(sizeof(plUInt32) == sizeof(plGALRenderTargetViewHandle));
namespace
{
  PL_ALWAYS_INLINE plStreamWriter& operator<<(plStreamWriter& Stream, const plGALRenderTargetViewHandle& Value)
  {
    Stream << reinterpret_cast<const plUInt32&>(Value);
    return Stream;
  }
} // namespace

void plResourceCacheVulkan::Initialize(plGALDeviceVulkan* pDevice, vk::Device device)
{
  s_pDevice = pDevice;
  s_device = device;
}

void plResourceCacheVulkan::DeInitialize()
{
  for (auto it : s_renderPasses)
  {
    s_device.destroyRenderPass(it.Value(), nullptr);
  }
  s_renderPasses.Clear();
  s_renderPasses.Compact();
  s_shallowRenderPasses.Clear();
  s_shallowRenderPasses.Compact();

  for (auto it : s_frameBuffers)
  {
    s_device.destroyFramebuffer(it.Value().m_frameBuffer, nullptr);
  }
  s_frameBuffers.Clear();
  s_frameBuffers.Compact();

  for (auto it : s_graphicsPipelines)
  {
    s_device.destroyPipeline(it.Value(), nullptr);
  }
  s_graphicsPipelines.Clear();
  GraphicsPipelineMap tmp;
  s_graphicsPipelines.Swap(tmp);
  s_graphicsPipelineUsedBy.Clear();
  plMap<const plRefCounted*, plHybridArray<GraphicsPipelineMap::Iterator, 1>> tmp2;
  s_graphicsPipelineUsedBy.Swap(tmp2);


  for (auto it : s_pipelineLayouts)
  {
    s_device.destroyPipelineLayout(it.Value(), nullptr);
  }
  s_pipelineLayouts.Clear();
  s_pipelineLayouts.Compact();

  for (auto it : s_descriptorSetLayouts)
  {
    s_device.destroyDescriptorSetLayout(it.Value(), nullptr);
  }
  s_descriptorSetLayouts.Clear();
  s_descriptorSetLayouts.Compact();

  s_device = nullptr;
}

void plResourceCacheVulkan::GetRenderPassDesc(const plGALRenderingSetup& renderingSetup, RenderPassDesc& out_desc)
{
  const bool bHasDepth = !renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const plUInt32 uiColorCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  out_desc.attachments.Clear();

  if (bHasDepth)
  {
    const plGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const plGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget()));

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    auto* pTex = static_cast<const plGALTextureVulkan*>(s_pDevice->GetTexture(hTexture)->GetParentResource());

    const plGALTextureCreationDescription& texDesc = pTex->GetDescription();
    plEnum<plGALResourceFormat> format = texDesc.m_Format;
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& depthAttachment = out_desc.attachments.ExpandAndGetRef();
    depthAttachment.format = formatInfo.m_format;
    depthAttachment.samples = plConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    if (renderingSetup.m_bDiscardDepth && !renderingSetup.m_bClearDepth)
    {
      depthAttachment.initialLayout = vk::ImageLayout::eUndefined;
      depthAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
    }
    else
    {
      depthAttachment.initialLayout = renderingSetup.m_bClearDepth ? vk::ImageLayout::eUndefined : vk::ImageLayout::eDepthStencilAttachmentOptimal;
      depthAttachment.loadOp = renderingSetup.m_bClearDepth ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
    }
    depthAttachment.storeOp = vk::AttachmentStoreOp::eStore;

    if (format == plGALResourceFormat::Enum::D24S8)
    {
      depthAttachment.stencilLoadOp = renderingSetup.m_bClearStencil ? vk::AttachmentLoadOp::eClear : vk::AttachmentLoadOp::eLoad;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eStore;
    }
    else
    {
      depthAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
      depthAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    }
  }

  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto colorRT = renderingSetup.m_RenderTargetSetup.GetRenderTarget(static_cast<plUInt8>(i));
    const plGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const plGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));

    auto hTexture = pRenderTargetView->GetDescription().m_hTexture;
    auto* pTex = static_cast<const plGALTextureVulkan*>(s_pDevice->GetTexture(hTexture)->GetParentResource());

    const plGALTextureCreationDescription& texDesc = pTex->GetDescription();
    plEnum<plGALResourceFormat> format = texDesc.m_Format;
    const auto& formatInfo = s_pDevice->GetFormatLookupTable().GetFormatInfo(format);

    AttachmentDesc& colorAttachment = out_desc.attachments.ExpandAndGetRef();
    colorAttachment.format = formatInfo.m_format;
    colorAttachment.samples = plConversionUtilsVulkan::GetSamples(texDesc.m_SampleCount);

    if (renderingSetup.m_bDiscardColor && !(renderingSetup.m_uiRenderTargetClearMask & (1u << i)))
    {
      colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
      colorAttachment.loadOp = vk::AttachmentLoadOp::eDontCare;
    }
    else
    {
      if (renderingSetup.m_uiRenderTargetClearMask & (1u << i))
      {
        colorAttachment.initialLayout = vk::ImageLayout::eUndefined;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eClear;
      }
      else
      {
        colorAttachment.initialLayout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttachment.loadOp = vk::AttachmentLoadOp::eLoad;
      }
    }
    colorAttachment.storeOp = vk::AttachmentStoreOp::eStore;
    colorAttachment.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    colorAttachment.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
  }
}

PL_DEFINE_AS_POD_TYPE(vk::AttachmentDescription);
PL_DEFINE_AS_POD_TYPE(vk::AttachmentReference);

vk::RenderPass plResourceCacheVulkan::RequestRenderPassInternal(const RenderPassDesc& desc)
{
  if (const vk::RenderPass* pPass = s_renderPasses.GetValue(desc))
  {
    return *pPass;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating RenderPass #{}", s_renderPasses.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  plHybridArray<vk::AttachmentDescription, 4> attachments;
  plHybridArray<vk::AttachmentReference, 1> depthAttachmentRefs;
  plHybridArray<vk::AttachmentReference, 4> colorAttachmentRefs;

  const plUInt32 uiCount = desc.attachments.GetCount();
  for (plUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc& attachmentDesc = desc.attachments[i];
    vk::AttachmentDescription& vkAttachment = attachments.ExpandAndGetRef();
    vkAttachment.format = attachmentDesc.format;
    vkAttachment.samples = attachmentDesc.samples;
    vkAttachment.loadOp = attachmentDesc.loadOp;
    vkAttachment.storeOp = attachmentDesc.storeOp;
    vkAttachment.stencilLoadOp = attachmentDesc.stencilLoadOp;
    vkAttachment.stencilStoreOp = attachmentDesc.stencilStoreOp;
    vkAttachment.initialLayout = attachmentDesc.initialLayout;

    const bool bIsDepth = plConversionUtilsVulkan::IsDepthFormat(attachmentDesc.format);
    if (bIsDepth)
    {
      vkAttachment.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
      vk::AttachmentReference& depthAttachment = depthAttachmentRefs.ExpandAndGetRef();
      depthAttachment.attachment = i;
      depthAttachment.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    }
    else
    {
      vkAttachment.finalLayout = vk::ImageLayout::eColorAttachmentOptimal;
      vk::AttachmentReference& colorAttachment = colorAttachmentRefs.ExpandAndGetRef();
      colorAttachment.attachment = i;
      colorAttachment.layout = vk::ImageLayout::eColorAttachmentOptimal;
    }
  }

  PL_ASSERT_DEV(depthAttachmentRefs.GetCount() <= 1, "There can be no more than 1 depth attachment.");
  const bool bHasColor = !colorAttachmentRefs.IsEmpty();
  const bool bHasDepth = !depthAttachmentRefs.IsEmpty();
  vk::SubpassDescription subpass;
  subpass.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
  subpass.colorAttachmentCount = colorAttachmentRefs.GetCount();
  subpass.pColorAttachments = bHasColor ? colorAttachmentRefs.GetData() : nullptr;
  subpass.pDepthStencilAttachment = bHasDepth ? depthAttachmentRefs.GetData() : nullptr;

  vk::SubpassDependency dependency;
  dependency.dstSubpass = 0;
  if (bHasColor)
    dependency.dstAccessMask |= vk::AccessFlagBits::eColorAttachmentWrite;

  if (bHasDepth)
    dependency.dstAccessMask |= vk::AccessFlagBits::eDepthStencilAttachmentWrite;

  dependency.dstStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.srcAccessMask = {};
  dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput | vk::PipelineStageFlagBits::eEarlyFragmentTests;

  vk::RenderPassCreateInfo renderPassCreateInfo;
  renderPassCreateInfo.attachmentCount = attachments.GetCount();
  renderPassCreateInfo.pAttachments = attachments.GetData();
  renderPassCreateInfo.subpassCount = 1;
  renderPassCreateInfo.pSubpasses = &subpass;
  renderPassCreateInfo.dependencyCount = 1;
  renderPassCreateInfo.pDependencies = &dependency;

  vk::RenderPass renderPass;
  VK_LOG_ERROR(s_device.createRenderPass(&renderPassCreateInfo, nullptr, &renderPass));

  s_renderPasses.Insert(desc, renderPass);
  return renderPass;
}

vk::RenderPass plResourceCacheVulkan::RequestRenderPass(const plGALRenderingSetup& renderingSetup)
{
  if (const vk::RenderPass* pPass = s_shallowRenderPasses.GetValue(renderingSetup))
  {
    return *pPass;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Redirecting shallow RenderPass #{}", s_shallowRenderPasses.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  RenderPassDesc renderPassDesc;
  GetRenderPassDesc(renderingSetup, renderPassDesc);
  vk::RenderPass renderPass = RequestRenderPassInternal(renderPassDesc);

  s_shallowRenderPasses.Insert(renderingSetup, renderPass);
  return renderPass;
}

void plResourceCacheVulkan::GetFrameBufferDesc(vk::RenderPass renderPass, const plGALRenderTargetSetup& renderTargetSetup, FramebufferDesc& out_desc)
{
  const bool bHasDepth = !renderTargetSetup.GetDepthStencilTarget().IsInvalidated();
  const plUInt32 uiColorCount = renderTargetSetup.GetRenderTargetCount();

  out_desc.renderPass = renderPass;
  if (bHasDepth)
  {
    const plGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const plGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(renderTargetSetup.GetDepthStencilTarget()));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    const plGALTextureVulkan* pTex = static_cast<const plGALTextureVulkan*>(pRenderTargetView->GetTexture()->GetParentResource());
    const plGALTextureCreationDescription& texDesc = pTex->GetDescription();
    vk::Extent3D extend = pTex->GetMipLevelSize(pRenderTargetView->GetDescription().m_uiMipLevel);
    out_desc.m_msaa = texDesc.m_SampleCount;
    out_desc.m_size = {extend.width, extend.height};
    out_desc.layers = pRenderTargetView->GetDescription().m_uiSliceCount;
  }
  for (size_t i = 0; i < uiColorCount; i++)
  {
    auto colorRT = renderTargetSetup.GetRenderTarget(static_cast<plUInt8>(i));
    const plGALRenderTargetViewVulkan* pRenderTargetView = static_cast<const plGALRenderTargetViewVulkan*>(s_pDevice->GetRenderTargetView(colorRT));
    out_desc.attachments.PushBack(pRenderTargetView->GetImageView());

    const plGALTextureVulkan* pTex = static_cast<const plGALTextureVulkan*>(pRenderTargetView->GetTexture()->GetParentResource());
    const plGALTextureCreationDescription& texDesc = pTex->GetDescription();
    vk::Extent3D extend = pTex->GetMipLevelSize(pRenderTargetView->GetDescription().m_uiMipLevel);
    out_desc.m_msaa = texDesc.m_SampleCount;
    out_desc.m_size = {extend.width, extend.height};
    out_desc.layers = pRenderTargetView->GetDescription().m_uiSliceCount;
  }

  // In some places rendering is started with an empty plGALRenderTargetSetup just to be able to run GPU commands.
  // An empty size is invalid in Vulkan so we just set it so 1,1.
  if (out_desc.m_size == plSizeU32(0, 0))
  {
    out_desc.m_size = {1, 1};
    out_desc.layers = 1;
  }
}

vk::Framebuffer plResourceCacheVulkan::RequestFrameBuffer(vk::RenderPass renderPass, const plGALRenderTargetSetup& renderTargetSetup, plSizeU32& out_Size, plEnum<plGALMSAASampleCount>& out_msaa, plUInt32& out_uiLayers)
{
  FramebufferKey key;
  key.m_renderPass = renderPass;
  key.m_renderTargetSetup = renderTargetSetup;

  if (const FrameBufferCache* pFrameBuffer = s_frameBuffers.GetValue(key))
  {
    out_Size = pFrameBuffer->m_size;
    out_msaa = pFrameBuffer->m_msaa;
    return pFrameBuffer->m_frameBuffer;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating FrameBuffer #{}", s_frameBuffers.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  FramebufferDesc desc;
  GetFrameBufferDesc(renderPass, renderTargetSetup, desc);

  vk::FramebufferCreateInfo framebufferInfo;
  framebufferInfo.renderPass = desc.renderPass;
  framebufferInfo.attachmentCount = desc.attachments.GetCount();
  framebufferInfo.pAttachments = desc.attachments.GetData();
  framebufferInfo.width = desc.m_size.width;
  framebufferInfo.height = desc.m_size.height;
  framebufferInfo.layers = desc.layers;

  FrameBufferCache cache;
  cache.m_size = desc.m_size;
  cache.m_msaa = desc.m_msaa;
  cache.m_layers = desc.layers;
  VK_LOG_ERROR(s_device.createFramebuffer(&framebufferInfo, nullptr, &cache.m_frameBuffer));

  s_frameBuffers.Insert(key, cache);
  out_Size = cache.m_size;
  out_msaa = cache.m_msaa;
  out_uiLayers = cache.m_layers;
  return cache.m_frameBuffer;
}

vk::PipelineLayout plResourceCacheVulkan::RequestPipelineLayout(const PipelineLayoutDesc& desc)
{
  if (const vk::PipelineLayout* pPipelineLayout = s_pipelineLayouts.GetValue(desc))
  {
    return *pPipelineLayout;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating Pipeline Layout #{}", s_pipelineLayouts.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  vk::PipelineLayoutCreateInfo layoutInfo;
  layoutInfo.setLayoutCount = desc.m_layout.GetCount();
  layoutInfo.pSetLayouts = desc.m_layout.GetData();
  if (desc.m_pushConstants.size != 0)
  {
    layoutInfo.pushConstantRangeCount = 1;
    layoutInfo.pPushConstantRanges = &desc.m_pushConstants;
  }

  vk::PipelineLayout layout;
  VK_ASSERT_DEBUG(s_device.createPipelineLayout(&layoutInfo, nullptr, &layout));

  s_pipelineLayouts.Insert(desc, layout);
  return layout;
}

vk::Pipeline plResourceCacheVulkan::RequestGraphicsPipeline(const GraphicsPipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_graphicsPipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating Graphics Pipeline #{}", s_graphicsPipelines.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  vk::PipelineVertexInputStateCreateInfo vertex_input;
  plHybridArray<vk::VertexInputBindingDescription, PL_GAL_MAX_VERTEX_BUFFER_COUNT> bindings;
  if (desc.m_pCurrentVertexDecl)
  {
    bindings = desc.m_pCurrentVertexDecl->GetBindings();
    for (plUInt32 i = 0; i < bindings.GetCount(); i++)
    {
      bindings[i].stride = desc.m_VertexBufferStrides[bindings[i].binding];
    }
    vertex_input.vertexAttributeDescriptionCount = desc.m_pCurrentVertexDecl->GetAttributes().GetCount();
    vertex_input.pVertexAttributeDescriptions = desc.m_pCurrentVertexDecl->GetAttributes().GetPtr();
    vertex_input.vertexBindingDescriptionCount = bindings.GetCount();
    vertex_input.pVertexBindingDescriptions = bindings.GetData();
  }

  vk::PipelineInputAssemblyStateCreateInfo input_assembly;
  input_assembly.topology = plConversionUtilsVulkan::GetPrimitiveTopology(desc.m_topology);

  const bool bTessellation = desc.m_pCurrentShader->GetShader(plGALShaderStage::HullShader) != nullptr;
  if (bTessellation)
  {
    // Tessellation shaders always need to use patch list as the topology.
    input_assembly.topology = vk::PrimitiveTopology::ePatchList;
  }

  // Specify rasterization state.
  const vk::PipelineRasterizationStateCreateInfo* raster = desc.m_pCurrentRasterizerState->GetRasterizerState();

  // Our attachment will write to all color channels
  vk::PipelineColorBlendStateCreateInfo blend = *desc.m_pCurrentBlendState->GetBlendState();
  blend.attachmentCount = desc.m_uiAttachmentCount;

  // We will have one viewport and scissor box.
  vk::PipelineViewportStateCreateInfo viewport;
  viewport.viewportCount = 1;
  viewport.scissorCount = 1;

  // Depth Testing
  const vk::PipelineDepthStencilStateCreateInfo* depth_stencil = desc.m_pCurrentDepthStencilState->GetDepthStencilState();

  // Multisampling.
  vk::PipelineMultisampleStateCreateInfo multisample;
  multisample.rasterizationSamples = plConversionUtilsVulkan::GetSamples(desc.m_msaa);
  if (multisample.rasterizationSamples != vk::SampleCountFlagBits::e1 && desc.m_pCurrentBlendState->GetDescription().m_bAlphaToCoverage)
  {
    multisample.alphaToCoverageEnable = true;
  }

  // Specify that these states will be dynamic, i.e. not part of pipeline state object.
  plHybridArray<vk::DynamicState, 2> dynamics;
  dynamics.PushBack(vk::DynamicState::eViewport);
  dynamics.PushBack(vk::DynamicState::eScissor);

  vk::PipelineDynamicStateCreateInfo dynamic;
  dynamic.pDynamicStates = dynamics.GetData();
  dynamic.dynamicStateCount = dynamics.GetCount();

  // Load our SPIR-V shaders.
  plHybridArray<vk::PipelineShaderStageCreateInfo, 6> shader_stages;
  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; i++)
  {
    if (vk::ShaderModule shader = desc.m_pCurrentShader->GetShader((plGALShaderStage::Enum)i))
    {
      vk::PipelineShaderStageCreateInfo& stage = shader_stages.ExpandAndGetRef();
      stage.stage = plConversionUtilsVulkan::GetShaderStage((plGALShaderStage::Enum)i);
      stage.module = shader;
      stage.pName = "main";
    }
  }

  vk::PipelineTessellationStateCreateInfo tessellationInfo;
  if (bTessellation)
  {
    tessellationInfo.patchControlPoints = desc.m_pCurrentShader->GetDescription().m_ByteCodes[plGALShaderStage::HullShader]->m_uiTessellationPatchControlPoints;
  }

  vk::GraphicsPipelineCreateInfo pipe;
  pipe.renderPass = desc.m_renderPass;
  pipe.layout = desc.m_layout;
  pipe.stageCount = shader_stages.GetCount();
  pipe.pStages = shader_stages.GetData();
  pipe.pVertexInputState = &vertex_input;
  pipe.pInputAssemblyState = &input_assembly;
  pipe.pRasterizationState = raster;
  pipe.pColorBlendState = &blend;
  pipe.pMultisampleState = &multisample;
  pipe.pViewportState = &viewport;
  pipe.pDepthStencilState = depth_stencil;
  pipe.pDynamicState = &dynamic;
  if (bTessellation)
    pipe.pTessellationState = &tessellationInfo;

  vk::Pipeline pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createGraphicsPipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_graphicsPipelines.Insert(desc, pipeline);
  {
    s_graphicsPipelineUsedBy[desc.m_pCurrentRasterizerState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentBlendState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentDepthStencilState].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
    s_graphicsPipelineUsedBy[desc.m_pCurrentVertexDecl].PushBack(it);
  }

  return pipeline;
}

vk::Pipeline plResourceCacheVulkan::RequestComputePipeline(const ComputePipelineDesc& desc)
{
  if (const vk::Pipeline* pPipeline = s_computePipelines.GetValue(desc))
  {
    return *pPipeline;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating Compute Pipeline #{}", s_computePipelines.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  vk::ComputePipelineCreateInfo pipe;
  pipe.layout = desc.m_layout;
  {
    vk::ShaderModule shader = desc.m_pCurrentShader->GetShader(plGALShaderStage::ComputeShader);
    pipe.stage.stage = plConversionUtilsVulkan::GetShaderStage(plGALShaderStage::ComputeShader);
    pipe.stage.module = shader;
    pipe.stage.pName = "main";
  }

  vk::Pipeline pipeline;
  vk::PipelineCache cache;
  VK_ASSERT_DEBUG(s_device.createComputePipelines(cache, 1, &pipe, nullptr, &pipeline));

  auto it = s_computePipelines.Insert(desc, pipeline);
  {
    s_computePipelineUsedBy[desc.m_pCurrentShader].PushBack(it);
  }

  return pipeline;
}

vk::DescriptorSetLayout plResourceCacheVulkan::RequestDescriptorSetLayout(const plGALShaderVulkan::DescriptorSetLayoutDesc& desc)
{
  if (const vk::DescriptorSetLayout* pLayout = s_descriptorSetLayouts.GetValue(desc))
  {
    return *pLayout;
  }

#ifdef PL_LOG_VULKAN_RESOURCES
  plLog::Info("Creating Descriptor Set Layout #{}", s_descriptorSetLayouts.GetCount());
#endif // PL_LOG_VULKAN_RESOURCES

  vk::DescriptorSetLayoutCreateInfo descriptorSetLayout;
  descriptorSetLayout.bindingCount = desc.m_bindings.GetCount();
  descriptorSetLayout.pBindings = desc.m_bindings.GetData();

  vk::DescriptorSetLayout layout;
  VK_ASSERT_DEBUG(s_device.createDescriptorSetLayout(&descriptorSetLayout, nullptr, &layout));

  s_descriptorSetLayouts.Insert(desc, layout);
  return layout;
}

void plResourceCacheVulkan::ResourceDeleted(const plRefCounted* pResource)
{
  auto it = s_graphicsPipelineUsedBy.Find(pResource);
  if (it.IsValid())
  {
    const auto& itArray = it.Value();
    for (GraphicsPipelineMap::Iterator it2 : itArray)
    {
      s_pDevice->DeleteLater(it2.Value());

      const GraphicsPipelineDesc& desc = it2.Key();
      plArrayPtr<const plRefCounted*> resources((const plRefCounted**)&desc.m_pCurrentRasterizerState, 5);
      for (const plRefCounted* pResource2 : resources)
      {
        if (pResource2 != pResource)
        {
          s_graphicsPipelineUsedBy[pResource2].RemoveAndSwap(it2);
        }
      }

      s_graphicsPipelines.Remove(it2);
    }

    s_graphicsPipelineUsedBy.Remove(it);
  }
}

void plResourceCacheVulkan::ShaderDeleted(const plGALShaderVulkan* pShader)
{
  if (pShader->GetDescription().HasByteCodeForStage(plGALShaderStage::ComputeShader))
  {
    auto it = s_computePipelineUsedBy.Find(pShader);
    if (it.IsValid())
    {
      const auto& itArray = it.Value();
      for (ComputePipelineMap::Iterator it2 : itArray)
      {
        s_pDevice->DeleteLater(it2.Value());
        s_computePipelines.Remove(it2);
      }

      s_computePipelineUsedBy.Remove(it);
    }
  }
  else
  {
    ResourceDeleted(pShader);
  }
}

plUInt32 plResourceCacheVulkan::ResourceCacheHash::Hash(const RenderPassDesc& renderingSetup)
{
  plHashStreamWriter32 writer;
  for (const auto& attachment : renderingSetup.attachments)
  {
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.format);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.samples);
    writer << plConversionUtilsVulkan::GetUnderlyingFlagsValue(attachment.usage);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.initialLayout);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.loadOp);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.storeOp);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.stencilLoadOp);
    writer << plConversionUtilsVulkan::GetUnderlyingValue(attachment.stencilStoreOp);
  }
  return writer.GetHashValue();
}


plUInt32 plResourceCacheVulkan::ResourceCacheHash::Hash(const plGALRenderingSetup& renderingSetup)
{
  plHashStreamWriter32 writer;
  writer << renderingSetup.m_RenderTargetSetup.GetDepthStencilTarget();
  const plUInt8 uiCount = renderingSetup.m_RenderTargetSetup.GetRenderTargetCount();
  for (plUInt8 i = 0; i < uiCount; ++i)
  {
    writer << renderingSetup.m_RenderTargetSetup.GetRenderTarget(i);
  }
  writer << renderingSetup.m_ClearColor;
  writer << renderingSetup.m_uiRenderTargetClearMask;
  writer << renderingSetup.m_fDepthClear;
  writer << renderingSetup.m_uiStencilClear;
  writer << renderingSetup.m_bClearDepth;
  writer << renderingSetup.m_bClearStencil;
  writer << renderingSetup.m_bDiscardColor;
  writer << renderingSetup.m_bDiscardDepth;
  return writer.GetHashValue();
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const RenderPassDesc& a, const RenderPassDesc& b)
{
  const plUInt32 uiCount = a.attachments.GetCount();
  if (uiCount != b.attachments.GetCount())
    return false;

  for (plUInt32 i = 0; i < uiCount; i++)
  {
    const AttachmentDesc& aE = a.attachments[i];
    const AttachmentDesc& bE = b.attachments[i];

    if (aE.format != bE.format || aE.samples != bE.samples || aE.usage != bE.usage || aE.initialLayout != bE.initialLayout || aE.loadOp != bE.loadOp || aE.storeOp != bE.storeOp || aE.stencilLoadOp != bE.stencilLoadOp || aE.stencilStoreOp != bE.stencilStoreOp)
      return false;
  }

  return true;
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const plGALRenderingSetup& a, const plGALRenderingSetup& b)
{
  return a == b;
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const plGALShaderVulkan::DescriptorSetLayoutDesc& a, const plGALShaderVulkan::DescriptorSetLayoutDesc& b)
{
  const plUInt32 uiCount = a.m_bindings.GetCount();
  if (uiCount != b.m_bindings.GetCount())
    return false;

  for (plUInt32 i = 0; i < uiCount; i++)
  {
    const vk::DescriptorSetLayoutBinding& aB = a.m_bindings[i];
    const vk::DescriptorSetLayoutBinding& bB = b.m_bindings[i];
    if (aB.binding != bB.binding || aB.descriptorType != bB.descriptorType || aB.descriptorCount != bB.descriptorCount || aB.stageFlags != bB.stageFlags || aB.pImmutableSamplers != bB.pImmutableSamplers)
      return false;
  }
  return true;
}

bool plResourceCacheVulkan::ResourceCacheHash::Less(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
#define LESS_CHECK(member)  \
  if (a.member != b.member) \
    return a.member < b.member;

  LESS_CHECK(m_renderPass);
  LESS_CHECK(m_layout);
  LESS_CHECK(m_topology);
  LESS_CHECK(m_msaa);
  LESS_CHECK(m_uiAttachmentCount);
  LESS_CHECK(m_pCurrentRasterizerState);
  LESS_CHECK(m_pCurrentBlendState);
  LESS_CHECK(m_pCurrentDepthStencilState);
  LESS_CHECK(m_pCurrentShader);
  LESS_CHECK(m_pCurrentVertexDecl);

  for (plUInt32 i = 0; i < PL_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (a.m_VertexBufferStrides[i] != b.m_VertexBufferStrides[i])
      return a.m_VertexBufferStrides[i] < b.m_VertexBufferStrides[i];
  }
  return false;

#undef LESS_CHECK
}

plUInt32 plResourceCacheVulkan::ResourceCacheHash::Hash(const FramebufferKey& key)
{
  plHashStreamWriter32 writer;
  writer << key.m_renderPass;
  writer << key.m_renderTargetSetup.GetDepthStencilTarget();
  const plUInt8 uiCount = key.m_renderTargetSetup.GetRenderTargetCount();
  for (plUInt8 i = 0; i < uiCount; ++i)
  {
    writer << key.m_renderTargetSetup.GetRenderTarget(i);
  }
  return writer.GetHashValue();
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const FramebufferKey& a, const FramebufferKey& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_renderTargetSetup == b.m_renderTargetSetup;
}

plUInt32 plResourceCacheVulkan::ResourceCacheHash::Hash(const PipelineLayoutDesc& desc)
{
  plHashStreamWriter32 writer;
  const plUInt32 uiCount = desc.m_layout.GetCount();
  writer << uiCount;
  for (plUInt32 i = 0; i < uiCount; ++i)
  {
    writer << desc.m_layout[i];
  }
  writer << desc.m_pushConstants.offset;
  writer << desc.m_pushConstants.size;
  writer << plConversionUtilsVulkan::GetUnderlyingFlagsValue(desc.m_pushConstants.stageFlags);
  return writer.GetHashValue();
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const PipelineLayoutDesc& a, const PipelineLayoutDesc& b)
{
  if (a.m_layout.GetCount() != b.m_layout.GetCount())
    return false;

  const plUInt32 uiCount = a.m_layout.GetCount();
  for (plUInt32 i = 0; i < uiCount; ++i)
  {
    if (a.m_layout[i] != b.m_layout[i])
      return false;
  }
  return a.m_pushConstants == b.m_pushConstants;
}

plUInt32 plResourceCacheVulkan::ResourceCacheHash::Hash(const GraphicsPipelineDesc& desc)
{
  plHashStreamWriter32 writer;
  writer << desc.m_renderPass;
  writer << desc.m_layout;
  writer << desc.m_topology;
  writer << desc.m_msaa;
  writer << desc.m_uiAttachmentCount;
  writer << desc.m_pCurrentRasterizerState;
  writer << desc.m_pCurrentBlendState;
  writer << desc.m_pCurrentDepthStencilState;
  writer << desc.m_pCurrentShader;
  writer << desc.m_pCurrentVertexDecl;
  writer.WriteArray(desc.m_VertexBufferStrides).IgnoreResult();
  return writer.GetHashValue();
}

bool ArraysEqual(const plUInt32 (&a)[PL_GAL_MAX_VERTEX_BUFFER_COUNT], const plUInt32 (&b)[PL_GAL_MAX_VERTEX_BUFFER_COUNT])
{
  for (plUInt32 i = 0; i < PL_GAL_MAX_VERTEX_BUFFER_COUNT; i++)
  {
    if (a[i] != b[i])
      return false;
  }
  return true;
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const GraphicsPipelineDesc& a, const GraphicsPipelineDesc& b)
{
  return a.m_renderPass == b.m_renderPass && a.m_layout == b.m_layout && a.m_topology == b.m_topology && a.m_msaa == b.m_msaa && a.m_uiAttachmentCount == b.m_uiAttachmentCount && a.m_pCurrentRasterizerState == b.m_pCurrentRasterizerState && a.m_pCurrentBlendState == b.m_pCurrentBlendState && a.m_pCurrentDepthStencilState == b.m_pCurrentDepthStencilState && a.m_pCurrentShader == b.m_pCurrentShader && a.m_pCurrentVertexDecl == b.m_pCurrentVertexDecl && ArraysEqual(a.m_VertexBufferStrides, b.m_VertexBufferStrides);
}

bool plResourceCacheVulkan::ResourceCacheHash::Less(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  if (a.m_layout != b.m_layout)
    return a.m_layout < b.m_layout;
  return a.m_pCurrentShader < b.m_pCurrentShader;
}

bool plResourceCacheVulkan::ResourceCacheHash::Equal(const ComputePipelineDesc& a, const ComputePipelineDesc& b)
{
  return a.m_layout == b.m_layout && a.m_pCurrentShader == b.m_pCurrentShader;
}
