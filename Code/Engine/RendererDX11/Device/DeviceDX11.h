
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/UniquePtr.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/Device/Device.h>

// TODO: This should not be included in a header, it exposes Windows.h to the outside
#include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#include <dxgi.h>

struct ID3D11Device;
struct ID3D11Device3;
struct ID3D11DeviceContext;
struct ID3D11Debug;
struct IDXGIFactory1;
struct IDXGIAdapter1;
struct IDXGIDevice1;
struct ID3D11Resource;
struct ID3D11Query;
struct IDXGIAdapter;

using plGALFormatLookupEntryDX11 = plGALFormatLookupEntry<DXGI_FORMAT, (DXGI_FORMAT)0>;
using plGALFormatLookupTableDX11 = plGALFormatLookupTable<plGALFormatLookupEntryDX11>;

class plGALPassDX11;

/// \brief The DX11 device implementation of the graphics abstraction layer.
class PL_RENDERERDX11_DLL plGALDeviceDX11 : public plGALDevice
{
private:
  friend plInternal::NewInstance<plGALDevice> CreateDX11Device(plAllocator* pAllocator, const plGALDeviceCreationDescription& description);
  plGALDeviceDX11(const plGALDeviceCreationDescription& Description);

public:
  virtual ~plGALDeviceDX11();

public:
  ID3D11Device* GetDXDevice() const;
  ID3D11Device3* GetDXDevice3() const;
  ID3D11DeviceContext* GetDXImmediateContext() const;
  IDXGIFactory1* GetDXGIFactory() const;
  plGALRenderCommandEncoder* GetRenderCommandEncoder() const;

  const plGALFormatLookupTableDX11& GetFormatLookupTable() const;

  void ReportLiveGpuObjects();

  void FlushDeadObjects();

  // These functions need to be implemented by a render API abstraction
protected:
  // Init & shutdown functions

  /// \brief Internal version of device init that allows to modify device creation flags and graphics adapter.
  ///
  /// \param pUsedAdapter
  ///   Null means default adapter.
  plResult InitPlatform(DWORD flags, IDXGIAdapter* pUsedAdapter);

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

private:
  friend class plGALCommandEncoderImplDX11;

  struct TempResourceType
  {
    enum Enum
    {
      Buffer,
      Texture,

      ENUM_COUNT
    };
  };

  ID3D11Query* GetTimestamp(plGALTimestampHandle hTimestamp);

  ID3D11Resource* FindTempBuffer(plUInt32 uiSize);
  ID3D11Resource* FindTempTexture(plUInt32 uiWidth, plUInt32 uiHeight, plUInt32 uiDepth, plGALResourceFormat::Enum format);
  void FreeTempResources(plUInt64 uiFrame);

  void FillFormatLookupTable();


  void InsertFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  bool IsFenceReachedPlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  void WaitForFencePlatform(ID3D11DeviceContext* pContext, ID3D11Query* pFence);

  ID3D11Device* m_pDevice = nullptr;
  ID3D11Device3* m_pDevice3 = nullptr;
  ID3D11DeviceContext* m_pImmediateContext;

  ID3D11Debug* m_pDebug = nullptr;

  IDXGIFactory1* m_pDXGIFactory = nullptr;

  IDXGIAdapter1* m_pDXGIAdapter = nullptr;

  IDXGIDevice1* m_pDXGIDevice = nullptr;

  plGALFormatLookupTableDX11 m_FormatLookupTable;

  // NOLINTNEXTLINE
  plUInt32 m_uiFeatureLevel; // D3D_FEATURE_LEVEL can't be forward declared

  plUniquePtr<plGALPassDX11> m_pDefaultPass;

  struct PerFrameData
  {
    ID3D11Query* m_pFence = nullptr;
    ID3D11Query* m_pDisjointTimerQuery = nullptr;
    double m_fInvTicksPerSecond = -1.0;
    plUInt64 m_uiFrame = -1;
  };

  PerFrameData m_PerFrameData[4];
  plUInt8 m_uiCurrentPerFrameData = 0;
  plUInt8 m_uiNextPerFrameData = 0;

  plUInt64 m_uiFrameCounter = 0;

  struct UsedTempResource
  {
    PL_DECLARE_POD_TYPE();

    ID3D11Resource* m_pResource;
    plUInt64 m_uiFrame;
    plUInt32 m_uiHash;
  };

  plMap<plUInt32, plDynamicArray<ID3D11Resource*>, plCompareHelper<plUInt32>, plLocalAllocatorWrapper> m_FreeTempResources[TempResourceType::ENUM_COUNT];
  plDeque<UsedTempResource, plLocalAllocatorWrapper> m_UsedTempResources[TempResourceType::ENUM_COUNT];

  plDynamicArray<ID3D11Query*, plLocalAllocatorWrapper> m_Timestamps;
  plUInt32 m_uiCurrentTimestamp = 0;
  plUInt32 m_uiNextTimestamp = 0;

  struct GPUTimingScope* m_pFrameTimingScope = nullptr;
  struct GPUTimingScope* m_pPipelineTimingScope = nullptr;
  struct GPUTimingScope* m_pPassTimingScope = nullptr;

  plTime m_SyncTimeDiff;
  bool m_bSyncTimeNeeded = true;
};

#include <RendererDX11/Device/Implementation/DeviceDX11_inl.h>
