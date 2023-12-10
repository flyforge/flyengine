
#pragma once

#include <RendererVulkan/RendererVulkanDLL.h>

#include <Foundation/Types/Bitflags.h>
#include <RendererFoundation/CommandEncoder/CommandEncoderPlatformInterface.h>
#include <RendererFoundation/Resources/RenderTargetSetup.h>
#include <RendererVulkan/Cache/ResourceCacheVulkan.h>

#include <vulkan/vulkan.hpp>

class plGALBlendStateVulkan;
class plGALBufferVulkan;
class plGALDepthStencilStateVulkan;
class plGALRasterizerStateVulkan;
class plGALResourceViewVulkan;
class plGALSamplerStateVulkan;
class plGALShaderVulkan;
class plGALUnorderedAccessViewVulkan;
class plGALDeviceVulkan;

class PLASMA_RENDERERVULKAN_DLL plGALCommandEncoderImplVulkan : public plGALCommandEncoderCommonPlatformInterface, public plGALCommandEncoderRenderPlatformInterface, public plGALCommandEncoderComputePlatformInterface
{
public:
  plGALCommandEncoderImplVulkan(plGALDeviceVulkan& device);
  ~plGALCommandEncoderImplVulkan();

  void Reset();
  void MarkDirty();
  void SetCurrentCommandBuffer(vk::CommandBuffer* commandBuffer, plPipelineBarrierVulkan* pipelineBarrier);

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
  virtual plResult GetQueryResultPlatform(const plGALQuery* pQuery, plUInt64& uiQueryResult) override;

  // Timestamp functions

  virtual void InsertTimestampPlatform(plGALTimestampHandle hTimestamp) override;

  // Resource update functions

  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4 clearValues) override;
  virtual void ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4U32 clearValues) override;

  virtual void CopyBufferPlatform(const plGALBuffer* pDestination, const plGALBuffer* pSource) override;
  virtual void CopyBufferRegionPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, const plGALBuffer* pSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount) override;

  virtual void UpdateBufferPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> pSourceData, plGALUpdateMode::Enum updateMode) override;

  virtual void CopyTexturePlatform(const plGALTexture* pDestination, const plGALTexture* pSource) override;
  virtual void CopyTextureRegionPlatform(const plGALTexture* pDestination, const plGALTextureSubresource& DestinationSubResource, const plVec3U32& DestinationPoint, const plGALTexture* pSource, const plGALTextureSubresource& SourceSubResource, const plBoundingBoxu32& Box) override;

  virtual void UpdateTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& DestinationSubResource, const plBoundingBoxu32& DestinationBox, const plGALSystemMemoryDescription& pSourceData) override;

  virtual void ResolveTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& DestinationSubResource, const plGALTexture* pSource, const plGALTextureSubresource& SourceSubResource) override;

  virtual void ReadbackTexturePlatform(const plGALTexture* pTexture) override;

  virtual void CopyTextureReadbackResultPlatform(const plGALTexture* pTexture, plArrayPtr<plGALTextureSubresource> SourceSubResource, plArrayPtr<plGALSystemMemoryDescription> TargetData) override;

  virtual void GenerateMipMapsPlatform(const plGALResourceView* pResourceView) override;

  void CopyImageToBuffer(const plGALTextureVulkan* pSource, const plGALBufferVulkan* pDestination);

  // Misc

  virtual void FlushPlatform() override;

  // Debug helper functions

  virtual void PushMarkerPlatform(const char* szMarker) override;
  virtual void PopMarkerPlatform() override;
  virtual void InsertEventMarkerPlatform(const char* szMarker) override;

  // plGALCommandEncoderRenderPlatformInterface
  void BeginRendering(const plGALRenderingSetup& renderingSetup);
  void EndRendering();

  // Draw functions

  virtual void ClearPlatform(const plColor& ClearColor, plUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, plUInt8 uiStencilClear) override;

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
  virtual void SetPrimitiveTopologyPlatform(plGALPrimitiveTopology::Enum Topology) override;

  virtual void SetBlendStatePlatform(const plGALBlendState* pBlendState, const plColor& BlendFactor, plUInt32 uiSampleMask) override;
  virtual void SetDepthStencilStatePlatform(const plGALDepthStencilState* pDepthStencilState, plUInt8 uiStencilRefValue) override;
  virtual void SetRasterizerStatePlatform(const plGALRasterizerState* pRasterizerState) override;

  virtual void SetViewportPlatform(const plRectFloat& rect, float fMinDepth, float fMaxDepth) override;
  virtual void SetScissorRectPlatform(const plRectU32& rect) override;

  // plGALCommandEncoderComputePlatformInterface
  // Dispatch
  void BeginCompute();
  void EndCompute();

  virtual void DispatchPlatform(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ) override;
  virtual void DispatchIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes) override;

private:
  void FlushDeferredStateChanges();

  plGALDeviceVulkan& m_GALDeviceVulkan;
  vk::Device m_vkDevice;

  vk::CommandBuffer* m_pCommandBuffer = nullptr;
  plPipelineBarrierVulkan* m_pPipelineBarrier = nullptr;


  // Cache flags.
  bool m_bPipelineStateDirty = true;
  bool m_bViewportDirty = true;
  bool m_bIndexBufferDirty = false;
  bool m_bDescriptorsDirty = false;
  plGAL::ModifiedRange m_BoundVertexBuffersRange;
  bool m_bRenderPassActive = false; ///< #TODO_VULKAN Disabling and re-enabling the render pass is buggy as we might execute a clear twice.
  bool m_bClearSubmitted = false; ///< Start render pass is lazy so if no draw call is executed we need to make sure the clear is executed anyways.
  bool m_bInsideCompute = false;  ///< Within BeginCompute / EndCompute block.


  // Bound objects for deferred state flushes
  plResourceCacheVulkan::PipelineLayoutDesc m_LayoutDesc;
  plResourceCacheVulkan::GraphicsPipelineDesc m_PipelineDesc;
  plResourceCacheVulkan::ComputePipelineDesc m_ComputeDesc;
  vk::Framebuffer m_frameBuffer;
  vk::RenderPassBeginInfo m_renderPass;
  plHybridArray<vk::ClearValue, PLASMA_GAL_MAX_RENDERTARGET_COUNT + 1> m_clearValues;
  vk::ImageAspectFlags m_depthMask = {};
  plUInt32 m_uiLayers = 0;

  vk::Viewport m_viewport;
  vk::Rect2D m_scissor;
  bool m_bScissorEnabled = false;

  const plGALRenderTargetView* m_pBoundRenderTargets[PLASMA_GAL_MAX_RENDERTARGET_COUNT] = {};
  const plGALRenderTargetView* m_pBoundDepthStencilTarget = nullptr;
  plUInt32 m_uiBoundRenderTargetCount;

  const plGALBufferVulkan* m_pIndexBuffer = nullptr;
  vk::Buffer m_pBoundVertexBuffers[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT];
  vk::DeviceSize m_VertexBufferOffsets[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT] = {};

  const plGALBufferVulkan* m_pBoundConstantBuffers[PLASMA_GAL_MAX_CONSTANT_BUFFER_COUNT] = {};
  plHybridArray<const plGALResourceViewVulkan*, 16> m_pBoundShaderResourceViews;
  plHybridArray<const plGALUnorderedAccessViewVulkan*, 16> m_pBoundUnoderedAccessViews;
  const plGALSamplerStateVulkan* m_pBoundSamplerStates[PLASMA_GAL_MAX_SAMPLER_COUNT] = {};

  plHybridArray<vk::WriteDescriptorSet, 16> m_DescriptorWrites;
  plHybridArray<vk::DescriptorSet, 4> m_DescriptorSets;
};
