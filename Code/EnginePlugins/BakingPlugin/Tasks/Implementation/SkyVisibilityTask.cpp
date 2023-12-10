#include <BakingPlugin/BakingPluginPCH.h>

#include <BakingPlugin/Tasks/SkyVisibilityTask.h>
#include <BakingPlugin/Tracer/TracerInterface.h>
#include <RendererCore/BakedProbes/BakingInterface.h>

using namespace plBakingInternal;

SkyVisibilityTask::SkyVisibilityTask(const plBakingSettings& settings, plTracerInterface& tracer, plArrayPtr<const plVec3> probePositions)
  : m_Settings(settings)
  , m_Tracer(tracer)
  , m_ProbePositions(probePositions)
{
}

SkyVisibilityTask::~SkyVisibilityTask() = default;

void SkyVisibilityTask::Execute()
{
  m_SkyVisibility.SetCountUninitialized(m_ProbePositions.GetCount());

  const plUInt32 uiNumSamples = m_Settings.m_uiNumSamplesPerProbe;
  plHybridArray<plTracerInterface::Ray, 128> rays(plFrameAllocator::GetCurrentAllocator());
  rays.SetCountUninitialized(uiNumSamples);

  plAmbientCube<float> weightNormalization;
  for (plUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
  {
    auto& ray = rays[uiSampleIndex];
    ray.m_vDir = plBakingUtils::FibonacciSphere(uiSampleIndex, uiNumSamples);
    ray.m_fDistance = m_Settings.m_fMaxRayDistance;

    weightNormalization.AddSample(ray.m_vDir, 1.0f);
  }

  for (plUInt32 i = 0; i < plAmbientCubeBasis::NumDirs; ++i)
  {
    weightNormalization.m_Values[i] = 1.0f / weightNormalization.m_Values[i];
  }

  plHybridArray<plTracerInterface::Hit, 128> hits(plFrameAllocator::GetCurrentAllocator());
  hits.SetCountUninitialized(uiNumSamples);

  for (plUInt32 uiProbeIndex = 0; uiProbeIndex < m_ProbePositions.GetCount(); ++uiProbeIndex)
  {
    plVec3 probePos = m_ProbePositions[uiProbeIndex];
    for (plUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      rays[uiSampleIndex].m_vStartPos = probePos;
    }

    m_Tracer.TraceRays(rays, hits);

    plAmbientCube<float> skyVisibility;
    for (plUInt32 uiSampleIndex = 0; uiSampleIndex < uiNumSamples; ++uiSampleIndex)
    {
      const auto& ray = rays[uiSampleIndex];
      const auto& hit = hits[uiSampleIndex];
      const float value = hit.m_fDistance < 0.0f ? 1.0f : 0.0f;

      skyVisibility.AddSample(ray.m_vDir, value);
    }

    for (plUInt32 i = 0; i < plAmbientCubeBasis::NumDirs; ++i)
    {
      skyVisibility.m_Values[i] *= weightNormalization.m_Values[i];
    }
    auto& compressedSkyVisibility = m_SkyVisibility[uiProbeIndex];
    compressedSkyVisibility = plBakingUtils::CompressSkyVisibility(skyVisibility);
  }
}
