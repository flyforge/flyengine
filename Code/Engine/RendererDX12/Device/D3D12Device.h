#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX12/RendererDX12PCH.h>
#include <RendererFoundation/Device/Device.h>

#include <d3d12.h>
#include <dxgi.h>

struct ID3D12CommandAllocator3;
struct ID3D12CommandList3;
struct ID3D12CommandQueue3;
struct ID3D12Device3;
struct ID3D12Debug2;
struct IDXGIFactory3;
struct IDXGIAdapter3;
struct IDXGIDevice3;
struct ID3D12Resource2;
struct ID3D12Query;
struct IDXGIAdapter3;

using plGALFormatLookupEntryDX12 = plGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using plGALFormatLookupTableDX12 = plGALFormatLookupTable<plGALFormatLookupEntryDX12>;

class plGALPassDX12;

/// The DX12 device implementation of the graphics abstraction layer.

class PLASMA_RENDERERDX12_DLL plGALDeviceDX12 : public plGALDevice
{
private:
  friend plInternal::NewInstance<plGALDevice> CreateDX12Device(plAllocatorBase* pAllocator, const plGALDeviceCreationDescription& desc);
  plGALDeviceDX12(const plGALDeviceCreationDescription& desc);

public:
  virtual ~plGALDeviceDX12() {}

public:
  ID3D12Device3* GetDXDevice() const;
  IDXGIFactory3* GetDXGIFactory() const;
  ID3D12CommandAllocator3* GetDirectCommandAllocator() const;
  ID3D12CommandAllocator3* GetComputeCommandAllocator() const;
  ID3D12CommandAllocator3* GetCopyCommandAllocator() const;

  plGALRenderCommandEncoder* GetRenderCommandEncoder() const;

  const plGALFormatLookupTableDX12& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

protected:

  plResult InitPlatform(D3D_FEATURE_LEVEL platformfeaturelevel, IDXGIAdapter1* wantedadapterused);

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
  virtual void DestroyUnorderedAccessViewPlatform(plGALUnorderedAccessView* pUnorderedAccessView) override;

  // Other rendering creation functions

  virtual plGALQuery* CreateQueryPlatform(const plGALQueryCreationDescription& Description) override;
  virtual void DestroyQueryPlatform(plGALQuery* pQuery) override;

  virtual plGALVertexDeclaration* CreateVertexDeclarationPlatform(const plGALVertexDeclarationCreationDescription& Description) override;
  virtual void DestroyVertexDeclarationPlatform(plGALVertexDeclaration* pVertexDeclaration) override;

  // Timestamp functions

  virtual plGALTimestampHandle GetTimestampPlatform() override;
  virtual plResult GetTimestampResultPlatform(plGALTimestampHandle hTimestamp, plTime& result) override;

  // Swap chain functions

  void PresentPlatform(const plGALSwapChain* pSwapChain, bool bVSync);

  // Misc functions

  virtual void BeginFramePlatform(const plUInt64 uiRenderFrame) override;
  virtual void EndFramePlatform() override;

  virtual void FillCapabilitiesPlatform() override;

  virtual void WaitIdlePlatform() override;

  virtual const plGALSharedTexture* GetSharedTexture(plGALTextureHandle hTexture) const override;

  /// \endcond
  friend class plGALCommandEncoderImplDX12;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,
      ENUM_COUNT
    };
  };

  ID3D12Query* GetTimestamp(plGALTimestampHandle hTimestamp);

  ID3D12Resource* FindTempBuffer(plUInt32 uiSize);
  ID3D12Resource* FindTempTexture(plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiDepth, plGALResourceFormat::Enum format);
  void FreeTempResources(plUInt64 uiFrame);

  void FillFormatLookupTable();



  void InsertFencePlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue, ID3D12Query* pFence);

  bool IsFenceReachedPlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue,  ID3D12Query* pFence);

  void WaitForFencePlatform(ID3D12Device3* pDevice, ID3D12CommandQueue3* pQueue, ID3D12Query* pFence);

  ID3D12Device* m_pDevice = nullptr;
  ID3D12Device3* m_pDevice3 = nullptr;

  ID3D12Debug1* m_pDebug = nullptr;
  ID3D12DebugDevice1* m_pDebugDevice = nullptr;

  IDXGIFactory3* m_pDXGIFactory = nullptr;

  IDXGIAdapter1* m_pDXGIAdapter = nullptr;

  IDXGIDevice3* m_pDXGIDevice = nullptr;

  plGALFormatLookupTableDX12 m_FormatLookupTable;

  /// Does the GPU Support DX12U? if so, this allows RT, Mesh Shaders, Work Graphs, and more.
  bool m_pDX12Ultimate = false;

  /// D3D_FEATURE_LEVEL can't be forward declared, This should only be 2 values: D3D_FEATURE_LEVEL_12_1 (Normal) D3D_FEATURE_LEVEL_12_2(DX12 Ultimate, Ray tracing, Mesh Shaders, Etc...)
  plUInt32 m_uiFeatureLevel; 
  plUInt32 m_uiDxgiFlags = 0;

  plUniquePtr<plGALPassDX12> m_pDefaultPass;

  struct PerFrameData
  {
    ID3D12Fence* m_pFence = nullptr;
    ID3D12Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    plUInt64 m_uiFrame = -1;
  };

  PerFrameData m_PerFrameData[4];
  plUInt8 m_uiCurrentPerFrameData = 0;
  plUInt8 m_uiNextPerFrameData = 0;

  plUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    PLASMA_DECLARE_POD_TYPE();

    ID3D12Resource* m_pResource;
    plUInt64 m_uiFrame;
    plUInt32 m_uiHash;
  };

  plMap<plUInt32, plDynamicArray<ID3D12Resource*>, plCompareHelper<plUInt32>, plLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  plDeque<UsedTempResource, plLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  plDynamicArray<ID3D12Query*, plLocalAllocatorWrapper> m_Timestamps;
  plUInt32 m_uiCurrentTimestamp = 0;
  plUInt32 m_uiNextTimestamp = 0;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;

  plTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

#include <RendererDX12/Device/Implementation/D3D12Device_inl.h>