#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererFoundation/Shader/Shader.h>

#include <d3d11.h>

plGALVertexDeclarationDX11::plGALVertexDeclarationDX11(const plGALVertexDeclarationCreationDescription& Description)
  : plGALVertexDeclaration(Description)

{
}

plGALVertexDeclarationDX11::~plGALVertexDeclarationDX11() = default;

static const char* GALSemanticToDX11[] = {"POSITION", "NORMAL", "TANGENT", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR", "COLOR",
  "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "TEXCOORD", "BITANGENT", "BONEINDICES",
  "BONEINDICES", "BONEWEIGHTS", "BONEWEIGHTS"};

static UINT GALSemanticToIndexDX11[] = {0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 1, 0, 1};

PL_CHECK_AT_COMPILETIME_MSG(PL_ARRAY_SIZE(GALSemanticToDX11) == plGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToDX11 array size does not match vertex attribute semantic count");
PL_CHECK_AT_COMPILETIME_MSG(PL_ARRAY_SIZE(GALSemanticToIndexDX11) == plGALVertexAttributeSemantic::ENUM_COUNT,
  "GALSemanticToIndexDX11 array size does not match vertex attribute semantic count");

PL_DEFINE_AS_POD_TYPE(D3D11_INPUT_ELEMENT_DESC);

plResult plGALVertexDeclarationDX11::InitPlatform(plGALDevice* pDevice)
{
  plHybridArray<D3D11_INPUT_ELEMENT_DESC, 8> DXInputElementDescs;

  plGALDeviceDX11* pDXDevice = static_cast<plGALDeviceDX11*>(pDevice);

  const plGALShader* pShader = pDevice->GetShader(m_Description.m_hShader);

  if (pShader == nullptr || !pShader->GetDescription().HasByteCodeForStage(plGALShaderStage::VertexShader))
  {
    return PL_FAILURE;
  }

  // Copy attribute descriptions
  for (plUInt32 i = 0; i < m_Description.m_VertexAttributes.GetCount(); i++)
  {
    const plGALVertexAttribute& Current = m_Description.m_VertexAttributes[i];

    D3D11_INPUT_ELEMENT_DESC DXDesc;
    DXDesc.AlignedByteOffset = Current.m_uiOffset;
    DXDesc.Format = pDXDevice->GetFormatLookupTable().GetFormatInfo(Current.m_eFormat).m_eVertexAttributeType;

    if (DXDesc.Format == DXGI_FORMAT_UNKNOWN)
    {
      plLog::Error("Vertex attribute format {0} of attribute at index {1} is unknown!", Current.m_eFormat, i);
      return PL_FAILURE;
    }

    DXDesc.InputSlot = Current.m_uiVertexBufferSlot;
    DXDesc.InputSlotClass = Current.m_bInstanceData ? D3D11_INPUT_PER_INSTANCE_DATA : D3D11_INPUT_PER_VERTEX_DATA;
    DXDesc.InstanceDataStepRate = Current.m_bInstanceData ? 1 : 0; /// \todo Expose step rate?
    DXDesc.SemanticIndex = GALSemanticToIndexDX11[Current.m_eSemantic];
    DXDesc.SemanticName = GALSemanticToDX11[Current.m_eSemantic];

    DXInputElementDescs.PushBack(DXDesc);
  }


  const plSharedPtr<const plGALShaderByteCode>& pByteCode = pShader->GetDescription().m_ByteCodes[plGALShaderStage::VertexShader];

  if (FAILED(pDXDevice->GetDXDevice()->CreateInputLayout(
        &DXInputElementDescs[0], DXInputElementDescs.GetCount(), pByteCode->GetByteCode(), pByteCode->GetSize(), &m_pDXInputLayout)))
  {
    return PL_FAILURE;
  }
  else
  {
    return PL_SUCCESS;
  }
}

plResult plGALVertexDeclarationDX11::DeInitPlatform(plGALDevice* pDevice)
{
  PL_GAL_DX11_RELEASE(m_pDXInputLayout);
  return PL_SUCCESS;
}



PL_STATICLINK_FILE(RendererDX11, RendererDX11_Shader_Implementation_VertexDeclarationDX11);
