#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/SortingFunctions.h>

namespace
{
  PLASMA_ALWAYS_INLINE plUInt32 CalculateTypeHash(const plRenderData* pRenderData)
  {
    plUInt32 uiTypeHash = plHashingUtils::StringHashTo32(pRenderData->GetDynamicRTTI()->GetTypeNameHash());
    return (uiTypeHash >> 16) ^ (uiTypeHash & 0xFFFF);
  }

  PLASMA_FORCE_INLINE plUInt32 CalculateDistance(const plRenderData* pRenderData, const plCamera& camera)
  {
    ///\todo far-plane is not enough to normalize distance
    const float fDistance = (camera.GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength() + pRenderData->m_fSortingDepthOffset;
    const float fNormalizedDistance = plMath::Clamp(fDistance / camera.GetFarPlane(), 0.0f, 1.0f);
    return static_cast<plUInt32>(fNormalizedDistance * 65535.0f);
  }
} // namespace

// static
plUInt64 plRenderSortingFunctions::ByRenderDataThenFrontToBack(const plRenderData* pRenderData, const plCamera& camera)
{
  const plUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const plUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const plUInt64 uiDistance = CalculateDistance(pRenderData, camera);

  const plUInt64 uiSortingKey = (uiTypeHash << 48) | (uiRenderDataSortingKey64 << 16) | uiDistance;
  return uiSortingKey;
}

// static
plUInt64 plRenderSortingFunctions::BackToFrontThenByRenderData(const plRenderData* pRenderData, const plCamera& camera)
{
  const plUInt64 uiTypeHash = CalculateTypeHash(pRenderData);
  const plUInt64 uiRenderDataSortingKey64 = pRenderData->m_uiSortingKey;
  const plUInt64 uiInvDistance = 0xFFFF - CalculateDistance(pRenderData, camera);

  const plUInt64 uiSortingKey = (uiInvDistance << 48) | (uiTypeHash << 32) | uiRenderDataSortingKey64;
  return uiSortingKey;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Pipeline_Implementation_SortingFunctions);
