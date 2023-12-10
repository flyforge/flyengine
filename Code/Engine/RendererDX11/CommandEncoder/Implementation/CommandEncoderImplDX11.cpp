#include <RendererDX11/RendererDX11PCH.h>

#include <RendererDX11/CommandEncoder/CommandEncoderImplDX11.h>
#include <RendererDX11/Device/DeviceDX11.h>
#include <RendererDX11/Resources/BufferDX11.h>
#include <RendererDX11/Resources/QueryDX11.h>
#include <RendererDX11/Resources/RenderTargetViewDX11.h>
#include <RendererDX11/Resources/ResourceViewDX11.h>
#include <RendererDX11/Resources/TextureDX11.h>
#include <RendererDX11/Resources/UnorderedAccessViewDX11.h>
#include <RendererDX11/Shader/ShaderDX11.h>
#include <RendererDX11/Shader/VertexDeclarationDX11.h>
#include <RendererDX11/State/StateDX11.h>
#include <RendererFoundation/CommandEncoder/CommandEncoder.h>

#include <d3d11_1.h>

plGALCommandEncoderImplDX11::plGALCommandEncoderImplDX11(plGALDeviceDX11& ref_deviceDX11)
  : m_GALDeviceDX11(ref_deviceDX11)
{
  m_pDXContext = m_GALDeviceDX11.GetDXImmediateContext();

  if (FAILED(m_pDXContext->QueryInterface(__uuidof(ID3DUserDefinedAnnotation), (void**)&m_pDXAnnotation)))
  {
    plLog::Warning("Failed to get annotation interface. GALContext marker will not work");
  }
}

plGALCommandEncoderImplDX11::~plGALCommandEncoderImplDX11()
{
  PLASMA_GAL_DX11_RELEASE(m_pDXAnnotation);
}

// State setting functions

void plGALCommandEncoderImplDX11::SetShaderPlatform(const plGALShader* pShader)
{
  ID3D11VertexShader* pVS = nullptr;
  ID3D11HullShader* pHS = nullptr;
  ID3D11DomainShader* pDS = nullptr;
  ID3D11GeometryShader* pGS = nullptr;
  ID3D11PixelShader* pPS = nullptr;
  ID3D11ComputeShader* pCS = nullptr;

  if (pShader != nullptr)
  {
    const plGALShaderDX11* pDXShader = static_cast<const plGALShaderDX11*>(pShader);

    pVS = pDXShader->GetDXVertexShader();
    pHS = pDXShader->GetDXHullShader();
    pDS = pDXShader->GetDXDomainShader();
    pGS = pDXShader->GetDXGeometryShader();
    pPS = pDXShader->GetDXPixelShader();
    pCS = pDXShader->GetDXComputeShader();
  }

  if (pVS != m_pBoundShaders[plGALShaderStage::VertexShader])
  {
    m_pDXContext->VSSetShader(pVS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::VertexShader] = pVS;
  }

  if (pHS != m_pBoundShaders[plGALShaderStage::HullShader])
  {
    m_pDXContext->HSSetShader(pHS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::HullShader] = pHS;
  }

  if (pDS != m_pBoundShaders[plGALShaderStage::DomainShader])
  {
    m_pDXContext->DSSetShader(pDS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::DomainShader] = pDS;
  }

  if (pGS != m_pBoundShaders[plGALShaderStage::GeometryShader])
  {
    m_pDXContext->GSSetShader(pGS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::GeometryShader] = pGS;
  }

  if (pPS != m_pBoundShaders[plGALShaderStage::PixelShader])
  {
    m_pDXContext->PSSetShader(pPS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::PixelShader] = pPS;
  }

  if (pCS != m_pBoundShaders[plGALShaderStage::ComputeShader])
  {
    m_pDXContext->CSSetShader(pCS, nullptr, 0);
    m_pBoundShaders[plGALShaderStage::ComputeShader] = pCS;
  }
}

void plGALCommandEncoderImplDX11::SetConstantBufferPlatform(const plShaderResourceBinding& binding, const plGALBuffer* pBuffer)
{
  ID3D11Buffer* pBufferDX11 = pBuffer != nullptr ? static_cast<const plGALBufferDX11*>(pBuffer)->GetDXBuffer() : nullptr;
  if (m_pBoundConstantBuffers[binding.m_iSlot] == pBufferDX11)
    return;

  m_pBoundConstantBuffers[binding.m_iSlot] = pBufferDX11;
  // The GAL doesn't care about stages for constant buffer, but we need to handle this internally.
  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
    m_BoundConstantBuffersRange[stage].SetToIncludeValue(binding.m_iSlot);
}

void plGALCommandEncoderImplDX11::SetSamplerStatePlatform(const plShaderResourceBinding& binding, const plGALSamplerState* pSamplerState)
{
  ID3D11SamplerState* pSamplerStateDX11 = pSamplerState != nullptr ? static_cast<const plGALSamplerStateDX11*>(pSamplerState)->GetDXSamplerState() : nullptr;

  for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    const plGALShaderStageFlags::Enum flag = plGALShaderStageFlags::MakeFromShaderStage((plGALShaderStage::Enum)stage);
    if (binding.m_Stages.IsSet(flag))
    {
      if (m_pBoundSamplerStates[stage][binding.m_iSlot] != pSamplerStateDX11)
      {
        m_pBoundSamplerStates[stage][binding.m_iSlot] = pSamplerStateDX11;
        m_BoundSamplerStatesRange[stage].SetToIncludeValue(binding.m_iSlot);
      }
    }
  }
}

void plGALCommandEncoderImplDX11::SetResourceViewPlatform(const plShaderResourceBinding& binding, const plGALResourceView* pResourceView)
{
  if (pResourceView != nullptr && UnsetUnorderedAccessViews(pResourceView->GetResource()))
  {
    FlushPlatform();
  }

  ID3D11ShaderResourceView* pResourceViewDX11 = pResourceView != nullptr ? static_cast<const plGALResourceViewDX11*>(pResourceView)->GetDXResourceView() : nullptr;

    for (plUInt32 stage = plGALShaderStage::VertexShader; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    const plGALShaderStageFlags::Enum flag = plGALShaderStageFlags::MakeFromShaderStage((plGALShaderStage::Enum)stage);
    if (binding.m_Stages.IsSet(flag))
    {
      auto& boundShaderResourceViews = m_pBoundShaderResourceViews[stage];
      boundShaderResourceViews.EnsureCount(binding.m_iSlot + 1);
      auto& resourcesForResourceViews = m_ResourcesForResourceViews[stage];
      resourcesForResourceViews.EnsureCount(binding.m_iSlot + 1);
      if (boundShaderResourceViews[binding.m_iSlot] != pResourceViewDX11)
      {
        boundShaderResourceViews[binding.m_iSlot] = pResourceViewDX11;
        resourcesForResourceViews[binding.m_iSlot] = pResourceView != nullptr ? pResourceView->GetResource() : nullptr;
        m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(binding.m_iSlot);
      }
    }
  }
}

void plGALCommandEncoderImplDX11::SetUnorderedAccessViewPlatform(const plShaderResourceBinding& binding, const plGALUnorderedAccessView* pUnorderedAccessView)
{
  if (pUnorderedAccessView != nullptr && UnsetResourceViews(pUnorderedAccessView->GetResource()))
  {
    FlushPlatform();
  }

  ID3D11UnorderedAccessView* pUnorderedAccessViewDX11 = pUnorderedAccessView != nullptr ? static_cast<const plGALUnorderedAccessViewDX11*>(pUnorderedAccessView)->GetDXResourceView() : nullptr;

  m_BoundUnoderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  m_ResourcesForUnorderedAccessViews.EnsureCount(binding.m_iSlot + 1);
  if (m_BoundUnoderedAccessViews[binding.m_iSlot] != pUnorderedAccessViewDX11)
  {
    m_BoundUnoderedAccessViews[binding.m_iSlot] = pUnorderedAccessViewDX11;
    m_ResourcesForUnorderedAccessViews[binding.m_iSlot] = pUnorderedAccessView != nullptr ? pUnorderedAccessView->GetResource() : nullptr;
    m_BoundUnoderedAccessViewsRange.SetToIncludeValue(binding.m_iSlot);
  }
}

// Query functions

void plGALCommandEncoderImplDX11::BeginQueryPlatform(const plGALQuery* pQuery)
{
  m_pDXContext->Begin(static_cast<const plGALQueryDX11*>(pQuery)->GetDXQuery());
}

void plGALCommandEncoderImplDX11::EndQueryPlatform(const plGALQuery* pQuery)
{
  m_pDXContext->End(static_cast<const plGALQueryDX11*>(pQuery)->GetDXQuery());
}

plResult plGALCommandEncoderImplDX11::GetQueryResultPlatform(const plGALQuery* pQuery, plUInt64& ref_uiQueryResult)
{
  return m_pDXContext->GetData(
           static_cast<const plGALQueryDX11*>(pQuery)->GetDXQuery(), &ref_uiQueryResult, sizeof(plUInt64), D3D11_ASYNC_GETDATA_DONOTFLUSH) == S_FALSE
           ? PLASMA_FAILURE
           : PLASMA_SUCCESS;
}

// Timestamp functions

void plGALCommandEncoderImplDX11::InsertTimestampPlatform(plGALTimestampHandle hTimestamp)
{
  ID3D11Query* pDXQuery = m_GALDeviceDX11.GetTimestamp(hTimestamp);

  m_pDXContext->End(pDXQuery);
}

// Resource update functions

void plGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4 vClearValues)
{
  const plGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const plGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewFloat(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void plGALCommandEncoderImplDX11::ClearUnorderedAccessViewPlatform(const plGALUnorderedAccessView* pUnorderedAccessView, plVec4U32 vClearValues)
{
  const plGALUnorderedAccessViewDX11* pUnorderedAccessViewDX11 = static_cast<const plGALUnorderedAccessViewDX11*>(pUnorderedAccessView);
  m_pDXContext->ClearUnorderedAccessViewUint(pUnorderedAccessViewDX11->GetDXResourceView(), &vClearValues.x);
}

void plGALCommandEncoderImplDX11::CopyBufferPlatform(const plGALBuffer* pDestination, const plGALBuffer* pSource)
{
  ID3D11Buffer* pDXDestination = static_cast<const plGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const plGALBufferDX11*>(pSource)->GetDXBuffer();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void plGALCommandEncoderImplDX11::CopyBufferRegionPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, const plGALBuffer* pSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount)
{
  ID3D11Buffer* pDXDestination = static_cast<const plGALBufferDX11*>(pDestination)->GetDXBuffer();
  ID3D11Buffer* pDXSource = static_cast<const plGALBufferDX11*>(pSource)->GetDXBuffer();

  D3D11_BOX srcBox = {uiSourceOffset, 0, 0, uiSourceOffset + uiByteCount, 1, 1};
  m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXSource, 0, &srcBox);
}

void plGALCommandEncoderImplDX11::UpdateBufferPlatform(const plGALBuffer* pDestination, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> sourceData, plGALUpdateMode::Enum updateMode)
{
  PLASMA_CHECK_ALIGNMENT_16(sourceData.GetPtr());

  ID3D11Buffer* pDXDestination = static_cast<const plGALBufferDX11*>(pDestination)->GetDXBuffer();

  if (pDestination->GetDescription().m_BufferType == plGALBufferType::ConstantBuffer)
  {
    PLASMA_ASSERT_DEV(uiDestOffset == 0 && sourceData.GetCount() == pDestination->GetSize(),
      "Constant buffers can't be updated partially (and we don't check for DX11.1)!");

    D3D11_MAPPED_SUBRESOURCE MapResult;
    if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, D3D11_MAP_WRITE_DISCARD, 0, &MapResult)))
    {
      memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

      m_pDXContext->Unmap(pDXDestination, 0);
    }
  }
  else
  {
    if (updateMode == plGALUpdateMode::CopyToTempStorage)
    {
      if (ID3D11Resource* pDXTempBuffer = m_GALDeviceDX11.FindTempBuffer(sourceData.GetCount()))
      {
        D3D11_MAPPED_SUBRESOURCE MapResult;
        HRESULT hRes = m_pDXContext->Map(pDXTempBuffer, 0, D3D11_MAP_WRITE, 0, &MapResult);
        PLASMA_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

        memcpy(MapResult.pData, sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXTempBuffer, 0);

        D3D11_BOX srcBox = {0, 0, 0, sourceData.GetCount(), 1, 1};
        m_pDXContext->CopySubresourceRegion(pDXDestination, 0, uiDestOffset, 0, 0, pDXTempBuffer, 0, &srcBox);
      }
      else
      {
        PLASMA_REPORT_FAILURE("Could not find a temp buffer for update.");
      }
    }
    else
    {
      D3D11_MAP mapType = (updateMode == plGALUpdateMode::Discard) ? D3D11_MAP_WRITE_DISCARD : D3D11_MAP_WRITE_NO_OVERWRITE;

      D3D11_MAPPED_SUBRESOURCE MapResult;
      if (SUCCEEDED(m_pDXContext->Map(pDXDestination, 0, mapType, 0, &MapResult)))
      {
        memcpy(plMemoryUtils::AddByteOffset(MapResult.pData, uiDestOffset), sourceData.GetPtr(), sourceData.GetCount());

        m_pDXContext->Unmap(pDXDestination, 0);
      }
      else
      {
        plLog::Error("Could not map buffer to update content.");
      }
    }
  }
}

void plGALCommandEncoderImplDX11::CopyTexturePlatform(const plGALTexture* pDestination, const plGALTexture* pSource)
{
  ID3D11Resource* pDXDestination = static_cast<const plGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const plGALTextureDX11*>(pSource)->GetDXTexture();

  m_pDXContext->CopyResource(pDXDestination, pDXSource);
}

void plGALCommandEncoderImplDX11::CopyTextureRegionPlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource,
  const plVec3U32& vDestinationPoint, const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource, const plBoundingBoxu32& box)
{
  ID3D11Resource* pDXDestination = static_cast<const plGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const plGALTextureDX11*>(pSource)->GetDXTexture();

  plUInt32 dstSubResource = D3D11CalcSubresource(
    destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  plUInt32 srcSubResource =
    D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  D3D11_BOX srcBox = {box.m_vMin.x, box.m_vMin.y, box.m_vMin.z, box.m_vMax.x, box.m_vMax.y, box.m_vMax.z};
  m_pDXContext->CopySubresourceRegion(
    pDXDestination, dstSubResource, vDestinationPoint.x, vDestinationPoint.y, vDestinationPoint.z, pDXSource, srcSubResource, &srcBox);
}

void plGALCommandEncoderImplDX11::UpdateTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource,
  const plBoundingBoxu32& destinationBox, const plGALSystemMemoryDescription& sourceData)
{
  ID3D11Resource* pDXDestination = static_cast<const plGALTextureDX11*>(pDestination)->GetDXTexture();

  plUInt32 uiWidth = plMath::Max(destinationBox.m_vMax.x - destinationBox.m_vMin.x, 1u);
  plUInt32 uiHeight = plMath::Max(destinationBox.m_vMax.y - destinationBox.m_vMin.y, 1u);
  plUInt32 uiDepth = plMath::Max(destinationBox.m_vMax.z - destinationBox.m_vMin.z, 1u);
  plGALResourceFormat::Enum format = pDestination->GetDescription().m_Format;

  if (ID3D11Resource* pDXTempTexture = m_GALDeviceDX11.FindTempTexture(uiWidth, uiHeight, uiDepth, format))
  {
    D3D11_MAPPED_SUBRESOURCE MapResult;
    HRESULT hRes = m_pDXContext->Map(pDXTempTexture, 0, D3D11_MAP_WRITE, 0, &MapResult);
    PLASMA_ASSERT_DEV(SUCCEEDED(hRes), "Implementation error");

    plUInt32 uiRowPitch = uiWidth * plGALResourceFormat::GetBitsPerElement(format) / 8;
    plUInt32 uiSlicePitch = uiRowPitch * uiHeight;
    PLASMA_ASSERT_DEV(sourceData.m_uiRowPitch == uiRowPitch, "Invalid row pitch. Expected {0} got {1}", uiRowPitch, sourceData.m_uiRowPitch);
    PLASMA_ASSERT_DEV(sourceData.m_uiSlicePitch == 0 || sourceData.m_uiSlicePitch == uiSlicePitch, "Invalid slice pitch. Expected {0} got {1}",
      uiSlicePitch, sourceData.m_uiSlicePitch);

    if (MapResult.RowPitch == uiRowPitch && MapResult.DepthPitch == uiSlicePitch)
    {
      memcpy(MapResult.pData, sourceData.m_pData, uiSlicePitch * uiDepth);
    }
    else
    {
      // Copy row by row
      for (plUInt32 z = 0; z < uiDepth; ++z)
      {
        const void* pSource = plMemoryUtils::AddByteOffset(sourceData.m_pData, z * uiSlicePitch);
        void* pDest = plMemoryUtils::AddByteOffset(MapResult.pData, z * MapResult.DepthPitch);

        for (plUInt32 y = 0; y < uiHeight; ++y)
        {
          memcpy(pDest, pSource, uiRowPitch);

          pSource = plMemoryUtils::AddByteOffset(pSource, uiRowPitch);
          pDest = plMemoryUtils::AddByteOffset(pDest, MapResult.RowPitch);
        }
      }
    }

    m_pDXContext->Unmap(pDXTempTexture, 0);

    plUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);

    D3D11_BOX srcBox = {0, 0, 0, uiWidth, uiHeight, uiDepth};
    m_pDXContext->CopySubresourceRegion(pDXDestination, dstSubResource, destinationBox.m_vMin.x, destinationBox.m_vMin.y, destinationBox.m_vMin.z, pDXTempTexture, 0, &srcBox);
  }
  else
  {
    PLASMA_REPORT_FAILURE("Could not find a temp texture for update.");
  }
}

void plGALCommandEncoderImplDX11::ResolveTexturePlatform(const plGALTexture* pDestination, const plGALTextureSubresource& destinationSubResource,
  const plGALTexture* pSource, const plGALTextureSubresource& sourceSubResource)
{
  ID3D11Resource* pDXDestination = static_cast<const plGALTextureDX11*>(pDestination)->GetDXTexture();
  ID3D11Resource* pDXSource = static_cast<const plGALTextureDX11*>(pSource)->GetDXTexture();

  plUInt32 dstSubResource = D3D11CalcSubresource(destinationSubResource.m_uiMipLevel, destinationSubResource.m_uiArraySlice, pDestination->GetDescription().m_uiMipLevelCount);
  plUInt32 srcSubResource = D3D11CalcSubresource(sourceSubResource.m_uiMipLevel, sourceSubResource.m_uiArraySlice, pSource->GetDescription().m_uiMipLevelCount);

  DXGI_FORMAT DXFormat = m_GALDeviceDX11.GetFormatLookupTable().GetFormatInfo(pDestination->GetDescription().m_Format).m_eResourceViewType;

  m_pDXContext->ResolveSubresource(pDXDestination, dstSubResource, pDXSource, srcSubResource, DXFormat);
}

void plGALCommandEncoderImplDX11::ReadbackTexturePlatform(const plGALTexture* pTexture)
{
  const plGALTextureDX11* pDXTexture = static_cast<const plGALTextureDX11*>(pTexture);

  // MSAA textures (e.g. backbuffers) need to be converted to non MSAA versions
  const bool bMSAASourceTexture = pDXTexture->GetDescription().m_SampleCount != plGALMSAASampleCount::None;

  PLASMA_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  PLASMA_ASSERT_DEV(pDXTexture->GetDXTexture() != nullptr, "Texture object is invalid");

  if (bMSAASourceTexture)
  {
    /// \todo Other mip levels etc?
    m_pDXContext->ResolveSubresource(pDXTexture->GetDXStagingTexture(), 0, pDXTexture->GetDXTexture(), 0, DXGI_FORMAT_R8G8B8A8_UNORM_SRGB);
  }
  else
  {
    m_pDXContext->CopyResource(pDXTexture->GetDXStagingTexture(), pDXTexture->GetDXTexture());
  }
}

plUInt32 GetMipSize(plUInt32 uiSize, plUInt32 uiMipLevel)
{
  for (plUInt32 i = 0; i < uiMipLevel; i++)
  {
    uiSize = uiSize / 2;
  }
  return plMath::Max(1u, uiSize);
}

void plGALCommandEncoderImplDX11::CopyTextureReadbackResultPlatform(const plGALTexture* pTexture, plArrayPtr<plGALTextureSubresource> sourceSubResource, plArrayPtr<plGALSystemMemoryDescription> targetData)
{
  const plGALTextureDX11* pDXTexture = static_cast<const plGALTextureDX11*>(pTexture);

  PLASMA_ASSERT_DEV(pDXTexture->GetDXStagingTexture() != nullptr, "No staging resource available for read-back");
  PLASMA_ASSERT_DEV(sourceSubResource.GetCount() == targetData.GetCount(), "Source and target arrays must be of the same size.");

  const plUInt32 uiSubResources = sourceSubResource.GetCount();
  for (plUInt32 i = 0; i < uiSubResources; i++)
  {
    const plGALTextureSubresource& subRes = sourceSubResource[i];
    const plGALSystemMemoryDescription& memDesc = targetData[i];
    const plUInt32 uiSubResourceIndex = D3D11CalcSubresource(subRes.m_uiMipLevel, subRes.m_uiArraySlice, pTexture->GetDescription().m_uiMipLevelCount);

    D3D11_MAPPED_SUBRESOURCE Mapped;
    if (SUCCEEDED(m_pDXContext->Map(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex, D3D11_MAP_READ, 0, &Mapped)))
    {
      // TODO: Depth pitch
      if (Mapped.RowPitch == memDesc.m_uiRowPitch)
      {
        const plUInt32 uiMemorySize = plGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) *
                                      GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) *
                                      GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel) / 8;
        memcpy(memDesc.m_pData, Mapped.pData, uiMemorySize);
      }
      else
      {
        // Copy row by row
        const plUInt32 uiHeight = GetMipSize(pDXTexture->GetDescription().m_uiHeight, subRes.m_uiMipLevel);
        for (plUInt32 y = 0; y < uiHeight; ++y)
        {
          const void* pSource = plMemoryUtils::AddByteOffset(Mapped.pData, y * Mapped.RowPitch);
          void* pDest = plMemoryUtils::AddByteOffset(memDesc.m_pData, y * memDesc.m_uiRowPitch);

          memcpy(
            pDest, pSource, plGALResourceFormat::GetBitsPerElement(pDXTexture->GetDescription().m_Format) * GetMipSize(pDXTexture->GetDescription().m_uiWidth, subRes.m_uiMipLevel) / 8);
        }
      }

      m_pDXContext->Unmap(pDXTexture->GetDXStagingTexture(), uiSubResourceIndex);
    }
  }
}

void plGALCommandEncoderImplDX11::GenerateMipMapsPlatform(const plGALResourceView* pResourceView)
{
  const plGALResourceViewDX11* pDXResourceView = static_cast<const plGALResourceViewDX11*>(pResourceView);

  m_pDXContext->GenerateMips(pDXResourceView->GetDXResourceView());
}

void plGALCommandEncoderImplDX11::FlushPlatform()
{
  FlushDeferredStateChanges();
}

// Debug helper functions

void plGALCommandEncoderImplDX11::PushMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    plStringWChar wsMarker(szMarker);
    m_pDXAnnotation->BeginEvent(wsMarker.GetData());
  }
}

void plGALCommandEncoderImplDX11::PopMarkerPlatform()
{
  if (m_pDXAnnotation != nullptr)
  {
    m_pDXAnnotation->EndEvent();
  }
}

void plGALCommandEncoderImplDX11::InsertEventMarkerPlatform(const char* szMarker)
{
  if (m_pDXAnnotation != nullptr)
  {
    plStringWChar wsMarker(szMarker);
    m_pDXAnnotation->SetMarker(wsMarker.GetData());
  }
}

//////////////////////////////////////////////////////////////////////////

void plGALCommandEncoderImplDX11::BeginRendering(const plGALRenderingSetup& renderingSetup)
{
  if (m_RenderTargetSetup != renderingSetup.m_RenderTargetSetup)
  {
    m_RenderTargetSetup = renderingSetup.m_RenderTargetSetup;

    const plGALRenderTargetView* pRenderTargetViews[PLASMA_GAL_MAX_RENDERTARGET_COUNT] = {nullptr};
    const plGALRenderTargetView* pDepthStencilView = nullptr;

    const plUInt32 uiRenderTargetCount = m_RenderTargetSetup.GetRenderTargetCount();

    bool bFlushNeeded = false;

    for (plUInt8 uiIndex = 0; uiIndex < uiRenderTargetCount; ++uiIndex)
    {
      const plGALRenderTargetView* pRenderTargetView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetRenderTarget(uiIndex));
      if (pRenderTargetView != nullptr)
      {
        const plGALResourceBase* pTexture = pRenderTargetView->GetTexture()->GetParentResource();

        bFlushNeeded |= UnsetResourceViews(pTexture);
        bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
      }

      pRenderTargetViews[uiIndex] = pRenderTargetView;
    }

    pDepthStencilView = m_GALDeviceDX11.GetRenderTargetView(m_RenderTargetSetup.GetDepthStencilTarget());
    if (pDepthStencilView != nullptr)
    {
      const plGALResourceBase* pTexture = pDepthStencilView->GetTexture()->GetParentResource();

      bFlushNeeded |= UnsetResourceViews(pTexture);
      bFlushNeeded |= UnsetUnorderedAccessViews(pTexture);
    }

    if (bFlushNeeded)
    {
      FlushPlatform();
    }

    for (plUInt32 i = 0; i < PLASMA_GAL_MAX_RENDERTARGET_COUNT; i++)
    {
      m_pBoundRenderTargets[i] = nullptr;
    }
    m_pBoundDepthStencilTarget = nullptr;

    if (uiRenderTargetCount != 0 || pDepthStencilView != nullptr)
    {
      for (plUInt32 i = 0; i < uiRenderTargetCount; i++)
      {
        if (pRenderTargetViews[i] != nullptr)
        {
          m_pBoundRenderTargets[i] = static_cast<const plGALRenderTargetViewDX11*>(pRenderTargetViews[i])->GetRenderTargetView();
        }
      }

      if (pDepthStencilView != nullptr)
      {
        m_pBoundDepthStencilTarget = static_cast<const plGALRenderTargetViewDX11*>(pDepthStencilView)->GetDepthStencilView();
      }

      // Bind rendertargets, bind max(new rt count, old rt count) to overwrite bound rts if new count < old count
      m_pDXContext->OMSetRenderTargets(plMath::Max(uiRenderTargetCount, m_uiBoundRenderTargetCount), m_pBoundRenderTargets, m_pBoundDepthStencilTarget);

      m_uiBoundRenderTargetCount = uiRenderTargetCount;
    }
    else
    {
      m_pBoundDepthStencilTarget = nullptr;
      m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
      m_uiBoundRenderTargetCount = 0;
    }
  }

  ClearPlatform(renderingSetup.m_ClearColor, renderingSetup.m_uiRenderTargetClearMask, renderingSetup.m_bClearDepth, renderingSetup.m_bClearStencil, renderingSetup.m_fDepthClear, renderingSetup.m_uiStencilClear);
}

void plGALCommandEncoderImplDX11::BeginCompute()
{
  // We need to unbind all render targets as otherwise using them in a compute shader as input will fail:
  // DEVICE_CSSETSHADERRESOURCES_HAZARD: Resource being set to CS shader resource slot 0 is still bound on output!
  m_RenderTargetSetup = plGALRenderTargetSetup();
  m_pDXContext->OMSetRenderTargets(0, nullptr, nullptr);
}

// Draw functions

void plGALCommandEncoderImplDX11::ClearPlatform(const plColor& clearColor, plUInt32 uiRenderTargetClearMask, bool bClearDepth, bool bClearStencil, float fDepthClear, plUInt8 uiStencilClear)
{
  for (plUInt32 i = 0; i < m_uiBoundRenderTargetCount; i++)
  {
    if (uiRenderTargetClearMask & (1u << i) && m_pBoundRenderTargets[i])
    {
      m_pDXContext->ClearRenderTargetView(m_pBoundRenderTargets[i], clearColor.GetData());
    }
  }

  if ((bClearDepth || bClearStencil) && m_pBoundDepthStencilTarget)
  {
    plUInt32 uiClearFlags = bClearDepth ? D3D11_CLEAR_DEPTH : 0;
    uiClearFlags |= bClearStencil ? D3D11_CLEAR_STENCIL : 0;

    m_pDXContext->ClearDepthStencilView(m_pBoundDepthStencilTarget, uiClearFlags, fDepthClear, uiStencilClear);
  }
}

void plGALCommandEncoderImplDX11::DrawPlatform(plUInt32 uiVertexCount, plUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->Draw(uiVertexCount, uiStartVertex);
}

void plGALCommandEncoderImplDX11::DrawIndexedPlatform(plUInt32 uiIndexCount, plUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);

  // In debug builds, with a debugger attached, the engine will break on D3D errors
  // this can be very annoying when an error happens repeatedly
  // you can disable it at runtime, by using the debugger to set bChangeBreakPolicy to 'true', or dragging the
  // the instruction pointer into the if
  volatile bool bChangeBreakPolicy = false;
  if (bChangeBreakPolicy)
  {
    if (m_GALDeviceDX11.m_pDebug != nullptr)
    {
      ID3D11InfoQueue* pInfoQueue = nullptr;
      if (SUCCEEDED(m_GALDeviceDX11.m_pDebug->QueryInterface(__uuidof(ID3D11InfoQueue), (void**)&pInfoQueue)))
      {
        // modify these, if you want to keep certain things enabled
        static BOOL bBreakOnCorruption = FALSE;
        static BOOL bBreakOnError = FALSE;
        static BOOL bBreakOnWarning = FALSE;

        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_CORRUPTION, bBreakOnCorruption);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_ERROR, bBreakOnError);
        pInfoQueue->SetBreakOnSeverity(D3D11_MESSAGE_SEVERITY_WARNING, bBreakOnWarning);
      }
    }
  }
#else
  m_pDXContext->DrawIndexed(uiIndexCount, uiStartIndex, 0);
#endif
}

void plGALCommandEncoderImplDX11::DrawIndexedInstancedPlatform(plUInt32 uiIndexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartIndex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstanced(uiIndexCountPerInstance, uiInstanceCount, uiStartIndex, 0, 0);
}

void plGALCommandEncoderImplDX11::DrawIndexedInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawIndexedInstancedIndirect(static_cast<const plGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void plGALCommandEncoderImplDX11::DrawInstancedPlatform(plUInt32 uiVertexCountPerInstance, plUInt32 uiInstanceCount, plUInt32 uiStartVertex)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstanced(uiVertexCountPerInstance, uiInstanceCount, uiStartVertex, 0);
}

void plGALCommandEncoderImplDX11::DrawInstancedIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DrawInstancedIndirect(static_cast<const plGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

void plGALCommandEncoderImplDX11::SetIndexBufferPlatform(const plGALBuffer* pIndexBuffer)
{
  if (pIndexBuffer != nullptr)
  {
    const plGALBufferDX11* pDX11Buffer = static_cast<const plGALBufferDX11*>(pIndexBuffer);
    m_pDXContext->IASetIndexBuffer(pDX11Buffer->GetDXBuffer(), pDX11Buffer->GetIndexFormat(), 0 /* \todo: Expose */);
  }
  else
  {
    m_pDXContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R16_UINT, 0);
  }
}

void plGALCommandEncoderImplDX11::SetVertexBufferPlatform(plUInt32 uiSlot, const plGALBuffer* pVertexBuffer)
{
  PLASMA_ASSERT_DEV(uiSlot < PLASMA_GAL_MAX_VERTEX_BUFFER_COUNT, "Invalid slot index");

  m_pBoundVertexBuffers[uiSlot] = pVertexBuffer != nullptr ? static_cast<const plGALBufferDX11*>(pVertexBuffer)->GetDXBuffer() : nullptr;
  m_VertexBufferStrides[uiSlot] = pVertexBuffer != nullptr ? pVertexBuffer->GetDescription().m_uiStructSize : 0;
  m_BoundVertexBuffersRange.SetToIncludeValue(uiSlot);
}

void plGALCommandEncoderImplDX11::SetVertexDeclarationPlatform(const plGALVertexDeclaration* pVertexDeclaration)
{
  m_pDXContext->IASetInputLayout(
    pVertexDeclaration != nullptr ? static_cast<const plGALVertexDeclarationDX11*>(pVertexDeclaration)->GetDXInputLayout() : nullptr);
}

static const D3D11_PRIMITIVE_TOPOLOGY GALTopologyToDX11[plGALPrimitiveTopology::ENUM_COUNT] = {
  D3D11_PRIMITIVE_TOPOLOGY_POINTLIST,
  D3D11_PRIMITIVE_TOPOLOGY_LINELIST,
  D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST,
};

void plGALCommandEncoderImplDX11::SetPrimitiveTopologyPlatform(plGALPrimitiveTopology::Enum topology)
{
  m_pDXContext->IASetPrimitiveTopology(GALTopologyToDX11[topology]);
}

void plGALCommandEncoderImplDX11::SetBlendStatePlatform(const plGALBlendState* pBlendState, const plColor& blendFactor, plUInt32 uiSampleMask)
{
  FLOAT BlendFactors[4] = {blendFactor.r, blendFactor.g, blendFactor.b, blendFactor.a};

  m_pDXContext->OMSetBlendState(
    pBlendState != nullptr ? static_cast<const plGALBlendStateDX11*>(pBlendState)->GetDXBlendState() : nullptr, BlendFactors, uiSampleMask);
}

void plGALCommandEncoderImplDX11::SetDepthStencilStatePlatform(const plGALDepthStencilState* pDepthStencilState, plUInt8 uiStencilRefValue)
{
  m_pDXContext->OMSetDepthStencilState(
    pDepthStencilState != nullptr ? static_cast<const plGALDepthStencilStateDX11*>(pDepthStencilState)->GetDXDepthStencilState() : nullptr,
    uiStencilRefValue);
}

void plGALCommandEncoderImplDX11::SetRasterizerStatePlatform(const plGALRasterizerState* pRasterizerState)
{
  m_pDXContext->RSSetState(pRasterizerState != nullptr ? static_cast<const plGALRasterizerStateDX11*>(pRasterizerState)->GetDXRasterizerState() : nullptr);
}

void plGALCommandEncoderImplDX11::SetViewportPlatform(const plRectFloat& rect, float fMinDepth, float fMaxDepth)
{
  D3D11_VIEWPORT Viewport;
  Viewport.TopLeftX = rect.x;
  Viewport.TopLeftY = rect.y;
  Viewport.Width = rect.width;
  Viewport.Height = rect.height;
  Viewport.MinDepth = fMinDepth;
  Viewport.MaxDepth = fMaxDepth;

  m_pDXContext->RSSetViewports(1, &Viewport);
}

void plGALCommandEncoderImplDX11::SetScissorRectPlatform(const plRectU32& rect)
{
  D3D11_RECT ScissorRect;
  ScissorRect.left = rect.x;
  ScissorRect.top = rect.y;
  ScissorRect.right = rect.x + rect.width;
  ScissorRect.bottom = rect.y + rect.height;

  m_pDXContext->RSSetScissorRects(1, &ScissorRect);
}

//////////////////////////////////////////////////////////////////////////

void plGALCommandEncoderImplDX11::DispatchPlatform(plUInt32 uiThreadGroupCountX, plUInt32 uiThreadGroupCountY, plUInt32 uiThreadGroupCountZ)
{
  FlushDeferredStateChanges();

  m_pDXContext->Dispatch(uiThreadGroupCountX, uiThreadGroupCountY, uiThreadGroupCountZ);
}

void plGALCommandEncoderImplDX11::DispatchIndirectPlatform(const plGALBuffer* pIndirectArgumentBuffer, plUInt32 uiArgumentOffsetInBytes)
{
  FlushDeferredStateChanges();

  m_pDXContext->DispatchIndirect(static_cast<const plGALBufferDX11*>(pIndirectArgumentBuffer)->GetDXBuffer(), uiArgumentOffsetInBytes);
}

//////////////////////////////////////////////////////////////////////////

static void SetShaderResources(plGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, plUInt32 uiStartSlot, plUInt32 uiNumSlots,
  ID3D11ShaderResourceView** pShaderResourceViews)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      pContext->VSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case plGALShaderStage::HullShader:
      pContext->HSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case plGALShaderStage::DomainShader:
      pContext->DSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case plGALShaderStage::GeometryShader:
      pContext->GSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case plGALShaderStage::PixelShader:
      pContext->PSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    case plGALShaderStage::ComputeShader:
      pContext->CSSetShaderResources(uiStartSlot, uiNumSlots, pShaderResourceViews);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetConstantBuffers(
  plGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, plUInt32 uiStartSlot, plUInt32 uiNumSlots, ID3D11Buffer** pConstantBuffers)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      pContext->VSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case plGALShaderStage::HullShader:
      pContext->HSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case plGALShaderStage::DomainShader:
      pContext->DSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case plGALShaderStage::GeometryShader:
      pContext->GSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case plGALShaderStage::PixelShader:
      pContext->PSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    case plGALShaderStage::ComputeShader:
      pContext->CSSetConstantBuffers(uiStartSlot, uiNumSlots, pConstantBuffers);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
}

static void SetSamplers(
  plGALShaderStage::Enum stage, ID3D11DeviceContext* pContext, plUInt32 uiStartSlot, plUInt32 uiNumSlots, ID3D11SamplerState** pSamplerStates)
{
  switch (stage)
  {
    case plGALShaderStage::VertexShader:
      pContext->VSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case plGALShaderStage::HullShader:
      pContext->HSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case plGALShaderStage::DomainShader:
      pContext->DSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case plGALShaderStage::GeometryShader:
      pContext->GSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case plGALShaderStage::PixelShader:
      pContext->PSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    case plGALShaderStage::ComputeShader:
      pContext->CSSetSamplers(uiStartSlot, uiNumSlots, pSamplerStates);
      break;
    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
  }
}

// Some state changes are deferred so they can be updated faster
void plGALCommandEncoderImplDX11::FlushDeferredStateChanges()
{
  if (m_BoundVertexBuffersRange.IsValid())
  {
    const plUInt32 uiStartSlot = m_BoundVertexBuffersRange.m_uiMin;
    const plUInt32 uiNumSlots = m_BoundVertexBuffersRange.GetCount();

    m_pDXContext->IASetVertexBuffers(uiStartSlot, uiNumSlots, m_pBoundVertexBuffers + uiStartSlot, m_VertexBufferStrides + uiStartSlot, m_VertexBufferOffsets + uiStartSlot);

    m_BoundVertexBuffersRange.Reset();
  }

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    if (m_pBoundShaders[stage] != nullptr && m_BoundConstantBuffersRange[stage].IsValid())
    {
      const plUInt32 uiStartSlot = m_BoundConstantBuffersRange[stage].m_uiMin;
      const plUInt32 uiNumSlots = m_BoundConstantBuffersRange[stage].GetCount();

      SetConstantBuffers((plGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundConstantBuffers + uiStartSlot);

      m_BoundConstantBuffersRange[stage].Reset();
    }
  }

  // Do UAV bindings before SRV since UAV are outputs which need to be unbound before they are potentially rebound as SRV again.
  if (m_BoundUnoderedAccessViewsRange.IsValid())
  {
    const plUInt32 uiStartSlot = m_BoundUnoderedAccessViewsRange.m_uiMin;
    const plUInt32 uiNumSlots = m_BoundUnoderedAccessViewsRange.GetCount();
    m_pDXContext->CSSetUnorderedAccessViews(uiStartSlot, uiNumSlots, m_BoundUnoderedAccessViews.GetData() + uiStartSlot, nullptr); // Todo: Count reset.

    m_BoundUnoderedAccessViewsRange.Reset();
  }

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    // Need to do bindings even on inactive shader stages since we might miss unbindings otherwise!
    if (m_BoundShaderResourceViewsRange[stage].IsValid())
    {
      const plUInt32 uiStartSlot = m_BoundShaderResourceViewsRange[stage].m_uiMin;
      const plUInt32 uiNumSlots = m_BoundShaderResourceViewsRange[stage].GetCount();

      SetShaderResources((plGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundShaderResourceViews[stage].GetData() + uiStartSlot);

      m_BoundShaderResourceViewsRange[stage].Reset();
    }

    // Don't need to unset sampler stages for unbound shader stages.
    if (m_pBoundShaders[stage] == nullptr)
      continue;

    if (m_BoundSamplerStatesRange[stage].IsValid())
    {
      const plUInt32 uiStartSlot = m_BoundSamplerStatesRange[stage].m_uiMin;
      const plUInt32 uiNumSlots = m_BoundSamplerStatesRange[stage].GetCount();

      SetSamplers((plGALShaderStage::Enum)stage, m_pDXContext, uiStartSlot, uiNumSlots, m_pBoundSamplerStates[stage] + uiStartSlot);

      m_BoundSamplerStatesRange[stage].Reset();
    }
  }
}
bool plGALCommandEncoderImplDX11::UnsetUnorderedAccessViews(const plGALResourceBase* pResource)
{
  PLASMA_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (plUInt32 uiSlot = 0; uiSlot < m_ResourcesForUnorderedAccessViews.GetCount(); ++uiSlot)
  {
    if (m_ResourcesForUnorderedAccessViews[uiSlot] == pResource)
    {
      m_ResourcesForUnorderedAccessViews[uiSlot] = nullptr;
      m_BoundUnoderedAccessViews[uiSlot] = nullptr;
      m_BoundUnoderedAccessViewsRange.SetToIncludeValue(uiSlot);
      bResult = true;
    }
  }

  return bResult;
}
bool plGALCommandEncoderImplDX11::UnsetResourceViews(const plGALResourceBase* pResource)
{
  PLASMA_ASSERT_DEV(pResource->GetParentResource() == pResource, "No proxies allowed");

  bool bResult = false;

  for (plUInt32 stage = 0; stage < plGALShaderStage::ENUM_COUNT; ++stage)
  {
    for (plUInt32 uiSlot = 0; uiSlot < m_ResourcesForResourceViews[stage].GetCount(); ++uiSlot)
    {
      if (m_ResourcesForResourceViews[stage][uiSlot] == pResource)
      {
        m_ResourcesForResourceViews[stage][uiSlot] = nullptr;
        m_pBoundShaderResourceViews[stage][uiSlot] = nullptr;
        m_BoundShaderResourceViewsRange[stage].SetToIncludeValue(uiSlot);
        bResult = true;
      }
    }
  }

  return bResult;
}
