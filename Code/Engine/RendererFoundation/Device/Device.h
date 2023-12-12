
#pragma once

#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/Strings/HashedString.h>
#include <RendererFoundation/Descriptors/Descriptors.h>
#include <RendererFoundation/Device/DeviceCapabilities.h>
#include <RendererFoundation/RendererFoundationDLL.h>

class plColor;

/// \brief The plRenderDevice class is the primary interface for interactions with rendering APIs
/// It contains a set of (non-virtual) functions to set state, create resources etc. which rely on
/// API specific implementations provided by protected virtual functions.
/// Redundant state changes are prevented at the platform independent level in the non-virtual functions.
class PLASMA_RENDERERFOUNDATION_DLL plGALDevice
{
public:
  plEvent<const plGALDeviceEvent&> m_Events;

  // Init & shutdown functions

  plResult Init();
  plResult Shutdown();

  // Pipeline & Pass functions

  void BeginPipeline(const char* szName, plGALSwapChainHandle hSwapChain);
  void EndPipeline(plGALSwapChainHandle hSwapChain);

  plGALPass* BeginPass(const char* szName);
  void EndPass(plGALPass* pPass);

  // State creation functions

  plGALBlendStateHandle CreateBlendState(const plGALBlendStateCreationDescription& description);
  void DestroyBlendState(plGALBlendStateHandle hBlendState);

  plGALDepthStencilStateHandle CreateDepthStencilState(const plGALDepthStencilStateCreationDescription& description);
  void DestroyDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState);

  plGALRasterizerStateHandle CreateRasterizerState(const plGALRasterizerStateCreationDescription& description);
  void DestroyRasterizerState(plGALRasterizerStateHandle hRasterizerState);

  plGALSamplerStateHandle CreateSamplerState(const plGALSamplerStateCreationDescription& description);
  void DestroySamplerState(plGALSamplerStateHandle hSamplerState);

  // Resource creation functions

  plGALShaderHandle CreateShader(const plGALShaderCreationDescription& description);
  void DestroyShader(plGALShaderHandle hShader);

  plGALBufferHandle CreateBuffer(const plGALBufferCreationDescription& description, plArrayPtr<const plUInt8> initialData = plArrayPtr<const plUInt8>());
  void DestroyBuffer(plGALBufferHandle hBuffer);

  // Helper functions for buffers (for common, simple use cases)

  plGALBufferHandle CreateVertexBuffer(plUInt32 uiVertexSize, plUInt32 uiVertexCount, plArrayPtr<const plUInt8> initialData = plArrayPtr<const plUInt8>(), bool bDataIsMutable = false);
  plGALBufferHandle CreateIndexBuffer(plGALIndexType::Enum indexType, plUInt32 uiIndexCount, plArrayPtr<const plUInt8> initialData = plArrayPtr<const plUInt8>(), bool bDataIsMutable = false);
  plGALBufferHandle CreateConstantBuffer(plUInt32 uiBufferSize);

  plGALTextureHandle CreateTexture(const plGALTextureCreationDescription& description, plArrayPtr<plGALSystemMemoryDescription> initialData = plArrayPtr<plGALSystemMemoryDescription>());
  void DestroyTexture(plGALTextureHandle hTexture);

  plGALTextureHandle CreateProxyTexture(plGALTextureHandle hParentTexture, plUInt32 uiSlice);
  void DestroyProxyTexture(plGALTextureHandle hProxyTexture);

  // Resource views
  plGALResourceViewHandle GetDefaultResourceView(plGALTextureHandle hTexture);
  plGALResourceViewHandle GetDefaultResourceView(plGALBufferHandle hBuffer);

  plGALResourceViewHandle CreateResourceView(const plGALResourceViewCreationDescription& description);
  void DestroyResourceView(plGALResourceViewHandle hResourceView);

  // Render target views
  plGALRenderTargetViewHandle GetDefaultRenderTargetView(plGALTextureHandle hTexture);

  plGALRenderTargetViewHandle CreateRenderTargetView(const plGALRenderTargetViewCreationDescription& description);
  void DestroyRenderTargetView(plGALRenderTargetViewHandle hRenderTargetView);

  // Unordered access views
  plGALUnorderedAccessViewHandle CreateUnorderedAccessView(const plGALUnorderedAccessViewCreationDescription& description);
  void DestroyUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView);


  // Other rendering creation functions

  using SwapChainFactoryFunction = plDelegate<plGALSwapChain*(plAllocatorBase*)>;
  plGALSwapChainHandle CreateSwapChain(const SwapChainFactoryFunction& func);
  plResult UpdateSwapChain(plGALSwapChainHandle hSwapChain, plEnum<plGALPresentMode> newPresentMode);
  void DestroySwapChain(plGALSwapChainHandle hSwapChain);

  plGALQueryHandle CreateQuery(const plGALQueryCreationDescription& description);
  void DestroyQuery(plGALQueryHandle hQuery);

  plGALVertexDeclarationHandle CreateVertexDeclaration(const plGALVertexDeclarationCreationDescription& description);
  void DestroyVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration);

  // Timestamp functions

  plResult GetTimestampResult(plGALTimestampHandle hTimestamp, plTime& ref_result);

  /// \todo Map functions to save on memcpys

  // Swap chain functions

  plGALTextureHandle GetBackBufferTextureFromSwapChain(plGALSwapChainHandle hSwapChain);


  // Misc functions

  void BeginFrame(const plUInt64 uiRenderFrame = 0);
  void EndFrame();

  plGALTimestampHandle GetTimestamp();

  const plGALDeviceCreationDescription* GetDescription() const;

  const plGALSwapChain* GetSwapChain(plGALSwapChainHandle hSwapChain) const;
  template <typename T>
  const T* GetSwapChain(plGALSwapChainHandle hSwapChain) const
  {
    return static_cast<const T*>(GetSwapChainInternal(hSwapChain, plGetStaticRTTI<T>()));
  }

  const plGALShader* GetShader(plGALShaderHandle hShader) const;
  const plGALTexture* GetTexture(plGALTextureHandle hTexture) const;
  const plGALBuffer* GetBuffer(plGALBufferHandle hBuffer) const;
  const plGALDepthStencilState* GetDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState) const;
  const plGALBlendState* GetBlendState(plGALBlendStateHandle hBlendState) const;
  const plGALRasterizerState* GetRasterizerState(plGALRasterizerStateHandle hRasterizerState) const;
  const plGALVertexDeclaration* GetVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration) const;
  const plGALSamplerState* GetSamplerState(plGALSamplerStateHandle hSamplerState) const;
  const plGALResourceView* GetResourceView(plGALResourceViewHandle hResourceView) const;
  const plGALRenderTargetView* GetRenderTargetView(plGALRenderTargetViewHandle hRenderTargetView) const;
  const plGALUnorderedAccessView* GetUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView) const;
  const plGALQuery* GetQuery(plGALQueryHandle hQuery) const;

  const plGALDeviceCapabilities& GetCapabilities() const;

  virtual plUInt64 GetMemoryConsumptionForTexture(const plGALTextureCreationDescription& description) const;
  virtual plUInt64 GetMemoryConsumptionForBuffer(const plGALBufferCreationDescription& description) const;

  static void SetDefaultDevice(plGALDevice* pDefaultDevice);
  static plGALDevice* GetDefaultDevice();
  static bool HasDefaultDevice();

  /// \brief Waits for the GPU to be idle and destroys any pending resources and GPU objects.
  void WaitIdle();

  // public in case someone external needs to lock multiple operations
  mutable plMutex m_Mutex;

private:
  static plGALDevice* s_pDefaultDevice;

protected:
  plGALDevice(const plGALDeviceCreationDescription& Description);

  virtual ~plGALDevice();

  template <typename IdTableType, typename ReturnType>
  ReturnType* Get(typename IdTableType::TypeOfId hHandle, const IdTableType& IdTable) const;

  void DestroyViews(plGALResourceBase* pResource);

  template <typename HandleType>
  void AddDeadObject(plUInt32 uiType, HandleType handle);

  template <typename HandleType>
  void ReviveDeadObject(plUInt32 uiType, HandleType handle);

  void DestroyDeadObjects();

  /// \brief Asserts that either this device supports multi-threaded resource creation, or that this function is executed on the main thread.
  void VerifyMultithreadedAccess() const;

  const plGALSwapChain* GetSwapChainInternal(plGALSwapChainHandle hSwapChain, const plRTTI* pRequestedType) const;

  plGALTextureHandle FinalizeTextureInternal(const plGALTextureCreationDescription& desc, plGALTexture* pTexture);
  plGALBufferHandle FinalizeBufferInternal(const plGALBufferCreationDescription& desc, plGALBuffer* pBuffer);

  plProxyAllocator m_Allocator;
  plLocalAllocatorWrapper m_AllocatorWrapper;

  using ShaderTable = plIdTable<plGALShaderHandle::IdType, plGALShader*, plLocalAllocatorWrapper>;
  using BlendStateTable = plIdTable<plGALBlendStateHandle::IdType, plGALBlendState*, plLocalAllocatorWrapper>;
  using DepthStencilStateTable = plIdTable<plGALDepthStencilStateHandle::IdType, plGALDepthStencilState*, plLocalAllocatorWrapper>;
  using RasterizerStateTable = plIdTable<plGALRasterizerStateHandle::IdType, plGALRasterizerState*, plLocalAllocatorWrapper>;
  using BufferTable = plIdTable<plGALBufferHandle::IdType, plGALBuffer*, plLocalAllocatorWrapper>;
  using TextureTable = plIdTable<plGALTextureHandle::IdType, plGALTexture*, plLocalAllocatorWrapper>;
  using ResourceViewTable = plIdTable<plGALResourceViewHandle::IdType, plGALResourceView*, plLocalAllocatorWrapper>;
  using SamplerStateTable = plIdTable<plGALSamplerStateHandle::IdType, plGALSamplerState*, plLocalAllocatorWrapper>;
  using RenderTargetViewTable = plIdTable<plGALRenderTargetViewHandle::IdType, plGALRenderTargetView*, plLocalAllocatorWrapper>;
  using UnorderedAccessViewTable = plIdTable<plGALUnorderedAccessViewHandle::IdType, plGALUnorderedAccessView*, plLocalAllocatorWrapper>;
  using SwapChainTable = plIdTable<plGALSwapChainHandle::IdType, plGALSwapChain*, plLocalAllocatorWrapper>;
  using QueryTable = plIdTable<plGALQueryHandle::IdType, plGALQuery*, plLocalAllocatorWrapper>;
  using VertexDeclarationTable = plIdTable<plGALVertexDeclarationHandle::IdType, plGALVertexDeclaration*, plLocalAllocatorWrapper>;

  ShaderTable m_Shaders;
  BlendStateTable m_BlendStates;
  DepthStencilStateTable m_DepthStencilStates;
  RasterizerStateTable m_RasterizerStates;
  BufferTable m_Buffers;
  TextureTable m_Textures;
  ResourceViewTable m_ResourceViews;
  SamplerStateTable m_SamplerStates;
  RenderTargetViewTable m_RenderTargetViews;
  UnorderedAccessViewTable m_UnorderedAccessViews;
  SwapChainTable m_SwapChains;
  QueryTable m_Queries;
  VertexDeclarationTable m_VertexDeclarations;


  // Hash tables used to prevent state object duplication
  plHashTable<plUInt32, plGALBlendStateHandle, plHashHelper<plUInt32>, plLocalAllocatorWrapper> m_BlendStateTable;
  plHashTable<plUInt32, plGALDepthStencilStateHandle, plHashHelper<plUInt32>, plLocalAllocatorWrapper> m_DepthStencilStateTable;
  plHashTable<plUInt32, plGALRasterizerStateHandle, plHashHelper<plUInt32>, plLocalAllocatorWrapper> m_RasterizerStateTable;
  plHashTable<plUInt32, plGALSamplerStateHandle, plHashHelper<plUInt32>, plLocalAllocatorWrapper> m_SamplerStateTable;
  plHashTable<plUInt32, plGALVertexDeclarationHandle, plHashHelper<plUInt32>, plLocalAllocatorWrapper> m_VertexDeclarationTable;

  struct DeadObject
  {
    PLASMA_DECLARE_POD_TYPE();

    plUInt32 m_uiType;
    plUInt32 m_uiHandle;
  };

  plDynamicArray<DeadObject, plLocalAllocatorWrapper> m_DeadObjects;

  plGALDeviceCreationDescription m_Description;

  plGALDeviceCapabilities m_Capabilities;

  // Deactivate Doxygen document generation for the following block. (API abstraction only)
  /// \cond

  // These functions need to be implemented by a render API abstraction
protected:
  friend class plMemoryUtils;

  // Init & shutdown functions

  virtual plResult InitPlatform() = 0;
  virtual plResult ShutdownPlatform() = 0;

  // Pipeline & Pass functions

  virtual void BeginPipelinePlatform(const char* szName, plGALSwapChain* pSwapChain) = 0;
  virtual void EndPipelinePlatform(plGALSwapChain* pSwapChain) = 0;

  virtual plGALPass* BeginPassPlatform(const char* szName) = 0;
  virtual void EndPassPlatform(plGALPass* pPass) = 0;

  // State creation functions

  virtual plGALBlendState* CreateBlendStatePlatform(const plGALBlendStateCreationDescription& Description) = 0;
  virtual void DestroyBlendStatePlatform(plGALBlendState* pBlendState) = 0;

  virtual plGALDepthStencilState* CreateDepthStencilStatePlatform(const plGALDepthStencilStateCreationDescription& Description) = 0;
  virtual void DestroyDepthStencilStatePlatform(plGALDepthStencilState* pDepthStencilState) = 0;

  virtual plGALRasterizerState* CreateRasterizerStatePlatform(const plGALRasterizerStateCreationDescription& Description) = 0;
  virtual void DestroyRasterizerStatePlatform(plGALRasterizerState* pRasterizerState) = 0;

  virtual plGALSamplerState* CreateSamplerStatePlatform(const plGALSamplerStateCreationDescription& Description) = 0;
  virtual void DestroySamplerStatePlatform(plGALSamplerState* pSamplerState) = 0;

  // Resource creation functions

  virtual plGALShader* CreateShaderPlatform(const plGALShaderCreationDescription& Description) = 0;
  virtual void DestroyShaderPlatform(plGALShader* pShader) = 0;

  virtual plGALBuffer* CreateBufferPlatform(const plGALBufferCreationDescription& Description, plArrayPtr<const plUInt8> pInitialData) = 0;
  virtual void DestroyBufferPlatform(plGALBuffer* pBuffer) = 0;

  virtual plGALTexture* CreateTexturePlatform(const plGALTextureCreationDescription& Description, plArrayPtr<plGALSystemMemoryDescription> pInitialData) = 0;
  virtual void DestroyTexturePlatform(plGALTexture* pTexture) = 0;

  virtual plGALResourceView* CreateResourceViewPlatform(plGALResourceBase* pResource, const plGALResourceViewCreationDescription& Description) = 0;
  virtual void DestroyResourceViewPlatform(plGALResourceView* pResourceView) = 0;

  virtual plGALRenderTargetView* CreateRenderTargetViewPlatform(plGALTexture* pTexture, const plGALRenderTargetViewCreationDescription& Description) = 0;
  virtual void DestroyRenderTargetViewPlatform(plGALRenderTargetView* pRenderTargetView) = 0;

  virtual plGALUnorderedAccessView* CreateUnorderedAccessViewPlatform(plGALResourceBase* pResource, const plGALUnorderedAccessViewCreationDescription& Description) = 0;
  virtual void DestroyUnorderedAccessViewPlatform(plGALUnorderedAccessView* pUnorderedAccessView) = 0;

  // Other rendering creation functions

  virtual plGALQuery* CreateQueryPlatform(const plGALQueryCreationDescription& Description) = 0;
  virtual void DestroyQueryPlatform(plGALQuery* pQuery) = 0;

  virtual plGALVertexDeclaration* CreateVertexDeclarationPlatform(const plGALVertexDeclarationCreationDescription& Description) = 0;
  virtual void DestroyVertexDeclarationPlatform(plGALVertexDeclaration* pVertexDeclaration) = 0;

  // Timestamp functions

  virtual plGALTimestampHandle GetTimestampPlatform() = 0;
  virtual plResult GetTimestampResultPlatform(plGALTimestampHandle hTimestamp, plTime& result) = 0;

  // Misc functions

  virtual void BeginFramePlatform(const plUInt64 uiRenderFrame) = 0;
  virtual void EndFramePlatform() = 0;

  virtual void FillCapabilitiesPlatform() = 0;

  virtual void WaitIdlePlatform() = 0;


  /// \endcond

private:
  bool m_bBeginFrameCalled = false;
  bool m_bBeginPipelineCalled = false;
  bool m_bBeginPassCalled = false;
};

#include <RendererFoundation/Device/Implementation/Device_inl.h>
