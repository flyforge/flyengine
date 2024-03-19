
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererVulkan/Device/DispatchContext.h>
#include <RendererVulkan/RendererVulkanDLL.h>

#include <vulkan/vulkan.hpp>

PL_DEFINE_AS_POD_TYPE(vk::Format);

struct plGALFormatLookupEntryVulkan
{
  plGALFormatLookupEntryVulkan() = default;
  plGALFormatLookupEntryVulkan(vk::Format format)
  {
    m_format = format;
    m_readback = format;
  }

  plGALFormatLookupEntryVulkan(vk::Format format, plArrayPtr<vk::Format> mutableFormats)
  {
    m_format = format;
    m_readback = format;
    m_mutableFormats = mutableFormats;
  }

  inline plGALFormatLookupEntryVulkan& R(vk::Format readbackType)
  {
    m_readback = readbackType;
    return *this;
  }

  vk::Format m_format = vk::Format::eUndefined;
  vk::Format m_readback = vk::Format::eUndefined;
  plHybridArray<vk::Format, 6> m_mutableFormats;
};

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
class PL_RENDERERVULKAN_DLL plGALDeviceVulkan : public plGALDevice
{
private:
  friend plInternal::NewInstance<plGALDevice> CreateVulkanDevice(plAllocator* pAllocator, const plGALDeviceCreationDescription& Description);
  plGALDeviceVulkan(const plGALDeviceCreationDescription& Description);

public:
  virtual ~plGALDeviceVulkan();

public:
  struct PendingDeletionFlags
  {
    using StorageType = plUInt32;

    enum Enum
    {
      UsesExternalMemory = PL_BIT(0),
      IsFileDescriptor = PL_BIT(1),
      Default = 0
    };

    struct Bits
    {
      StorageType UsesExternalMemory : 1;
      StorageType IsFileDescriptor : 1;
    };
  };

  struct PendingDeletion
  {
    PL_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    plBitflags<PendingDeletionFlags> m_flags;
    void* m_pObject;
    union
    {
      plVulkanAllocation m_allocation;
      void* m_pContext;
    };
  };

  struct ReclaimResource
  {
    PL_DECLARE_POD_TYPE();
    vk::ObjectType m_type;
    void* m_pObject;
    void* m_pContext = nullptr;
  };

  struct Extensions
  {
    bool m_bSurface = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    bool m_bWin32Surface = false;
#elif PL_ENABLED(PL_SUPPORTS_GLFW)
#else
#  error "Vulkan Platform not supported"
#endif

    bool m_bDebugUtils = false;
    bool m_bDebugUtilsMarkers = false;
    PFN_vkCreateDebugUtilsMessengerEXT pfn_vkCreateDebugUtilsMessengerEXT = nullptr;
    PFN_vkDestroyDebugUtilsMessengerEXT pfn_vkDestroyDebugUtilsMessengerEXT = nullptr;
    PFN_vkSetDebugUtilsObjectNameEXT pfn_vkSetDebugUtilsObjectNameEXT = nullptr;

    bool m_bDeviceSwapChain = false;
    bool m_bShaderViewportIndexLayer = false;

    vk::PhysicalDeviceCustomBorderColorFeaturesEXT m_borderColorEXT;
    bool m_bBorderColorFloat = false;

    bool m_bImageFormatList = false;
    vk::PhysicalDeviceTimelineSemaphoreFeatures m_timelineSemaphoresEXT;
    bool m_bTimelineSemaphore = false;

    bool m_bExternalMemoryFd = false;
    bool m_bExternalSemaphoreFd = false;

    bool m_bExternalMemoryWin32 = false;
    bool m_bExternalSemaphoreWin32 = false;
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
  const plVulkanDispatchContext& GetDispatchContext() const { return m_dispatchContext; }
  vk::PipelineStageFlags GetSupportedStages() const;

  vk::CommandBuffer& GetCurrentCommandBuffer();
  plPipelineBarrierVulkan& GetCurrentPipelineBarrier();
  plQueryPoolVulkan& GetQueryPool() const;
  plStagingBufferPoolVulkan& GetStagingBufferPool() const;
  plInitContextVulkan& GetInitContext() const;
  plProxyAllocator& GetAllocator();

  plGALTextureHandle CreateTextureInternal(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, bool bLinearCPU = false, bool bStaging = false);
  plGALBufferHandle CreateBufferInternal(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData, bool bCPU = false);

  const plGALFormatLookupTableVulkan& GetFormatLookupTable() const;

  plInt32 GetMemoryIndex(vk::MemoryPropertyFlags properties, const vk::MemoryRequirements& requirements) const;

  vk::Fence Submit(bool bAddSignalSemaphore = true);

  void DeleteLaterImpl(const PendingDeletion& deletion);

  void DeleteLater(vk::Image& image, vk::DeviceMemory& externalMemory)
  {
    if (image)
    {
      PendingDeletion del = {vk::ObjectType::eImage, {PendingDeletionFlags::UsesExternalMemory}, (void*)image, nullptr};
      del.m_pContext = (void*)externalMemory;
      DeleteLaterImpl(del);
    }
    image = nullptr;
    externalMemory = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, plVulkanAllocation& allocation)
  {
    if (object)
    {
      DeleteLaterImpl({object.objectType, {}, (void*)object, allocation});
    }
    object = nullptr;
    allocation = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object, void* pContext)
  {
    if (object)
    {
      PendingDeletion del = {object.objectType, {}, (void*)object, nullptr};
      del.m_pContext = pContext;
      DeleteLaterImpl(static_cast<const PendingDeletion&>(del));
    }
    object = nullptr;
  }

  template <typename T>
  void DeleteLater(T& object)
  {
    if (object)
    {
      DeleteLaterImpl({object.objectType, {}, (void*)object, nullptr});
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
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
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

  virtual const plGALSharedTexture* GetSharedTexture(plGALTextureHandle hTexture) const override;

  struct SemaphoreInfo
  {
    static SemaphoreInfo MakeWaitSemaphore(vk::Semaphore semaphore, vk::PipelineStageFlagBits waitStage = vk::PipelineStageFlagBits::eAllCommands, vk::SemaphoreType type = vk::SemaphoreType::eBinary, plUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, waitStage, uiValue};
    }

    static SemaphoreInfo MakeSignalSemaphore(vk::Semaphore semaphore, vk::SemaphoreType type = vk::SemaphoreType::eBinary, plUInt64 uiValue = 0)
    {
      return SemaphoreInfo{semaphore, type, vk::PipelineStageFlagBits::eNone, uiValue};
    }

    vk::Semaphore m_semaphore;
    vk::SemaphoreType m_type = vk::SemaphoreType::eBinary;
    vk::PipelineStageFlagBits m_waitStage = vk::PipelineStageFlagBits::eAllCommands;
    plUInt64 m_uiValue = 0;
  };
  void AddWaitSemaphore(const SemaphoreInfo& waitSemaphore);
  void AddSignalSemaphore(const SemaphoreInfo& signalSemaphore);

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

  virtual void FlushPlatform() override;


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

  virtual plGALTexture* CreateSharedTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData, plEnum<plGALSharedTextureType> sharedType, plGALPlatformSharedHandle handle) override;
  virtual void DestroySharedTexturePlatform(plGALTexture* pTexture) override;

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

#if PL_ENABLED(PL_USE_PROFILING)
  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;
#endif

  Extensions m_extensions;
  plVulkanDispatchContext m_dispatchContext;
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;
#endif
  plHybridArray<SemaphoreInfo, 3> m_waitSemaphores;
  plHybridArray<SemaphoreInfo, 3> m_signalSemaphores;
};

#include <RendererVulkan/Device/Implementation/DeviceVulkan_inl.h>
