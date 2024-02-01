#include <RendererFoundation/RendererFoundationPCH.h>

#include <RendererFoundation/CommandEncoder/CommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Buffer.h>
#include <RendererFoundation/Resources/Query.h>
#include <RendererFoundation/Resources/RenderTargetView.h>
#include <RendererFoundation/Resources/ResourceView.h>
#include <RendererFoundation/Resources/Texture.h>
#include <RendererFoundation/Resources/UnorderedAccesView.h>

void plGALCommandEncoder::SetShader(plGALShaderHandle hShader)
{
  AssertRenderingThread();
  /// \todo Assert for shader capabilities (supported shader stages etc.)

  if (m_State.m_hShader == hShader)
    return;

  const plGALShader* pShader = m_Device.GetShader(hShader);
  PL_ASSERT_DEV(pShader != nullptr, "The given shader handle isn't valid, this may be a use after destroy!");

  m_CommonImpl.SetShaderPlatform(pShader);

  m_State.m_hShader = hShader;
}

void plGALCommandEncoder::SetConstantBuffer(const plShaderResourceBinding& binding, plGALBufferHandle hBuffer)
{
  AssertRenderingThread();

  const plGALBuffer* pBuffer = m_Device.GetBuffer(hBuffer);
  PL_ASSERT_DEV(pBuffer == nullptr || pBuffer->GetDescription().m_BufferType == plGALBufferType::ConstantBuffer, "Wrong buffer type");

  m_CommonImpl.SetConstantBufferPlatform(binding, pBuffer);
}

void plGALCommandEncoder::SetSamplerState(const plShaderResourceBinding& binding, plGALSamplerStateHandle hSamplerState)
{
  AssertRenderingThread();

  const plGALSamplerState* pSamplerState = m_Device.GetSamplerState(hSamplerState);

  m_CommonImpl.SetSamplerStatePlatform(binding, pSamplerState);
}

void plGALCommandEncoder::SetResourceView(const plShaderResourceBinding& binding, plGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const plGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);

  m_CommonImpl.SetResourceViewPlatform(binding, pResourceView);
}

void plGALCommandEncoder::SetUnorderedAccessView(const plShaderResourceBinding& binding, plGALUnorderedAccessViewHandle hUnorderedAccessView)
{
  AssertRenderingThread();

  const plGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  m_CommonImpl.SetUnorderedAccessViewPlatform(binding, pUnorderedAccessView);
}

void plGALCommandEncoder::SetPushConstants(plArrayPtr<const plUInt8> data)
{
  AssertRenderingThread();
  m_CommonImpl.SetPushConstantsPlatform(data);
}

void plGALCommandEncoder::BeginQuery(plGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  PL_ASSERT_DEV(!query->m_bStarted, "Can't stat plGALQuery because it is already running.");

  m_CommonImpl.BeginQueryPlatform(query);
}

void plGALCommandEncoder::EndQuery(plGALQueryHandle hQuery)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  PL_ASSERT_DEV(query->m_bStarted, "Can't end plGALQuery, query hasn't started yet.");

  m_CommonImpl.EndQueryPlatform(query);
}

plResult plGALCommandEncoder::GetQueryResult(plGALQueryHandle hQuery, plUInt64& ref_uiQueryResult)
{
  AssertRenderingThread();

  auto query = m_Device.GetQuery(hQuery);
  PL_ASSERT_DEV(!query->m_bStarted, "Can't retrieve data from plGALQuery while query is still running.");

  return m_CommonImpl.GetQueryResultPlatform(query, ref_uiQueryResult);
}

plGALTimestampHandle plGALCommandEncoder::InsertTimestamp()
{
  plGALTimestampHandle hTimestamp = m_Device.GetTimestamp();

  m_CommonImpl.InsertTimestampPlatform(hTimestamp);

  return hTimestamp;
}

void plGALCommandEncoder::ClearUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView, plVec4 vClearValues)
{
  AssertRenderingThread();

  const plGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    PL_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void plGALCommandEncoder::ClearUnorderedAccessView(plGALUnorderedAccessViewHandle hUnorderedAccessView, plVec4U32 vClearValues)
{
  AssertRenderingThread();

  const plGALUnorderedAccessView* pUnorderedAccessView = m_Device.GetUnorderedAccessView(hUnorderedAccessView);
  if (pUnorderedAccessView == nullptr)
  {
    PL_REPORT_FAILURE("ClearUnorderedAccessView failed, unordered access view handle invalid.");
    return;
  }

  m_CommonImpl.ClearUnorderedAccessViewPlatform(pUnorderedAccessView, vClearValues);
}

void plGALCommandEncoder::CopyBuffer(plGALBufferHandle hDest, plGALBufferHandle hSource)
{
  AssertRenderingThread();

  const plGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const plGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyBufferPlatform(pDest, pSource);
  }
  else
  {
    PL_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", plArgP(pDest), plArgP(pSource));
  }
}

void plGALCommandEncoder::CopyBufferRegion(
  plGALBufferHandle hDest, plUInt32 uiDestOffset, plGALBufferHandle hSource, plUInt32 uiSourceOffset, plUInt32 uiByteCount)
{
  AssertRenderingThread();

  const plGALBuffer* pDest = m_Device.GetBuffer(hDest);
  const plGALBuffer* pSource = m_Device.GetBuffer(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    const plUInt32 uiDestSize = pDest->GetSize();
    const plUInt32 uiSourceSize = pSource->GetSize();

    PL_IGNORE_UNUSED(uiDestSize);
    PL_ASSERT_DEV(uiDestSize >= uiDestOffset + uiByteCount, "Destination buffer too small (or offset too big)");
    PL_IGNORE_UNUSED(uiSourceSize);
    PL_ASSERT_DEV(uiSourceSize >= uiSourceOffset + uiByteCount, "Source buffer too small (or offset too big)");

    m_CommonImpl.CopyBufferRegionPlatform(pDest, uiDestOffset, pSource, uiSourceOffset, uiByteCount);
  }
  else
  {
    PL_REPORT_FAILURE("CopyBuffer failed, buffer handle invalid - destination = {0}, source = {1}", plArgP(pDest), plArgP(pSource));
  }
}

void plGALCommandEncoder::UpdateBuffer(plGALBufferHandle hDest, plUInt32 uiDestOffset, plArrayPtr<const plUInt8> sourceData, plGALUpdateMode::Enum updateMode)
{
  AssertRenderingThread();

  PL_ASSERT_DEV(!sourceData.IsEmpty(), "Source data for buffer update is invalid!");

  const plGALBuffer* pDest = m_Device.GetBuffer(hDest);

  if (pDest != nullptr)
  {
    if (updateMode == plGALUpdateMode::NoOverwrite && !(GetDevice().GetCapabilities().m_bNoOverwriteBufferUpdate))
    {
      updateMode = plGALUpdateMode::CopyToTempStorage;
    }

    PL_ASSERT_DEV(pDest->GetSize() >= (uiDestOffset + sourceData.GetCount()), "Buffer {} is too small (or offset {} too big) for {} bytes", pDest->GetSize(), uiDestOffset, sourceData.GetCount());
    m_CommonImpl.UpdateBufferPlatform(pDest, uiDestOffset, sourceData, updateMode);
  }
  else
  {
    PL_REPORT_FAILURE("UpdateBuffer failed, buffer handle invalid");
  }
}

void plGALCommandEncoder::CopyTexture(plGALTextureHandle hDest, plGALTextureHandle hSource)
{
  AssertRenderingThread();

  const plGALTexture* pDest = m_Device.GetTexture(hDest);
  const plGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTexturePlatform(pDest, pSource);
  }
  else
  {
    PL_REPORT_FAILURE("CopyTexture failed, texture handle invalid - destination = {0}, source = {1}", plArgP(pDest), plArgP(pSource));
  }
}

void plGALCommandEncoder::CopyTextureRegion(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource,
  const plVec3U32& vDestinationPoint, plGALTextureHandle hSource, const plGALTextureSubresource& sourceSubResource, const plBoundingBoxu32& box)
{
  AssertRenderingThread();

  const plGALTexture* pDest = m_Device.GetTexture(hDest);
  const plGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.CopyTextureRegionPlatform(pDest, destinationSubResource, vDestinationPoint, pSource, sourceSubResource, box);
  }
  else
  {
    PL_REPORT_FAILURE("CopyTextureRegion failed, texture handle invalid - destination = {0}, source = {1}", plArgP(pDest), plArgP(pSource));
  }
}

void plGALCommandEncoder::UpdateTexture(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource,
  const plBoundingBoxu32& destinationBox, const plGALSystemMemoryDescription& sourceData)
{
  AssertRenderingThread();

  const plGALTexture* pDest = m_Device.GetTexture(hDest);

  if (pDest != nullptr)
  {
    m_CommonImpl.UpdateTexturePlatform(pDest, destinationSubResource, destinationBox, sourceData);
  }
  else
  {
    PL_REPORT_FAILURE("UpdateTexture failed, texture handle invalid - destination = {0}", plArgP(pDest));
  }
}

void plGALCommandEncoder::ResolveTexture(plGALTextureHandle hDest, const plGALTextureSubresource& destinationSubResource, plGALTextureHandle hSource,
  const plGALTextureSubresource& sourceSubResource)
{
  AssertRenderingThread();

  const plGALTexture* pDest = m_Device.GetTexture(hDest);
  const plGALTexture* pSource = m_Device.GetTexture(hSource);

  if (pDest != nullptr && pSource != nullptr)
  {
    m_CommonImpl.ResolveTexturePlatform(pDest, destinationSubResource, pSource, sourceSubResource);
  }
  else
  {
    PL_REPORT_FAILURE("ResolveTexture failed, texture handle invalid - destination = {0}, source = {1}", plArgP(pDest), plArgP(pSource));
  }
}

void plGALCommandEncoder::ReadbackTexture(plGALTextureHandle hTexture)
{
  AssertRenderingThread();

  const plGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    PL_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.ReadbackTexturePlatform(pTexture);
  }
}

void plGALCommandEncoder::CopyTextureReadbackResult(plGALTextureHandle hTexture, plArrayPtr<plGALTextureSubresource> sourceSubResource, plArrayPtr<plGALSystemMemoryDescription> targetData)
{
  AssertRenderingThread();

  const plGALTexture* pTexture = m_Device.GetTexture(hTexture);

  if (pTexture != nullptr)
  {
    PL_ASSERT_RELEASE(pTexture->GetDescription().m_ResourceAccess.m_bReadBack,
      "A texture supplied to read-back needs to be created with the correct resource usage (m_bReadBack = true)!");

    m_CommonImpl.CopyTextureReadbackResultPlatform(pTexture, sourceSubResource, targetData);
  }
}

void plGALCommandEncoder::GenerateMipMaps(plGALResourceViewHandle hResourceView)
{
  AssertRenderingThread();

  const plGALResourceView* pResourceView = m_Device.GetResourceView(hResourceView);
  if (pResourceView != nullptr)
  {
    PL_ASSERT_DEV(!pResourceView->GetDescription().m_hTexture.IsInvalidated(), "Resource view needs a valid texture to generate mip maps.");
    const plGALTexture* pTexture = m_Device.GetTexture(pResourceView->GetDescription().m_hTexture);
    PL_IGNORE_UNUSED(pTexture);
    PL_ASSERT_DEV(pTexture->GetDescription().m_bAllowDynamicMipGeneration,
      "Dynamic mip map generation needs to be enabled (m_bAllowDynamicMipGeneration = true)!");

    m_CommonImpl.GenerateMipMapsPlatform(pResourceView);
  }
}

void plGALCommandEncoder::Flush()
{
  AssertRenderingThread();

  m_CommonImpl.FlushPlatform();
}

// Debug helper functions

void plGALCommandEncoder::PushMarker(const char* szMarker)
{
  AssertRenderingThread();

  PL_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.PushMarkerPlatform(szMarker);
}

void plGALCommandEncoder::PopMarker()
{
  AssertRenderingThread();

  m_CommonImpl.PopMarkerPlatform();
}

void plGALCommandEncoder::InsertEventMarker(const char* szMarker)
{
  AssertRenderingThread();

  PL_ASSERT_DEV(szMarker != nullptr, "Invalid marker!");

  m_CommonImpl.InsertEventMarkerPlatform(szMarker);
}

void plGALCommandEncoder::ClearStatisticsCounters()
{
}

plGALCommandEncoder::plGALCommandEncoder(plGALDevice& device, plGALCommandEncoderState& state, plGALCommandEncoderCommonPlatformInterface& commonImpl)
  : m_Device(device)
  , m_State(state)
  , m_CommonImpl(commonImpl)
{
}

plGALCommandEncoder::~plGALCommandEncoder() = default;

void plGALCommandEncoder::InvalidateState()
{
  m_State.InvalidateState();
}


