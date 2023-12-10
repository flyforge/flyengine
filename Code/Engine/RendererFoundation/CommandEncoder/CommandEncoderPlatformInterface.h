
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct plShaderResourceBinding;

class PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoderCommonPlatformInterface
{
public:
  // State setting functions

  virtual void SetShaderPlatform(const plGALShader* pShader) = 0;

  virtual void SetConstantBufferPlatform(const plShaderResourceBinding& binding, const plGALBuffer* pBuffer) = 0;
  virtual void SetSamplerStatePlatform(const plShaderResourceBinding& binding, const plGALSamplerState* pSamplerState) = 0;
  virtual void SetResourceViewPlatform(const plShaderResourceBinding& binding, const plGALResourceView* pResourceView) = 0;
  virtual void SetUnorderedAccessViewPlatform(const plShaderResourceBinding& binding, const plGALUnorderedAccessView* pUnorderedAccessView) = 0;

  // Query functions

  virtual void BeginQueryPlatform(const plGALQuery* pQuery) = 0;
  virtual void EndQueryPlatform(const plGALQuery* pQuery) = 0;
  virtual plResult GetQueryResultPlatform(const plGALQuery* pQuery, plUInt64& ref_uiQueryResult) = 0;

  // Timestamp functions

  virtual void InsertTimestampPlatform(plGALTimestampHandle hTimestamp) = 0;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4 vClearValues) = 0;
  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4U32 vClearValues) = 0;

  virtual void CopyBufferPlatform(const plGALBuffer* pDestination, const plGALBuffer* pSource) = 0;
  virtual void CopyBufferRegionPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, const plGALBuffer* pSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount) = 0;

  virtual void UpdateBufferPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> sourceData, plGALUpdateMode::Enum updateMode) = 0;

  virtual void CopyTexturePlatform(const plGALTexture* pDestination, const plGALTexture* pSource) = 0;
  virtual void CopyTextureRegionPlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource, const plVec3U32& vDestinationPoint, const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource, const plBoundingBoxu32& box) = 0;

  virtual void UpdateTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource, const plBoundingBoxu32& destinationBox, const plGALSystemMemoryDescription& sourceData) = 0;

  virtual void ResolveTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource, const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource) = 0;

  virtual void ReadbackTexturePlatform(const plGALTexture* pTexture) = 0;

  virtual void CopyTextureReadbackResultPlatform(const plGALTexture* pTexture, plArrayPtr<plGALTextureSubresource> sourceSubResource, plArrayPtr<plGALSystemMemoryDescription> targetData) = 0;

  virtual void GenerateMipMapsPlatform(const plGALResourceView* pResourceView) = 0;

  // Misc

  virtual void FlushPlatform() = 0;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) = 0;
  virtual void PopMarkerPlatform() = 0;
  virtual void InsertEventMarkerPlatform(const char* szMarker) = 0;
};

class PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoderRenderPlatformInterface
{
public:
  // Draw functions

  virtual void ClearPlatform(const plColor& clearColor, plUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, plUInt8 uiStencilClear) = 0;

  virtual void DrawPlatform(plUInt32 uiVertexCount, plUInt32 uiStartVertex) = 0;
  virtual void DrawIndexedPlatform(plUInt32 uiIndexCount, plUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedPlatform(plUInt32 uiIndexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartIndex) = 0;
  virtual void DrawIndexedInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) = 0;
  virtual void DrawInstancedPlatform(plUInt32 uiVertexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartVertex) = 0;
  virtual void DrawInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) = 0;

  // State functions

  virtual void SetIndexBufferPlatform(const plGALBuffer* pIndexBuffer) = 0;
  virtual void SetVertexBufferPlatform(plUInt32 uiSlot, const plGALBuffer* pVertexBuffer) = 0;
  virtual void SetVertexDeclarationPlatform(const plGALVertexDeclaration* pVertexDeclaration) = 0;
  virtual void SetPrimitiveTopologyPlatform(plGALPrimitiveTopology::Enum topology) = 0;

  virtual void SetBlendStatePlatform(const plGALBlendState* pBlendState, const plColor& blendFactor, plUInt32 uiSampleMask) = 0;
  virtual void SetDepthStencilStatePlatform(const plGALDepthStencilState* pDepthStencilState, plUInt8 uiStencilRefValue) = 0;
  virtual void SetRasterizerStatePlatform(const plGALRasterizerState* pRasterizerState) = 0;

  virtual void SetViewportPlatform(const plRectFloat& rect, float fMinDepth, float fMaxDepth) = 0;
  virtual void SetScissorRectPlatform(const plRectU32& rect) = 0;
};

class PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoderComputePlatformInterface
{
public:
  // Dispatch

  virtual void DispatchPlatform(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ) = 0;
  virtual void DispatchIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) = 0;
};
