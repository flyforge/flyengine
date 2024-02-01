#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>

#include <d3d11.h>

plGALQueryDX11::plGALQueryDX11(const plGALQueryCreationDescription& Description)
  : plGALQuery(Description)

{
}

plGALQueryDX11::~plGALQueryDX11() = default;

plResult plGALQueryDX11::InitPlatform(plGALDevice* pDevice)
{
  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);

  D3D11_QUERY_DESC desc;
  if (m_Description.m_type == plGALQueryType::AnySamplesPassed)
    desc.MiscFlags = m_Description.m_bDrawIfUnknown ? D3D11_QUERY_MISC_PREDICATEHINT : 0;
  else
    desc.MiscFlags = 0;

  switch (m_Description.m_type)
  {
    case plGALQueryType::NumSamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION;
      break;
    case plGALQueryType::AnySamplesPassed:
      desc.Query = D3D11_QUERY_OCCLUSION_PREDICATE;
      break;
    default:
      PL_ASSERT_NOT_IMPLEMENTED;
  }

  if (SUCCEEDED(pDXDevice->GetDXDevice()->CreateQuery(&desc, &m_pDXQuery)))
  {
    return PL_SUCCESS;
  }
  else
  {
    plLog::Error("Creation of native DirectX query failed!");
    return PL_FAILURE;
  }
}

plResult plGALQueryDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXQuery);
  return PL_SUCCESS;
}

void plGALQueryDX11::SetDebugNamePlatform(const char* szName) const
{
  plUInt32 uiLength = plStringUtils::GetStringElementCount(szName);

  if (m_pDXQuery != nullptr)
  {
    m_pDXQuery->SetPrivateData(WKPDID_D3DDebugObjectName, uiLength, szName);
  }
}

PL_STATICLINK_FILE(RendererDX11, RendererDX11_Resources_Implementation_QueryDX11);
