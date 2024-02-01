#include <RendererCore/RendererCorePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Meshes/SkinnedMeshComponent.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererFoundation/Shader/Types.h>
#include <RendererFoundation/Device/Device.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSkinnedMeshRenderData, 1, plRTTIDefaultAllocator<plSkinnedMeshRenderData>)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plSkinnedMeshRenderData::FillBatchIdAndSortingKey()
{
  FillBatchIdAndSortingKeyInternal(m_hSkinningTransforms.GetInternalID().m_Data);
}

plSkinningState::plSkinningState() = default;

plSkinningState::~plSkinningState()
{
  Clear();
}

void plSkinningState::Clear()
{
  if (!m_hGpuBuffer.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyBuffer(m_hGpuBuffer);
    m_hGpuBuffer.Invalidate();
  }

  m_bTransformsUpdated[0] = nullptr;
  m_bTransformsUpdated[1] = nullptr;
  m_Transforms.Clear();
}

void plSkinningState::TransformsChanged()
{
  if (m_hGpuBuffer.IsInvalidated())
  {
    if (m_Transforms.GetCount() == 0)
      return;

    plGALBufferCreationDescription BufferDesc;
    BufferDesc.m_uiStructSize = sizeof(plShaderTransform);
    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_Transforms.GetCount();
    BufferDesc.m_bUseAsStructuredBuffer = true;
    BufferDesc.m_bAllowShaderResourceView = true;
    BufferDesc.m_ResourceAccess.m_bImmutable = false;

    m_hGpuBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(BufferDesc, m_Transforms.GetArrayPtr().ToByteArray());

    m_bTransformsUpdated[0] = std::make_shared<bool>(true);
    m_bTransformsUpdated[1] = std::make_shared<bool>(true);
  }
  else
  {
    const plUInt32 uiRenIdx = plRenderWorld::GetDataIndexForExtraction();
    *m_bTransformsUpdated[uiRenIdx] = false;
  }
}

void plSkinningState::FillSkinnedMeshRenderData(plSkinnedMeshRenderData& ref_renderData) const
{
  ref_renderData.m_hSkinningTransforms = m_hGpuBuffer;

  const plUInt32 uiExIdx = plRenderWorld::GetDataIndexForExtraction();

  if (m_bTransformsUpdated[uiExIdx] && *m_bTransformsUpdated[uiExIdx] == false)
  {
    auto pSkinningMatrices = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plShaderTransform, m_Transforms.GetCount());
    pSkinningMatrices.CopyFrom(m_Transforms);

    ref_renderData.m_pNewSkinningTransformData = pSkinningMatrices.ToByteArray();
    ref_renderData.m_bTransformsUpdated = m_bTransformsUpdated[uiExIdx];
  }
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Meshes_Implementation_SkinnedMeshComponent);
