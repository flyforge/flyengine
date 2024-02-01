#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeMapping.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

plReflectionProbeMapping::plReflectionProbeMapping(plUInt32 uiAtlasSize)
  : m_uiAtlasSize(uiAtlasSize)
{
  m_MappedCubes.SetCount(m_uiAtlasSize);
  m_ActiveProbes.Reserve(m_uiAtlasSize);
  m_UnusedProbeSlots.Reserve(m_uiAtlasSize);
  m_AddProbes.Reserve(m_uiAtlasSize);

  PL_ASSERT_DEV(m_hReflectionSpecularTexture.IsInvalidated(), "World data already created.");
  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();
  plGALTextureCreationDescription desc;
  desc.m_uiWidth = s_uiReflectionCubeMapSize;
  desc.m_uiHeight = s_uiReflectionCubeMapSize;
  desc.m_uiMipLevelCount = GetMipLevels();
  desc.m_uiArraySize = s_uiNumReflectionProbeCubeMaps;
  desc.m_Format = plGALResourceFormat::RGBAHalf;
  desc.m_Type = plGALTextureType::TextureCube;
  desc.m_bCreateRenderTarget = true;
  desc.m_bAllowUAV = true;
  desc.m_ResourceAccess.m_bReadBack = true;
  desc.m_ResourceAccess.m_bImmutable = false;

  m_hReflectionSpecularTexture = pDevice->CreateTexture(desc);
  pDevice->GetTexture(m_hReflectionSpecularTexture)->SetDebugName("Reflection Specular Texture");
}

plReflectionProbeMapping::~plReflectionProbeMapping()
{
  PL_ASSERT_DEV(!m_hReflectionSpecularTexture.IsInvalidated(), "World data not created.");
  plGALDevice::GetDefaultDevice()->DestroyTexture(m_hReflectionSpecularTexture);
  m_hReflectionSpecularTexture.Invalidate();
}

void plReflectionProbeMapping::AddProbe(plReflectionProbeId probe, plBitflags<plProbeFlags> flags)
{
  m_RegisteredProbes.EnsureCount(probe.m_InstanceIndex + 1);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  PL_ASSERT_DEBUG(probeData.m_Flags == 0, "");
  probeData.m_id = probe;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(plProbeMappingFlags::Dirty);
  if (probeData.m_Flags.IsSet(plProbeMappingFlags::SkyLight))
  {
    m_SkyLight = probe;
    MapProbe(probe, 0);
  }
}

void plReflectionProbeMapping::UpdateProbe(plReflectionProbeId probe, plBitflags<plProbeFlags> flags)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (!probeData.m_Flags.IsSet(plProbeMappingFlags::SkyLight) && probeData.m_Flags.IsSet(plProbeMappingFlags::Dynamic) != flags.IsSet(plProbeFlags::Dynamic))
  {
    UnmapProbe(probe);
  }
  plBitflags<plProbeMappingFlags> preserveFlags = probeData.m_Flags & plProbeMappingFlags::Usable;
  probeData.m_Flags.SetValue(flags.GetValue());
  probeData.m_Flags.Add(preserveFlags | plProbeMappingFlags::Dirty);
}

void plReflectionProbeMapping::ProbeUpdateFinished(plReflectionProbeId probe)
{
  ProbeDataInternal& probeData0 = m_RegisteredProbes[probe.m_InstanceIndex];
  if (m_SkyLight == probe && probeData0.m_Flags.IsSet(plProbeMappingFlags::Dirty))
  {
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(plProbeMappingFlags::Dirty);
      }
    }
  }
  probeData0.m_Flags.Add(plProbeMappingFlags::Usable);
  probeData0.m_Flags.Remove(plProbeMappingFlags::Dirty);
}

void plReflectionProbeMapping::RemoveProbe(plReflectionProbeId probe)
{
  if (m_SkyLight == probe)
  {
    m_SkyLight.Invalidate();
    // If the sky irradiance changed all other probes are no longer valid and need to be marked as dirty.
    for (ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated() && probeData.m_id != probe)
      {
        probeData.m_Flags.Add(plProbeMappingFlags::Dirty);
      }
    }
  }
  UnmapProbe(probe);
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData = {};
}

plInt32 plReflectionProbeMapping::GetReflectionIndex(plReflectionProbeId probe, bool bForExtraction) const
{
  const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  if (bForExtraction && !probeData.m_Flags.IsSet(plProbeMappingFlags::Usable))
  {
    return -1;
  }
  return probeData.m_uiReflectionIndex;
}

void plReflectionProbeMapping::PreExtraction()
{
  // Reset priorities
  for (ProbeDataInternal& probeData : m_RegisteredProbes)
  {
    probeData.m_fPriority = 0.0f;
  }
  if (!m_SkyLight.IsInvalidated())
  {
    ProbeDataInternal& probeData = m_RegisteredProbes[m_SkyLight.m_InstanceIndex];
    probeData.m_fPriority = plMath::MaxValue<float>();
  }

  m_SortedProbes.Clear();
  m_ActiveProbes.Clear();
  m_UnusedProbeSlots.Clear();
  m_AddProbes.Clear();
}

void plReflectionProbeMapping::AddWeight(plReflectionProbeId probe, float fPriority)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_InstanceIndex];
  probeData.m_fPriority = plMath::Max(probeData.m_fPriority, fPriority);
}

void plReflectionProbeMapping::PostExtraction()
{
  {
    // Sort all active non-skylight probes so we can find the best candidates to evict from the atlas.
    for (plUInt32 i = 1; i < s_uiNumReflectionProbeCubeMaps; i++)
    {
      auto id = m_MappedCubes[i];
      if (!id.IsInvalidated())
      {
        m_ActiveProbes.PushBack({id, m_RegisteredProbes[id.m_InstanceIndex].m_fPriority});
      }
      else
      {
        m_UnusedProbeSlots.PushBack(i);
      }
    }
    m_ActiveProbes.Sort();
  }

  {
    // Sort all exiting probes by priority.
    m_SortedProbes.Reserve(m_RegisteredProbes.GetCount());
    for (const ProbeDataInternal& probeData : m_RegisteredProbes)
    {
      if (!probeData.m_id.IsInvalidated())
      {
        m_SortedProbes.PushBack({probeData.m_id, probeData.m_fPriority});
      }
    }
    m_SortedProbes.Sort();
  }

  {
    // Look at the first N best probes that would ideally be mapped in the atlas and find unmapped ones.
    const plUInt32 uiMaxCount = plMath::Min(m_uiAtlasSize, m_SortedProbes.GetCount());
    for (plUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_SortedProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];

      if (probeData.m_uiReflectionIndex < 0)
      {
        // We found a better probe to be mapped to the atlas.
        m_AddProbes.PushBack(probe);
      }
    }
  }

  {
    // Trigger resource loading of static or updates of dynamic probes.
    const plUInt32 uiMaxCount = m_AddProbes.GetCount();
    for (plUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes& probe = m_AddProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      //#TODO static probe resource loading
    }
  }

  // Unmap probes in case we need free slots using results from last frame
  {
    // Only unmap one probe per frame
    //#TODO better heuristic to decide how many if any should be unmapped.
    if (m_UnusedProbeSlots.GetCount() == 0 && m_AddProbes.GetCount() > 0)
    {
      const SortedProbes probe = m_ActiveProbes.PeekBack();
      UnmapProbe(probe.m_uiIndex);
    }
  }

  // Map probes with higher priority
  {
    const plUInt32 uiMaxCount = plMath::Min(m_AddProbes.GetCount(), m_UnusedProbeSlots.GetCount());
    for (plUInt32 i = 0; i < uiMaxCount; i++)
    {
      plInt32 iReflectionIndex = m_UnusedProbeSlots[i];
      const SortedProbes probe = m_AddProbes[i];
      MapProbe(probe.m_uiIndex, iReflectionIndex);
    }
  }

  // Enqueue dynamic probe updates
  {
    // We add the skylight again as we want to consider it for dynamic updates.
    if (!m_SkyLight.IsInvalidated())
    {
      m_ActiveProbes.PushBack({m_SkyLight, plMath::MaxValue<float>()});
    }
    const plUInt32 uiMaxCount = m_ActiveProbes.GetCount();
    for (plUInt32 i = 0; i < uiMaxCount; i++)
    {
      const SortedProbes probe = m_ActiveProbes[i];
      const ProbeDataInternal& probeData = m_RegisteredProbes[probe.m_uiIndex.m_InstanceIndex];
      if (probeData.m_Flags.IsSet(plProbeMappingFlags::Dynamic))
      {
        plReflectionProbeMappingEvent e = {probeData.m_id, plReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
        m_Events.Broadcast(e);
      }
      else
      {

        //#TODO Add static probes once resources are loaded.
        if (probeData.m_Flags.IsSet(plProbeMappingFlags::Dirty))
        {
          plReflectionProbeMappingEvent e = {probeData.m_id, plReflectionProbeMappingEvent::Type::ProbeUpdateRequested};
          m_Events.Broadcast(e);
        }
      }
    }
  }
}

void plReflectionProbeMapping::MapProbe(plReflectionProbeId id, plInt32 iReflectionIndex)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];

  probeData.m_uiReflectionIndex = iReflectionIndex;
  m_MappedCubes[probeData.m_uiReflectionIndex] = id;
  m_ActiveProbes.PushBack({id, 0.0f});

  plReflectionProbeMappingEvent e = {id, plReflectionProbeMappingEvent::Type::ProbeMapped};
  m_Events.Broadcast(e);
}

void plReflectionProbeMapping::UnmapProbe(plReflectionProbeId id)
{
  ProbeDataInternal& probeData = m_RegisteredProbes[id.m_InstanceIndex];
  if (probeData.m_uiReflectionIndex != -1)
  {
    m_MappedCubes[probeData.m_uiReflectionIndex].Invalidate();
    probeData.m_uiReflectionIndex = -1;

    plReflectionProbeMappingEvent e = {id, plReflectionProbeMappingEvent::Type::ProbeUnmapped};
    m_Events.Broadcast(e);
  }
}


