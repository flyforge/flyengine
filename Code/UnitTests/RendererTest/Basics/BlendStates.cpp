#include <RendererTest/RendererTestPCH.h>

#include "Basics.h"

plTestAppRun plRendererTestBasics::SubtestBlendStates()
{
  BeginFrame();

  plGALBlendStateHandle hState;

  plGALBlendStateCreationDescription StateDesc;
  StateDesc.m_RenderTargetBlendDescriptions[0].m_bBlendingEnabled = true;

  if (m_iFrame == 0)
  {
    // StateDesc.m_RenderTargetBlendDescriptions[0].
  }

  if (m_iFrame == 1)
  {
    StateDesc.m_RenderTargetBlendDescriptions[0].m_SourceBlend = plGALBlend::SrcAlpha;
    StateDesc.m_RenderTargetBlendDescriptions[0].m_DestBlend = plGALBlend::InvSrcAlpha;
  }

  plColor clear(0, 0, 0, 0);
  // if (StateDesc.m_bDepthClip)
  //  clear.r = 0.5f;
  // if (StateDesc.m_bFrontCounterClockwise)
  //  clear.g = 0.5f;
  // if (StateDesc.m_CullMode == plGALCullMode::Front)
  //  clear.b = 0.5f;
  // if (StateDesc.m_CullMode == plGALCullMode::Back)
  //  clear.b = 1.0f;

  ClearScreen(clear);

  hState = m_pDevice->CreateBlendState(StateDesc);
  PLASMA_ASSERT_DEV(!hState.IsInvalidated(), "Couldn't create blend state!");

  plRenderContext::GetDefaultInstance()->GetRenderCommandEncoder()->SetBlendState(hState);

  RenderObjects(plShaderBindFlags::NoBlendState);

  PLASMA_TEST_IMAGE(m_iFrame, 150);

  EndFrame();

  m_pDevice->DestroyBlendState(hState);

  return m_iFrame < 1 ? plTestAppRun::Continue : plTestAppRun::Quit;
}
