#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/RendererFoundationDLL.h>
#include <RendererFoundation/State/State.h>

plGALBlendState::plGALBlendState(const plGALBlendStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALBlendState::~plGALBlendState() = default;



plGALDepthStencilState::plGALDepthStencilState(const plGALDepthStencilStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALDepthStencilState::~plGALDepthStencilState() = default;



plGALRasterizerState::plGALRasterizerState(const plGALRasterizerStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALRasterizerState::~plGALRasterizerState() = default;


plGALSamplerState::plGALSamplerState(const plGALSamplerStateCreationDescription& Description)
  : plGALObject(Description)
{
}

plGALSamplerState::~plGALSamplerState() = default;


