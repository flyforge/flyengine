
#pragma once

#include <Foundation/System/PlatformFeatures.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

using plGALFormatLookupEntryVulkan = plGALFormatLookupEntry<vk::Format, (vk::Format)0>;
using plGALFormatLookupTableVulkan = plGALFormatLookupTable<plGALFormatLookupEntryVulkan>;

class plGALBufferVulkan;
class plGALTextureVulkan;
class plGALPassVulkan;
class plPipelineBarrierVulkan;
class plCommandBufferPoolVulkan;
class plStagingBufferPoolVulkan;
class plQueryPoolVulkan;
class plInitContextVulkan;

/// \brief The Vulkan device implementation of the graphics abstraction layer.
class PLASMA_RENDERERVULKAN_DLL plGALDeviceVulkan : public plGALDevice
{
private:
  friend plInternal::NewInstance<plGALDevice> CreateVulkanDevice(plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& Description);
  plGALDeviceVulkan(const plGALDeviceCreationDescription& Description);

public:
  virtual ~plGALDeviceVulkan();

public:
  struct PendingDeletion
  {
    PLASMA_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    union
    {
      plVulkanAllocation m_allocation;
      void* m_pContext;
    };
  };

  struct ReclaimResource
  {
    PLASMA_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    void* m_pContext = nullptr;
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif PLASMA_ENABLED(PLASMA_SUPPORTS_GLFW)
#else
#  error "Vulkan Platform not supported"
#endif

    bool m_bDebugUtils = false;
    PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;

    bool m_bDeviceSwapChain = false;
    bool m_bShaderViewportIndexLayer = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool m_bBorderColorFloat = false;
  };

  struct Queue
  {
    vk::Queue m_queue;
    plUInt32 m_uiQueueFamily = -1;
    plUInt32 m_uiQueueIndex = 0;
  };

  plUInt64 GetCurrentFrame() const { return m_uiFrameCounter; }
  plUInt64 GetSafeFrame() const { return m_uiSafeFrame; }

  vk::Instance GetVulkanInstance() const;
  vk::Device GetVulkanDevice() const;
  const Queue& GetGraphicsQueue() const;
  const Queue& GetTransferQueue() const;

  vk::PhysicalDevice GetVulkanPhysicalDevice() const;
  const vk::PhysicalDeviceProperties& GetPhysicalDeviceProperties() const { return m_properties; }
  const Extensions& GetExtensions() const { return m_extensions; }
  vk::PipelineStageFlags GetSupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();
  plPipelineBarrierVulkan& GetCurrentPipelineBarrier();
  plQueryPoolVulkan& GetQueryPool() const;
  plStagingBufferPoolVulkan& GetStagingBufferPool() const;
  plInitContextVulkan& GetInitContext() const;
  plProxyAllocator& GetAllocator();

  plGALTextureHandle CreateTextureInternal(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, vk::Format OverrideFormat, bool bLinearCPU = false);
  plGALBufferHandle CreateBufferInternal(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData, bool bCPU = false);

  const plGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  plInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(vk::Semaphore waitSemaphore, vk::PipelineStageFlags waitStage, vk::Semaphore signalSemaphore);

  void DeleteLater(const PendingDeletion& deletion);

  template <typename T>
  void DeleteLater(T& object, plVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, allocation});
    }
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, void* pContext)
  {
    if (object)
    {
      PendingDeletion del = {object.objectType, (void*)object, nullptr};
      del.m_pContext = pContext;
      DeleteLater(static_cast<const PendingDeletion&>(del));
    }
    object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLater({object.objectType, (void*)object, nullptr});
    }
    object = nullptr;
  }

  void ReclaimLater(const ReclaimResource& reclaim);

  template <typename T>
  void ReclaimLater(T& object, void* pContext = nullptr)
  {
    ReclaimLater({object.objectType, (void*)object, pContext});
    object = nullptr;
  }

  void SetDebugName(const vk::DebugUtilsObjectNameInfoEXT& info, plVulkanAllocation allocation = nullptr);

  template <typename T>
  void SetDebugName(const char* szName, T& object, plVulkanAllocation allocation = nullptr)
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (object)
    {
      vk::DebugUtilsObjectNameInfoEXT nameInfo;
      nameInfo.objectType = object.objectType;
      nameInfo.objectHandle = (uint64_t) static_cast<typename T::NativeType>(object);
      nameInfo.pObjectName = szName;

      SetDebugName(nameInfo, allocation);
    }
#endif
  }

  void ReportLiveGpuObjects();

  static void UploadBufferStaging(plStagingBufferPoolVulkan* pStagingBufferPool, plPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const plGALBufferVulkan* pBuffer, plArrayPtr<const plUInt8> pInitialData, vk::DeviceSize dstOffset = 0);
  static void UploadTextureStaging(plStagingBufferPoolVulkan* pStagingBufferPool, plPipelineBarrierVulkan* pPipelineBarrier, vk::CommandBuffer commandBuffer, const plGALTextureVulkan* pTexture, const vk::ImageSubresourceLayers& subResource, const plGALSystemMemoryDescription& data);

  struct OnBeforeImageDestroyedData
  {
    vk::Image image;
    plGALDeviceVulkan& GALDeviceVulkan;
  };
  plEvent<OnBeforeImageDestroyedData> OnBeforeImageDestroyed;


  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  vk::Result SelectInstanceExtensions(plHybridArray<const char*, 6>& extensions);
  vk::Result SelectDeviceExtensions(vk::DeviceCreateInfo& deviceCreateInfo, plHybridArray<const char*, 6>& extensions);

  virtual plResult InitPlatform() override;
  virtual plResult ShutdownPlatform() override;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, plGALSwapChain* pSwapChain) override;
  virtual void EndPipelinePlatform(plGALSwapChain* pSwapChain) override;

  virtual plGALPass* BeginPassPlatform(const char* szName) override;
  virtual void EndPassPlatform(plGALPass* pPass) override;


  // State creation functions

  virtual plGALBlendState* CreateBlendStatePlatform(const plGALBlendStateCreationDescription& Description) override;
  virtual void DestroyBlendStatePlatform(plGALBlendState* pBlendState) override;

  virtual plGALDepthStencilState* CreateDepthStencilStatePlatform(const plGALDepthStencilStateCreationDescription& Description) override;
  virtual void DestroyDepthStencilStatePlatform(plGALDepthStencilState* pDepthStencilState) override;

  virtual plGALRasterizerState* CreateRasterizerStatePlatform(const plGALRasterizerStateCreationDescription& Description) override;
  virtual void DestroyRasterizerStatePlatform(plGALRasterizerState* pRasterizerState) override;

  virtual plGALSamplerState* CreateSamplerStatePlatform(const plGALSamplerStateCreationDescription& Description) override;
  virtual void DestroySamplerStatePlatform(plGALSamplerState* pSamplerState) override;


  // Resource creation functions

  virtual plGALShader* CreateShaderPlatform(const plGALShaderCreationDescription& Description) override;
  virtual void DestroyShaderPlatform(plGALShader* pShader) override;

  virtual plGALBuffer* CreateBufferPlatform(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData) override;
  virtual void DestroyBufferPlatform(plGALBuffer* pBuffer) override;

  virtual plGALTexture* CreateTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData) override;
  virtual void DestroyTexturePlatform(plGALTexture* pTexture) override;

  virtual plGALResourceView* CreateResourceViewPlatform(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description) override;
  virtual void DestroyResourceViewPlatform(plGALResourceView* pResourceView) override;

  virtual plGALRenderTargetView* CreateRenderTargetViewPlatform(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description) override;
  virtual void DestroyRenderTargetViewPlatform(plGALRenderTargetView* pRenderTargetView) override;

  plGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description) override;
  virtual void DestroyUnorderedAccessViewPlatform(plGALUnorderedAccessView* pResource) override;

  // Other rendering creation functions

  virtual plGALQuery* CreateQueryPlatform(const plGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(plGALQuery* pQuery) override;

  virtual plGALVertexDeclaration* CreateVertexDeclarationPlatform(const plGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(plGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual plGALTimestampHandle GetTimestampPlatform() override;
  virtual plResult GetTimestampResultPlatform(plGALTimestampHandle hTimestamp, plTime& result) override;

  // Misc functions

  virtual void BeginFramePlatform(const plUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  /// \endcond

private:
  struct PerFrameData
  {
    /// \brief These are all fences passed into submit calls. For some reason waiting for the fence of the last submit is not enough. At least I can't get it to work (neither semaphores nor barriers make it past the validation layer).
    plHybridArray<vk::Fence, 2> m_CommandBufferFences;

    vk::CommandBuffer m_currentCommandBuffer;
    //ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    plUInt64 m_uiFrame = -1;

    plMutex m_pendingDeletionsMutex;
    plDeque<PendingDeletion> m_pendingDeletions;
    plDeque<PendingDeletion> m_pendingDeletionsPrevious;

    plMutex m_reclaimResourcesMutex;
    plDeque<ReclaimResource> m_reclaimResources;
    plDeque<ReclaimResource> m_reclaimResourcesPrevious;
  };

  void DeletePendingResources(plDeque<PendingDeletion>& pendingDeletions);
  void ReclaimResources(plDeque<ReclaimResource>& resources);

  void FillFormatLookupTable();

  plUInt64 m_uiFrameCounter = 1; ///< We start at 1 so m_uiFrameCounter and m_uiSafeFrame are not equal at the start.
  plUInt64 m_uiSafeFrame = 0;
  plUInt8 m_uiCurrentPerFrameData = 0;
  plUInt8 m_uiNextPerFrameData = 1;

  vk::Instance m_instance;
  vk::PhysicalDevice m_physicalDevice;
  vk::PhysicalDeviceProperties m_properties;
  vk::Device m_device;
  Queue m_graphicsQueue;
  Queue m_transferQueue;

  plGALFormatLookupTableVulkan m_FormatLookupTable;
  vk::PipelineStageFlags m_supportedStages;
  vk::PhysicalDeviceMemoryProperties m_memoryProperties;

  plUniquePtr<plGALPassVulkan> m_pDefaultPass;
  plUniquePtr<plPipelineBarrierVulkan> m_pPipelineBarrier;
  plUniquePtr<plCommandBufferPoolVulkan> m_pCommandBufferPool;
  plUniquePtr<plStagingBufferPoolVulkan> m_pStagingBufferPool;
  plUniquePtr<plQueryPoolVulkan> m_pQueryPool;
  plUniquePtr<plInitContextVulkan> m_pInitContext;

  // We daisy-chain all command buffers in a frame in sequential order via this semaphore for now.
  vk::Semaphore m_lastCommandBufferFinished;

  PerFrameData m_PerFrameData[4];

#if PLASMA_ENABLED(PLASMA_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
#endif

  Extensions m_extensions;
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
