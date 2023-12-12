#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoderState.h>

void plGALCommandEncoderState::InvalidateState()
{
  m_hShader = plGALShaderHandle();

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_hConstantBuffers); ++i)
  {
    m_hConstantBuffers[i].Invalidate();
  }

  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; ++i)
  {
    m_hResourceViews[i].Clear();
    m_pResourcesForResourceViews[i].Clear();
  }

  m_hUnorderedAccessViews.Clear();
  m_pResourcesForUnorderedAccessViews.Clear();

  for (plUInt32 i = 0; i < plGALShaderStage::ENUM_COUNT; ++i)
  {
    for (plUInt32 j = 0; j < PLASMA_GAL_MAX_SAMPLER_COUNT; j++)
    {
      m_hSamplerStates[i][j].Invalidate();
    }
  }
}

void plGALCommandEncoderRenderState::InvalidateState()
{
  plGALCommandEncoderState::InvalidateState();

  for (plUInt32 i = 0; i < PLASMA_ARRAY_SIZE(m_hVertexBuffers); ++i)
  {
    m_hVertexBuffers[i].Invalidate();
  }
  m_hIndexBuffer.Invalidate();

  m_hVertexDeclaration.Invalidate();
  m_Topology = plGALPrimitiveTopology::ENUM_COUNT;

  m_hBlendState.Invalidate();
  m_BlendFactor = plColor(0, 0, 0, 0);
  m_uiSampleMask = 0;

  m_hDepthStencilState.Invalidate();
  m_uiStencilRefValue = 0;

  m_hRasterizerState.Invalidate();

  m_ScissorRect = plRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  m_ViewPortRect = plRectFloat(plMath::MaxValue<float>(), plMath::MaxValue<float>(), 0.0f, 0.0f);
  m_fViewPortMinDepth = plMath::MaxValue<float>();
  m_fViewPortMaxDepth = -plMath::MaxValue<float>();
}


PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_CommandEncoder_Implementation_CommandEncoderState);
