
#pragma once

#include <Foundation/Math/Rect.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

class PL_RENDERERFOUNDATION_DLL plGALRenderCommandEncoder : public plGALCommandEncoder
{
public:
  plGALRenderCommandEncoder(plGALDevice& ref_device, plGALCommandEncoderRenderState& ref_renderState, plGALCommandEncoderCommonPlatformInterface& ref_commonImpl, plGALCommandEncoderRenderPlatformInterface& ref_renderImpl);
  virtual ~plGALRenderCommandEncoder();

  // Draw functions

  /// \brief Clears active rendertargets.
  ///
  /// \param uiRenderTargetClearMask
  ///   Each bit represents a bound color target. If all bits are set, all bound color targets will be cleared.
  void Clear(const plColor& clearColor, plUInt32 uiRenderTargetClearMask = 0xFFFFFFFFu, bool bClearDepth = true, bool bClearStencil = true, float fDepthClear = 1.0f, plUInt8 uiStencilClear = 0x0u);

  plResult Draw(plUInt32 uiVertexCount, plUInt32 uiStartVertex);
  plResult DrawIndexed(plUInt32 uiIndexCount, plUInt32 uiStartIndex);
  plResult DrawIndexedInstanced(plUInt32 uiIndexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartIndex);
  plResult DrawIndexedInstancedIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes);
  plResult DrawInstanced(plUInt32 uiVertexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartVertex);
  plResult DrawInstancedIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes);

  // State functions

  void SetIndexBuffer(plGALBufferHandle hIndexBuffer);
  void SetVertexBuffer(plUInt32 uiSlot, plGALBufferHandle hVertexBuffer);
  void SetVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration);

  plGALPrimitiveTopology::Enum GetPrimitiveTopology() const { return m_RenderState.m_Topology; }
  void SetPrimitiveTopology(plGALPrimitiveTopology::Enum topology);

  void SetBlendState(plGALBlendStateHandle hBlendState, const plColor& blendFactor = plColor::White, plUInt32 uiSampleMask = 0xFFFFFFFFu);
  void SetDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState, plUInt8 uiStencilRefValue = 0xFFu);
  void SetRasterizerState(plGALRasterizerStateHandle hRasterizerState);

  void SetViewport(const plRectFloat& rect, float fMinDepth = 0.0f, float fMaxDepth = 1.0f);
  void SetScissorRect(const plRectU32& rect);

  virtual void ClearStatisticsCounters() override;

private:
  void CountDrawCall() { m_uiDrawCalls++; }

  // Statistic variables
  plUInt32 m_uiDrawCalls = 0;

  plGALCommandEncoderRenderState& m_RenderState;

  plGALCommandEncoderRenderPlatformInterface& m_RenderImpl;
};
