#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/RenderCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

plGALRenderCommandEncoder::plGALRenderCommandEncoder(plGALDevice& ref_device, plGALCommandEncoderRenderState& ref_renderState, plGALCommandEncoderCommonPlatformInterface& ref_commonImpl, plGALCommandEncoderRenderPlatformInterface& ref_renderImpl)
  : plGALCommandEncoder(ref_device, ref_renderState, ref_commonImpl)
  , m_RenderState(ref_renderState)
  , m_RenderImpl(ref_renderImpl)
{
}

plGALRenderCommandEncoder::~plGALRenderCommandEncoder() = default;

void plGALRenderCommandEncoder::Clear(const plColor& clearColor, plUInt32 uiRenderTargetClearMask /*= 0xFFFFFFFFu*/, bool bClearDepth /*= true*/, bool bClearStencil /*= true*/, float fDepthClear /*= 1.0f*/, plUInt8 uiStencilClear /*= 0x0u*/)
{
  AssertRenderingThread();
  m_RenderImpl.ClearPlatform(clearColor, uiRenderTargetClearMask, bClearDepth, bClearStencil, fDepthClear, uiStencilClear);
}

plResult plGALRenderCommandEncoder::Draw(plUInt32 uiVertexCount, plUInt32 uiStartVertex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawPlatform(uiVertexCount, uiStartVertex);
}

plResult plGALRenderCommandEncoder::DrawIndexed(plUInt32 uiIndexCount, plUInt32 uiStartIndex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawIndexedPlatform(uiIndexCount, uiStartIndex);
}

plResult plGALRenderCommandEncoder::DrawIndexedInstanced(plUInt32 uiIndexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartIndex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawIndexedInstancedPlatform(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex);
}

plResult plGALRenderCommandEncoder::DrawIndexedInstancedIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  PL_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_RenderImpl.DrawIndexedInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

plResult plGALRenderCommandEncoder::DrawInstanced(plUInt32 uiVertexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartVertex)
{
  AssertRenderingThread();
  CountDrawCall();
  return m_RenderImpl.DrawInstancedPlatform(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex);
}

plResult plGALRenderCommandEncoder::DrawInstancedIndirect(plGALBufferHandle hIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  AssertRenderingThread();
  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hIndirectArgumentBuffer);
  PL_ASSERT_DEV(pBuffer != nullptr, "Invalid buffer handle for indirect arguments!");
  CountDrawCall();
  return m_RenderImpl.DrawInstancedIndirectPlatform(pBuffer, uiArgumentOffsetInBytes);
}

void plGALRenderCommandEncoder::SetIndexBuffer(plGALBufferHandle hIndexBuffer)
{
  if (m_RenderState.m_hIndexBuffer == hIndexBuffer)
  {
    return;
  }

  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hIndexBuffer);
  /// \todo Assert on index buffer type (if non nullptr)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetIndexBufferPlatform(pBuffer);

  m_RenderState.m_hIndexBuffer = hIndexBuffer;
}

void plGALRenderCommandEncoder::SetVertexBuffer(plUInt32 uiSlot, plGALBufferHandle hVertexBuffer)
{
  if (m_RenderState.m_hVertexBuffers[uiSlot] == hVertexBuffer)
  {
    return;
  }

  const plGALBuffer* pBuffer = GetDevice().GetBuffer(hVertexBuffer);
  // Assert on vertex buffer type (if non-zero)
  // Note that GL4 can bind arbitrary buffer to arbitrary binding points (index/vertex/transform-feedback/indirect-draw/...)

  m_RenderImpl.SetVertexBufferPlatform(uiSlot, pBuffer);

  m_RenderState.m_hVertexBuffers[uiSlot] = hVertexBuffer;
}

void plGALRenderCommandEncoder::SetPrimitiveTopology(plGALPrimitiveTopology::Enum topology)
{
  AssertRenderingThread();

  if (m_RenderState.m_Topology == topology)
  {
    return;
  }

  m_RenderImpl.SetPrimitiveTopologyPlatform(topology);

  m_RenderState.m_Topology = topology;
}

void plGALRenderCommandEncoder::SetVertexDeclaration(plGALVertexDeclarationHandle hVertexDeclaration)
{
  AssertRenderingThread();

  if (m_RenderState.m_hVertexDeclaration == hVertexDeclaration)
  {
    return;
  }

  const plGALVertexDeclaration* pVertexDeclaration = GetDevice().GetVertexDeclaration(hVertexDeclaration);
  // Assert on vertex buffer type (if non-zero)

  m_RenderImpl.SetVertexDeclarationPlatform(pVertexDeclaration);

  m_RenderState.m_hVertexDeclaration = hVertexDeclaration;
}

void plGALRenderCommandEncoder::SetBlendState(plGALBlendStateHandle hBlendState, const plColor& blendFactor, plUInt32 uiSampleMask)
{
  AssertRenderingThread();

  if (m_RenderState.m_hBlendState == hBlendState && m_RenderState.m_BlendFactor.IsEqualRGBA(blendFactor, 0.001f) && m_RenderState.m_uiSampleMask == uiSampleMask)
  {
    return;
  }

  const plGALBlendState* pBlendState = GetDevice().GetBlendState(hBlendState);

  m_RenderImpl.SetBlendStatePlatform(pBlendState, blendFactor, uiSampleMask);

  m_RenderState.m_hBlendState = hBlendState;
  m_RenderState.m_BlendFactor = blendFactor;
  m_RenderState.m_uiSampleMask = uiSampleMask;
}

void plGALRenderCommandEncoder::SetDepthStencilState(plGALDepthStencilStateHandle hDepthStencilState, plUInt8 uiStencilRefValue /*= 0xFFu*/)
{
  AssertRenderingThread();

  if (m_RenderState.m_hDepthStencilState == hDepthStencilState && m_RenderState.m_uiStencilRefValue == uiStencilRefValue)
  {
    return;
  }

  const plGALDepthStencilState* pDepthStencilState = GetDevice().GetDepthStencilState(hDepthStencilState);

  m_RenderImpl.SetDepthStencilStatePlatform(pDepthStencilState, uiStencilRefValue);

  m_RenderState.m_hDepthStencilState = hDepthStencilState;
  m_RenderState.m_uiStencilRefValue = uiStencilRefValue;
}

void plGALRenderCommandEncoder::SetRasterizerState(plGALRasterizerStateHandle hRasterizerState)
{
  AssertRenderingThread();

  if (m_RenderState.m_hRasterizerState == hRasterizerState)
  {
    return;
  }

  const plGALRasterizerState* pRasterizerState = GetDevice().GetRasterizerState(hRasterizerState);

  m_RenderImpl.SetRasterizerStatePlatform(pRasterizerState);

  m_RenderState.m_hRasterizerState = hRasterizerState;
}

void plGALRenderCommandEncoder::SetViewport(const plRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  AssertRenderingThread();

  if (m_RenderState.m_ViewPortRect == rect && m_RenderState.m_fViewPortMinDepth == fMinDepth && m_RenderState.m_fViewPortMaxDepth == fMaxDepth)
  {
    return;
  }

  m_RenderImpl.SetViewportPlatform(rect, fMinDepth, fMaxDepth);

  m_RenderState.m_ViewPortRect = rect;
  m_RenderState.m_fViewPortMinDepth = fMinDepth;
  m_RenderState.m_fViewPortMaxDepth = fMaxDepth;
}

void plGALRenderCommandEncoder::SetScissorRect(const plRectU32& rect)
{
  AssertRenderingThread();

  if (m_RenderState.m_ScissorRect == rect)
  {
    return;
  }

  m_RenderImpl.SetScissorRectPlatform(rect);

  m_RenderState.m_ScissorRect = rect;
}

void plGALRenderCommandEncoder::ClearStatisticsCounters()
{
  plGALCommandEncoder::ClearStatisticsCounters();

  m_uiDrawCalls = 0;
}


