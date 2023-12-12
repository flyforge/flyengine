#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/State/State.h>

plGALBlendState::plGALBlendState(const plGALBlendStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALBlendState::~plGALBlendState() {}



plGALDepthStencilState::plGALDepthStencilState(const plGALDepthStencilStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALDepthStencilState::~plGALDepthStencilState() {}



plGALRasterizerState::plGALRasterizerState(const plGALRasterizerStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALRasterizerState::~plGALRasterizerState() {}


plGALSamplerState::plGALSamplerState(const plGALSamplerStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALSamplerState::~plGALSamplerState() {}



PLASMA_STATICLINK_FILE(RendererFoundation, RendererFoundation_State_Implementation_State);
