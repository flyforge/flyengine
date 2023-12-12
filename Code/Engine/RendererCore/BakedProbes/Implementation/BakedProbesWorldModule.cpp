#include <RendererCore/RendererCorePCH.h>

#include <Foundation/SimdMath/SimdConversion.h>
#include <Foundation/SimdMath/SimdVec4i.h>
#include <RendererCore/BakedProbes/BakedProbesWorldModule.h>
#include <RendererCore/BakedProbes/ProbeTreeSectorResource.h>

// clang-format off
PLASMA_IMPLEMENT_WORLD_MODULE(plBakedProbesWorldModule);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plBakedProbesWorldModule, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plBakedProbesWorldModule::plBakedProbesWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plBakedProbesWorldModule::~plBakedProbesWorldModule() = default;

void plBakedProbesWorldModule::Initialize()
{
}

void plBakedProbesWorldModule::Deinitialize()
{
}

bool plBakedProbesWorldModule::HasProbeData() const
{
  return m_hProbeTree.IsValid();
}

plResult plBakedProbesWorldModule::GetProbeIndexData(const plVec3& vGlobalPosition, const plVec3& vNormal, ProbeIndexData& out_probeIndexData) const
{
  // TODO: optimize

  if (!HasProbeData())
    return PLASMA_FAILURE;

  plResourceLock<plProbeTreeSectorResource> pProbeTree(m_hProbeTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != plResourceAcquireResult::Final)
    return PLASMA_FAILURE;

  plSimdVec4f gridSpacePos = plSimdConversion::ToVec3((vGlobalPosition - pProbeTree->GetGridOrigin()).CompDiv(pProbeTree->GetProbeSpacing()));
  gridSpacePos = gridSpacePos.CompMax(plSimdVec4f::ZeroVector());

  plSimdVec4f gridSpacePosFloor = gridSpacePos.Floor();
  plSimdVec4f weights = gridSpacePos - gridSpacePosFloor;

  plSimdVec4i maxIndices = plSimdVec4i(pProbeTree->GetProbeCount().x, pProbeTree->GetProbeCount().y, pProbeTree->GetProbeCount().z) - plSimdVec4i(1);
  plSimdVec4i pos0 = plSimdVec4i::Truncate(gridSpacePosFloor).CompMin(maxIndices);
  plSimdVec4i pos1 = (pos0 + plSimdVec4i(1)).CompMin(maxIndices);

  plUInt32 x0 = pos0.x();
  plUInt32 y0 = pos0.y();
  plUInt32 z0 = pos0.z();

  plUInt32 x1 = pos1.x();
  plUInt32 y1 = pos1.y();
  plUInt32 z1 = pos1.z();

  plUInt32 xCount = pProbeTree->GetProbeCount().x;
  plUInt32 xyCount = xCount * pProbeTree->GetProbeCount().y;

  out_probeIndexData.m_probeIndices[0] = z0 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[1] = z0 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[2] = z0 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[3] = z0 * xyCount + y1 * xCount + x1;
  out_probeIndexData.m_probeIndices[4] = z1 * xyCount + y0 * xCount + x0;
  out_probeIndexData.m_probeIndices[5] = z1 * xyCount + y0 * xCount + x1;
  out_probeIndexData.m_probeIndices[6] = z1 * xyCount + y1 * xCount + x0;
  out_probeIndexData.m_probeIndices[7] = z1 * xyCount + y1 * xCount + x1;

  plVec3 w1 = plSimdConversion::ToVec3(weights);
  plVec3 w0 = plVec3(1.0f) - w1;

  // TODO: add geometry factor to weight
  out_probeIndexData.m_probeWeights[0] = w0.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[1] = w1.x * w0.y * w0.z;
  out_probeIndexData.m_probeWeights[2] = w0.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[3] = w1.x * w1.y * w0.z;
  out_probeIndexData.m_probeWeights[4] = w0.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[5] = w1.x * w0.y * w1.z;
  out_probeIndexData.m_probeWeights[6] = w0.x * w1.y * w1.z;
  out_probeIndexData.m_probeWeights[7] = w1.x * w1.y * w1.z;

  float weightSum = 0;
  for (plUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    weightSum += out_probeIndexData.m_probeWeights[i];
  }

  float normalizeFactor = 1.0f / weightSum;
  for (plUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    out_probeIndexData.m_probeWeights[i] *= normalizeFactor;
  }

  return PLASMA_SUCCESS;
}

plAmbientCube<float> plBakedProbesWorldModule::GetSkyVisibility(const ProbeIndexData& indexData) const
{
  // TODO: optimize

  plAmbientCube<float> result;

  plResourceLock<plProbeTreeSectorResource> pProbeTree(m_hProbeTree, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pProbeTree.GetAcquireResult() != plResourceAcquireResult::Final)
    return result;

  auto compressedSkyVisibility = pProbeTree->GetSkyVisibility();
  plAmbientCube<float> skyVisibility;

  for (plUInt32 i = 0; i < ProbeIndexData::NumProbes; ++i)
  {
    plBakingUtils::DecompressSkyVisibility(compressedSkyVisibility[indexData.m_probeIndices[i]], skyVisibility);

    for (plUInt32 d = 0; d < plAmbientCubeBasis::NumDirs; ++d)
    {
      result.m_Values[d] += skyVisibility.m_Values[d] * indexData.m_probeWeights[i];
    }
  }

  return result;
}

void plBakedProbesWorldModule::SetProbeTreeResourcePrefix(const plHashedString& prefix)
{
  plStringBuilder sResourcePath;
  sResourcePath.Format("{}_Global.plProbeTreeSector", prefix);

  m_hProbeTree = plResourceManager::LoadResource<plProbeTreeSectorResource>(sResourcePath);
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakedProbesWorldModule);
