#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/RendererDX11DLL.h>
#include <RendererDX11/State/StateDX11.h>

#include <d3d11.h>
#include <d3d11_3.h>


// Mapping tables to map plGAL constants to DX11 constants
#include <RendererDX11/State/Implementation/StateDX11_MappingTables.inl>

// Blend state

plGALBlendStateDX11::plGALBlendStateDX11(const plGALBlendStateCreationDescription& Description)
  : plGALBlendState(Description)

{
}

plGALBlendStateDX11::~plGALBlendStateDX11() = default;

static D3D11_BLEND_OP ToD3DBlendOp(plGALBlendOp::Enum e)
{
  switch (e)
  {
    case plGALBlendOp::Add:
      return D3D11_BLEND_OP_ADD;
    case plGALBlendOp::Max:
      return D3D11_BLEND_OP_MAX;
    case plGALBlendOp::Min:
      return D3D11_BLEND_OP_MIN;
    case plGALBlendOp::RevSubtract:
      return D3D11_BLEND_OP_REV_SUBTRACT;
    case plGALBlendOp::Subtract:
      return D3D11_BLEND_OP_SUBTRACT;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_OP_ADD;
}

static D3D11_BLEND ToD3DBlend(plGALBlend::Enum e)
{
  switch (e)
  {
    case plGALBlend::BlendFactor:
      PL_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in plGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_BLEND_FACTOR;
    case plGALBlend::DestAlpha:
      return D3D11_BLEND_DEST_ALPHA;
    case plGALBlend::DestColor:
      return D3D11_BLEND_DEST_COLOR;
    case plGALBlend::InvBlendFactor:
      PL_ASSERT_NOT_IMPLEMENTED;
      // if this is used, it also must be implemented in plGALContextDX11::SetBlendStatePlatform
      return D3D11_BLEND_INV_BLEND_FACTOR;
    case plGALBlend::InvDestAlpha:
      return D3D11_BLEND_INV_DEST_ALPHA;
    case plGALBlend::InvDestColor:
      return D3D11_BLEND_INV_DEST_COLOR;
    case plGALBlend::InvSrcAlpha:
      return D3D11_BLEND_INV_SRC_ALPHA;
    case plGALBlend::InvSrcColor:
      return D3D11_BLEND_INV_SRC_COLOR;
    case plGALBlend::One:
      return D3D11_BLEND_ONE;
    case plGALBlend::SrcAlpha:
      return D3D11_BLEND_SRC_ALPHA;
    case plGALBlend::SrcAlphaSaturated:
      return D3D11_BLEND_SRC_ALPHA_SAT;
    case plGALBlend::SrcColor:
      return D3D11_BLEND_SRC_COLOR;
    case plGALBlend::Zero:
      return D3D11_BLEND_ZERO;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  return D3D11_BLEND_ONE;
}

plResult plGALBlendStateDX11::InitPlatform(plGALDevice* pDevice)
{
  D3D11_BLEND_DESC DXDesc;
  DXDesc.AlphaToCoverageEnable = m_Description.m_bAlphaToCoverage;
  DXDesc.IndependentBlendEnable = m_Description.m_bIndependentBlend;

  for (plInt32 i = 0; i < 8; ++i)
  {
    DXDesc.RenderTarget[i].BlendEnable = m_Description.m_RenderTargetBlendDescriptions[i].m_bBlendingEnabled;
    DXDesc.RenderTarget[i].BlendOp = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOp);
    DXDesc.RenderTarget[i].BlendOpAlpha = ToD3DBlendOp(m_Description.m_RenderTargetBlendDescriptions[i].m_BlendOpAlpha);
    DXDesc.RenderTarget[i].DestBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlend);
    DXDesc.RenderTarget[i].DestBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_DestBlendAlpha);
    DXDesc.RenderTarget[i].SrcBlend = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlend);
    DXDesc.RenderTarget[i].SrcBlendAlpha = ToD3DBlend(m_Description.m_RenderTargetBlendDescriptions[i].m_SourceBlendAlpha);
    DXDesc.RenderTarget[i].RenderTargetWriteMask = m_Description.m_RenderTargetBlendDescriptions[i].m_uiWriteMask &
                                                   0x0F; // D3D11: RenderTargetWriteMask can only have the least significant 4 bits set.
  }

  if (FAILED(static_cast<plGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateBlendState(&DXDesc, &m_pDXBlendState)))
  {
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plGALBlendStateDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXBlendState);
  return PL_SUCCESS;
}

// Depth Stencil state

plGALDepthStencilStateDX11::plGALDepthStencilStateDX11(const plGALDepthStencilStateCreationDescription& Description)
  : plGALDepthStencilState(Description)

{
}

plGALDepthStencilStateDX11::~plGALDepthStencilStateDX11() = default;

plResult plGALDepthStencilStateDX11::InitPlatform(plGALDevice* pDevice)
{
  D3D11_DEPTH_STENCIL_DESC DXDesc;
  DXDesc.DepthEnable = m_Description.m_bDepthTest;
  DXDesc.DepthWriteMask = m_Description.m_bDepthWrite ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
  DXDesc.DepthFunc = GALCompareFuncToDX11[m_Description.m_DepthTestFunc];
  DXDesc.StencilEnable = m_Description.m_bStencilTest;
  DXDesc.StencilReadMask = m_Description.m_uiStencilReadMask;
  DXDesc.StencilWriteMask = m_Description.m_uiStencilWriteMask;

  DXDesc.FrontFace.StencilFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_FailOp];
  DXDesc.FrontFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_DepthFailOp];
  DXDesc.FrontFace.StencilPassOp = GALStencilOpTableIndexToDX11[m_Description.m_FrontFaceStencilOp.m_PassOp];
  DXDesc.FrontFace.StencilFunc = GALCompareFuncToDX11[m_Description.m_FrontFaceStencilOp.m_StencilFunc];

  const plGALStencilOpDescription& backFaceStencilOp =
    m_Description.m_bSeparateFrontAndBack ? m_Description.m_BackFaceStencilOp : m_Description.m_FrontFaceStencilOp;
  DXDesc.BackFace.StencilFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_FailOp];
  DXDesc.BackFace.StencilDepthFailOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_DepthFailOp];
  DXDesc.BackFace.StencilPassOp = GALStencilOpTableIndexToDX11[backFaceStencilOp.m_PassOp];
  DXDesc.BackFace.StencilFunc = GALCompareFuncToDX11[backFaceStencilOp.m_StencilFunc];


  if (FAILED(static_cast<plGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateDepthStencilState(&DXDesc, &m_pDXDepthStencilState)))
  {
    return PL_FAILURE;
  }
  else
  {
    return PL_SUCCESS;
  }
}

plResult plGALDepthStencilStateDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXDepthStencilState);
  return PL_SUCCESS;
}


// Rasterizer state

plGALRasterizerStateDX11::plGALRasterizerStateDX11(const plGALRasterizerStateCreationDescription& Description)
  : plGALRasterizerState(Description)

{
}

plGALRasterizerStateDX11::~plGALRasterizerStateDX11() = default;



plResult plGALRasterizerStateDX11::InitPlatform(plGALDevice* pDevice)
{
  const bool NeedsStateDesc2 = m_Description.m_bConservativeRasterization;

  if (NeedsStateDesc2)
  {
    D3D11_RASTERIZER_DESC2 DXDesc2;
    DXDesc2.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc2.DepthBias = m_Description.m_iDepthBias;
    DXDesc2.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc2.DepthClipEnable = TRUE;
    DXDesc2.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc2.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc2.MultisampleEnable = TRUE;
    DXDesc2.AntialiasedLineEnable = TRUE;
    DXDesc2.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc2.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;
    DXDesc2.ConservativeRaster =
      m_Description.m_bConservativeRasterization ? D3D11_CONSERVATIVE_RASTERIZATION_MODE_ON : D3D11_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    DXDesc2.ForcedSampleCount = 0;

    if (!pDevice->GetCapabilities().m_bConservativeRasterization && m_Description.m_bConservativeRasterization)
    {
      plLog::Error("Rasterizer state description enables conservative rasterization which is not available!");
      return PL_FAILURE;
    }

    ID3D11RasterizerState2* pDXRasterizerState2 = nullptr;

    if (FAILED(static_cast<plGALDeviceDX11*>(pDevice)->GetDXDevice3()->CreateRasterizerState2(&DXDesc2, &pDXRasterizerState2)))
    {
      return PL_FAILURE;
    }
    else
    {
      m_pDXRasterizerState = pDXRasterizerState2;
      return PL_SUCCESS;
    }
  }
  else
  {
    D3D11_RASTERIZER_DESC DXDesc;
    DXDesc.CullMode = GALCullModeToDX11[m_Description.m_CullMode];
    DXDesc.DepthBias = m_Description.m_iDepthBias;
    DXDesc.DepthBiasClamp = m_Description.m_fDepthBiasClamp;
    DXDesc.DepthClipEnable = TRUE;
    DXDesc.FillMode = m_Description.m_bWireFrame ? D3D11_FILL_WIREFRAME : D3D11_FILL_SOLID;
    DXDesc.FrontCounterClockwise = m_Description.m_bFrontCounterClockwise;
    DXDesc.MultisampleEnable = TRUE;
    DXDesc.AntialiasedLineEnable = TRUE;
    DXDesc.ScissorEnable = m_Description.m_bScissorTest;
    DXDesc.SlopeScaledDepthBias = m_Description.m_fSlopeScaledDepthBias;

    if (FAILED(static_cast<plGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateRasterizerState(&DXDesc, &m_pDXRasterizerState)))
    {
      return PL_FAILURE;
    }
    else
    {
      return PL_SUCCESS;
    }
  }
}


plResult plGALRasterizerStateDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXRasterizerState);
  return PL_SUCCESS;
}


// Sampler state

plGALSamplerStateDX11::plGALSamplerStateDX11(const plGALSamplerStateCreationDescription& Description)
  : plGALSamplerState(Description)

{
}

plGALSamplerStateDX11::~plGALSamplerStateDX11() = default;

/*
 */

plResult plGALSamplerStateDX11::InitPlatform(plGALDevice* pDevice)
{
  D3D11_SAMPLER_DESC DXDesc;
  DXDesc.AddressU = GALTextureAddressModeToDX11[m_Description.m_AddressU];
  DXDesc.AddressV = GALTextureAddressModeToDX11[m_Description.m_AddressV];
  DXDesc.AddressW = GALTextureAddressModeToDX11[m_Description.m_AddressW];
  DXDesc.BorderColor[0] = m_Description.m_BorderColor.r;
  DXDesc.BorderColor[1] = m_Description.m_BorderColor.g;
  DXDesc.BorderColor[2] = m_Description.m_BorderColor.b;
  DXDesc.BorderColor[3] = m_Description.m_BorderColor.a;
  DXDesc.ComparisonFunc = GALCompareFuncToDX11[m_Description.m_SampleCompareFunc];

  if (m_Description.m_MagFilter == plGALTextureFilterMode::Anisotropic || m_Description.m_MinFilter == plGALTextureFilterMode::Anisotropic ||
      m_Description.m_MipFilter == plGALTextureFilterMode::Anisotropic)
  {
    if (m_Description.m_SampleCompareFunc == plGALCompareFunc::Never)
      DXDesc.Filter = D3D11_FILTER_ANISOTROPIC;
    else
      DXDesc.Filter = D3D11_FILTER_COMPARISON_ANISOTROPIC;
  }
  else
  {
    plUInt32 uiTableIndex = 0;

    if (m_Description.m_MipFilter == plGALTextureFilterMode::Linear)
      uiTableIndex |= 1;

    if (m_Description.m_MagFilter == plGALTextureFilterMode::Linear)
      uiTableIndex |= 2;

    if (m_Description.m_MinFilter == plGALTextureFilterMode::Linear)
      uiTableIndex |= 4;

    if (m_Description.m_SampleCompareFunc != plGALCompareFunc::Never)
      uiTableIndex |= 8;

    DXDesc.Filter = GALFilterTableIndexToDX11[uiTableIndex];
  }

  DXDesc.MaxAnisotropy = m_Description.m_uiMaxAnisotropy;
  DXDesc.MaxLOD = m_Description.m_fMaxMip;
  DXDesc.MinLOD = m_Description.m_fMinMip;
  DXDesc.MipLODBias = m_Description.m_fMipLodBias;

  if (FAILED(static_cast<plGALDeviceDX11*>(pDevice)->GetDXDevice()->CreateSamplerState(&DXDesc, &m_pDXSamplerState)))
  {
    return PL_FAILURE;
  }
  else
  {
    return PL_SUCCESS;
  }
}


plResult plGALSamplerStateDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXSamplerState);
  return PL_SUCCESS;
}



PL_STATICLINK_FILE(RendererDX11, RendererDX11_State_Implementation_StateDX11);
