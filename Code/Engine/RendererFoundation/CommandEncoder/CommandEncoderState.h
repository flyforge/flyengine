
#pragma once

#include <Foundation/Math/Color.h>
#include <Foundation/Math/Rect.h>
#include <RendererFoundation/RendererFoundationDLL.h>

struct PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoderState
{
  virtual void InvalidateState();

  plGALShaderHandle m_hShader;

  plGALBufferHandle m_hConstantBuffers[PLASMA_GAL_MAX_CONSTANT_BUFFER_COUNT];

  plHybridArray<plGALResourceViewHandle, 16> m_hResourceViews[plGALShaderStage::ENUM_COUNT];
  plHybridArray<const plGALResourceBase*, 16> m_pResourcesForResourceViews[plGALShaderStage::ENUM_COUNT];

  plHybridArray<plGALUnorderedAccessViewHandle, 16> m_hUnorderedAccessViews;
  plHybridArray<const plGALResourceBase*, 16> m_pResourcesForUnorderedAccessViews;

  plGALSamplerStateHandle m_hSamplerStates[plGALShaderStage::ENUM_COUNT][PLASMA_GAL_MAX_SAMPLER_COUNT];
};

struct PLASMA_RENDERERFOUNDATION_DLL plGALCommandEncoderRenderState : public plGALCommandEncoderState
{
  virtual void InvalidateState() override;

  plGALBufferHandle m_hVertexBuffers[PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT];
  plGALBufferHandle m_hIndexBuffer;

  plGALVertexDeclarationHandle m_hVertexDeclaration;
  plGALPrimitiveTopology::Enum m_Topology = plGALPrimitiveTopology::ENUM_COUNT;

  plGALBlendStateHandle m_hBlendState;
  plColor m_BlendFactor = plColor(0, 0, 0, 0);
  plUInt32 m_uiSampleMask = 0;

  plGALDepthStencilStateHandle m_hDepthStencilState;
  plUInt8 m_uiStencilRefValue = 0;

  plGALRasterizerStateHandle m_hRasterizerState;

  plRectU32 m_ScissorRect = plRectU32(0xFFFFFFFF, 0xFFFFFFFF, 0, 0);
  plRectFloat m_ViewPortRect = plRectFloat(plMath::MaxValue<float>(), plMath::MaxValue<float>(), 0.0f, 0.0f);
  float m_fViewPortMinDepth = plMath::MaxValue<float>();
  float m_fViewPortMaxDepth = -plMath::MaxValue<float>();
};
