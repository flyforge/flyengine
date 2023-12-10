
#pragma once

#include <Foundation/Types/Bitflags.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>

struct ID3D11DeviceChild;
struct ID3D11DeviceContext;
struct ID3DUserDefinedAnnotation;
struct ID3D11RenderTargetView;
struct ID3D11DepthStencilView;
struct ID3D11Buffer;
struct ID3D11ShaderResourceView;
struct ID3D11UnorderedAccessView;
struct ID3D11SamplerState;
struct ID3D11Query;

class plGALDeviceDX11;

class PLASMA_RENDERERDX11_DLL plGALCommandEncoderImplDX11 : public plGALCommandEncoderCommonPlatformInterface, public plGALCommandEncoderRenderPlatformInterface, public plGALCommandEncoderComputePlatformInterface
{
public:
  plGALCommandEncoderImplDX11(plGALDeviceDX11& ref_deviceDX11);
  ~plGALCommandEncoderImplDX11();

  // plGALCommandEncoderCommonPlatformInterface
  // State setting functions

  virtual void SetShaderPlatform(const plGALShader* pShader) override;

  virtual void SetConstantBufferPlatform(const plShaderResourceBinding& binding, const plGALBuffer* pBuffer) override;
  virtual void SetSamplerStatePlatform(const plShaderResourceBinding& binding, const plGALSamplerState* pSamplerState) override;
  virtual void SetResourceViewPlatform(const plShaderResourceBinding& binding, const plGALResourceView* pResourceView) override;
  virtual void SetUnorderedAccessViewPlatform(const plShaderResourceBinding& binding, const plGALUnorderedAccessView* pUnorderedAccessView) override;

  // Query functions

  virtual void BeginQueryPlatform(const plGALQuery* pQuery) override;
  virtual void EndQueryPlatform(const plGALQuery* pQuery) override;
  virtual plResult GetQueryResultPlatform(const plGALQuery* pQuery, plUInt64& ref_uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(plGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4 vClearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4U32 vClearValues) override;

  virtual void CopyBufferPlatform(const plGALBuffer* pDestination, const plGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, const plGALBuffer* pSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> sourceData, plGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const plGALTexture* pDestination, const plGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource, const plVec3U32& vDestinationPoint, const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource, const plBoundingBoxu32& box) override;

  virtual void UpdateTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource,
    const plBoundingBoxu32& destinationBox, const plGALSystemMemoryDescription& sourceData) override;

  virtual void ResolveTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource,
    const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource) override;

  virtual void ReadbackTexturePlatform(const plGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const plGALTexture* pTexture, plArrayPtr<plGALTextureSubresource> sourceSubResource, plArrayPtr<plGALSystemMemoryDescription> targetData) override;

  virtual void GenerateMipMapsPlatform(const plGALResourceView* pResourceView) override;

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;


  // plGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const plGALRenderingSetup& renderingSetup);
  void BeginCompute();

  // Draw functions

  virtual void ClearPlatform(const plColor& clearColor, plUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, plUInt8 uiStencilClear) override;

  virtual void DrawPlatform(plUInt32 uiVertexCount, plUInt32 uiStartVertex) override;
  virtual void DrawIndexedPlatform(plUInt32 uiIndexCount, plUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedPlatform(plUInt32 uiIndexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartIndex) override;
  virtual void DrawIndexedInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) override;
  virtual void DrawInstancedPlatform(plUInt32 uiVertexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartVertex) override;
  virtual void DrawInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) override;

  // State functions

  virtual void SetIndexBufferPlatform(const plGALBuffer* pIndexBuffer) override;
  virtual void SetVertexBufferPlatform(plUInt32 uiSlot, const plGALBuffer* pVertexBuffer) override;
  virtual void SetVertexDeclarationPlatform(const plGALVertexDeclaration* pVertexDeclaration) override;
  virtual void SetPrimitiveTopologyPlatform(plGALPrimitiveTopology::Enum topology) override;

  virtual void SetBlendStatePlatform(const plGALBlendState* pBlendState, const plColor& blendFactor, plUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const plGALDepthStencilState* pDepthStencilState, plUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const plGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const plRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const plRectU32& rect) override;


  // plGALCommandEncoderComputePlatformInterface
  // Dispatch

  virtual void DispatchPlatform(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) override;

private:
  friend class plGALPassDX11;

  bool UnsetResourceViews(const plGALResourceBase* pResource);
  bool UnsetUnorderedAccessViews(const plGALResourceBase* pResource);
  void FlushDeferredStateChanges();

  plGALDeviceDX11& m_GALDeviceDX11;
  plGALCommandEncoder* m_pOwner = nullptr;

  ID3D11DeviceContext* m_pDXContext = nullptr;
  ID3DUserDefinedAnnotation* m_pDXAnnotation = nullptr;

  // Bound objects for deferred state flushes
  ID3D11Buffer* m_pBoundConstantBuffers[PLASMA_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  plGAL::ModifiedRange m_BoundConstantBuffersRange[plGALShaderStage::ENUM_COUNT];

  plHybridArray<ID3D11ShaderResourceView*, 16> m_pBoundShaderResourceViews[plGALShaderStage::ENUM_COUNT] = {};
  plHybridArray<const plGALResourceBase*, 16> m_ResourcesForResourceViews[plGALShaderStage::ENUM_COUNT];
  plGAL::ModifiedRange m_BoundShaderResourceViewsRange[plGALShaderStage::ENUM_COUNT];

  plHybridArray<ID3D11UnorderedAccessView*, 16> m_BoundUnoderedAccessViews;
  plHybridArray<const plGALResourceBase*, 16> m_ResourcesForUnorderedAccessViews;
  plGAL::ModifiedRange m_BoundUnoderedAccessViewsRange;

  ID3D11SamplerState* m_pBoundSamplerStates[plGALShaderStage::ENUM_COUNT][PLASMA_GAL_MAX_SAMPLER_COUNT] = {};
  plGAL::ModifiedRange m_BoundSamplerStatesRange[plGALShaderStage::ENUM_COUNT];

  ID3D11DeviceChild* m_pBoundShaders[plGALShaderStage::ENUM_COUNT] = {};

  plGALRenderTargetSetup m_RenderTargetSetup;
  ID3D11RenderTargetView* m_pBoundRenderTargets[PLASMA_GAL_MAX_RENDERTARGET_COUNT] = {};
  plUInt32 m_uiBoundRenderTargetCount = 0;
  ID3D11DepthStencilView* m_pBoundDepthStencilTarget = nullptr;

  ID3D11Buffer* m_pBoundVertexBuffers[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  plGAL::ModifiedRange m_BoundVertexBuffersRange;

  plUInt32 m_VertexBufferStrides[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
  plUInt32 m_VertexBufferOffsets[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT] = {};
};
