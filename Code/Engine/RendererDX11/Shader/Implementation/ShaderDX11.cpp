#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>

#include <d3d11.h>

plGALShaderDX11::plGALShaderDX11(const plGALShaderCreationDescription& Description)
  : plGALShader(Description)
  , m_pVertexShader(nullptr)
  , m_pHullShader(nullptr)
  , m_pDomainShader(nullptr)
  , m_pGeometryShader(nullptr)
  , m_pPixelShader(nullptr)
  , m_pComputeShader(nullptr)
{
}

plGALShaderDX11::~plGALShaderDX11() {}

void plGALShaderDX11::SetDebugName(const char* szName) const
{
  plUInt32 uiLength = plStringUtils::GetStringElementCount(szName);

  if (m_pVertexShader != nullptr)
  {
    m_pVertexShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pHullShader != nullptr)
  {
    m_pHullShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pDomainShader != nullptr)
  {
    m_pDomainShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pGeometryShader != nullptr)
  {
    m_pGeometryShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pPixelShader != nullptr)
  {
    m_pPixelShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }

  if (m_pComputeShader != nullptr)
  {
    m_pComputeShader->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

plResult plGALShaderDX11::InitPlatform(plGALDevice* pDevice)
{
  PLASMA_SUCCEED_OR_RETURN(CreateBindingMapping());

  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);
  ID3D11Device* pD3D11Device = pDXDevice->GetDXDevice();

  if (m_Description.HasByteCodeForStage(plGALShaderStage::VertexShader))
  {
    if (FAILED(pD3D11Device->CreateVertexShader(m_Description.m_ByteCodes[plGALShaderStage::VertexShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::VertexShader]->GetSize(), nullptr, &m_pVertexShader)))
    {
      plLog::Error("Couldn't create native vertex shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(plGALShaderStage::HullShader))
  {
    if (FAILED(pD3D11Device->CreateHullShader(m_Description.m_ByteCodes[plGALShaderStage::HullShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::HullShader]->GetSize(), nullptr, &m_pHullShader)))
    {
      plLog::Error("Couldn't create native hull shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(plGALShaderStage::DomainShader))
  {
    if (FAILED(pD3D11Device->CreateDomainShader(m_Description.m_ByteCodes[plGALShaderStage::DomainShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::DomainShader]->GetSize(), nullptr, &m_pDomainShader)))
    {
      plLog::Error("Couldn't create native domain shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(plGALShaderStage::GeometryShader))
  {
    if (FAILED(pD3D11Device->CreateGeometryShader(m_Description.m_ByteCodes[plGALShaderStage::GeometryShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::GeometryShader]->GetSize(), nullptr, &m_pGeometryShader)))
    {
      plLog::Error("Couldn't create native geometry shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(plGALShaderStage::PixelShader))
  {
    if (FAILED(pD3D11Device->CreatePixelShader(m_Description.m_ByteCodes[plGALShaderStage::PixelShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::PixelShader]->GetSize(), nullptr, &m_pPixelShader)))
    {
      plLog::Error("Couldn't create native pixel shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }

  if (m_Description.HasByteCodeForStage(plGALShaderStage::ComputeShader))
  {
    if (FAILED(pD3D11Device->CreateComputeShader(m_Description.m_ByteCodes[plGALShaderStage::ComputeShader]->GetByteCode(),
          m_Description.m_ByteCodes[plGALShaderStage::ComputeShader]->GetSize(), nullptr, &m_pComputeShader)))
    {
      plLog::Error("Couldn't create native compute shader from bytecode!");
      return PLASMA_FAILURE;
    }
  }


  return PLASMA_SUCCESS;
}

plResult plGALShaderDX11::DeInitPlatform(plGALDevice* pDevice)
{
  DestroyBindingMapping();
  PLASMA_GAL_DX11_RELEASE(m_pVertexShader);
  PLASMA_GAL_DX11_RELEASE(m_pHullShader);
  PLASMA_GAL_DX11_RELEASE(m_pDomainShader);
  PLASMA_GAL_DX11_RELEASE(m_pGeometryShader);
  PLASMA_GAL_DX11_RELEASE(m_pPixelShader);
  PLASMA_GAL_DX11_RELEASE(m_pComputeShader);

  return PLASMA_SUCCESS;
}

PLASMA_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_ShaderDX11);
