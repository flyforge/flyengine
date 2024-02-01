#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Pipeline/ExtractedRenderData.h>
#include <RendererCore/Pipeline/InstanceDataProvider.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererFoundation/Profiling/Profiling.h>

#include <Shaders/Common/ObjectConstants.h>

plInstanceData::plInstanceData(plUInt32 uiMaxInstanceCount /*= 1024*/)

{
  CreateBuffer(uiMaxInstanceCount);

  m_hConstantBuffer = plRenderContext::CreateConstantBufferStorage<plObjectConstants>();
}

plInstanceData::~plInstanceData()
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  pDevice->DestroyBuffer(m_hInstanceDataBuffer);

  plRenderContext::DeleteConstantBufferStorage(m_hConstantBuffer);
}

void plInstanceData::BindResources(plRenderContext* pRenderContext)
{
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  pRenderContext->BindBuffer("perInstanceData", pDevice->GetDefaultResourceView(m_hInstanceDataBuffer));
  pRenderContext->BindConstantBuffer("plObjectConstants", m_hConstantBuffer);
}

plArrayPtr<plPerInstanceData> plInstanceData::GetInstanceData(plUInt32 uiCount, plUInt32& out_uiOffset)
{
  uiCount = plMath::Min(uiCount, m_uiBufferSize);
  if (m_uiBufferOffset + uiCount > m_uiBufferSize)
  {
    m_uiBufferOffset = 0;
  }

  out_uiOffset = m_uiBufferOffset;
  return m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
}

void plInstanceData::UpdateInstanceData(plRenderContext* pRenderContext, plUInt32 uiCount)
{
  PL_ASSERT_DEV(m_uiBufferOffset + uiCount <= m_uiBufferSize, "Implementation error");

  plGALCommandEncoder* pGALCommandEncoder = pRenderContext->GetCommandEncoder();

  plUInt32 uiDestOffset = m_uiBufferOffset * sizeof(plPerInstanceData);
  auto pSourceData = m_PerInstanceData.GetArrayPtr().GetSubArray(m_uiBufferOffset, uiCount);
  plGALUpdateMode::Enum updateMode = (m_uiBufferOffset == 0) ? plGALUpdateMode::Discard : plGALUpdateMode::NoOverwrite;

  pGALCommandEncoder->UpdateBuffer(m_hInstanceDataBuffer, uiDestOffset, pSourceData.ToByteArray(), updateMode);


  plObjectConstants* pConstants = pRenderContext->GetConstantBufferData<plObjectConstants>(m_hConstantBuffer);
  pConstants->InstanceDataOffset = m_uiBufferOffset;

  m_uiBufferOffset += uiCount;
}

void plInstanceData::CreateBuffer(plUInt32 uiSize)
{
  m_uiBufferSize = uiSize;
  m_PerInstanceData.SetCountUninitialized(m_uiBufferSize);

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  plGALBufferCreationDescription desc;
  desc.m_uiStructSize = sizeof(plPerInstanceData);
  desc.m_uiTotalSize = desc.m_uiStructSize * uiSize;
  desc.m_BufferType = plGALBufferType::Generic;
  desc.m_bUseAsStructuredBuffer = true;
  desc.m_bAllowShaderResourceView = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hInstanceDataBuffer = pDevice->CreateBuffer(desc);
}

void plInstanceData::Reset()
{
  m_uiBufferOffset = 0;
}

//////////////////////////////////////////////////////////////////////////

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plInstanceDataProvider, 1, plRTTIDefaultAllocator<plInstanceDataProvider>)
  {
  }
PL_END_DYNAMIC_REFLECTED_TYPE;

plInstanceDataProvider::plInstanceDataProvider() = default;

plInstanceDataProvider::~plInstanceDataProvider() = default;

void* plInstanceDataProvider::UpdateData(const plRenderViewContext& renderViewContext, const plExtractedRenderData& extractedData)
{
  m_Data.Reset();

  return &m_Data;
}



PL_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_InstanceDataProvider);
