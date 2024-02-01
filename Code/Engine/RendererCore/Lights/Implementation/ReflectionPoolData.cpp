#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>

#include <Core/Graphics/Geometry.h>
#include <RendererCore/Lights/BoxReflectionProbeComponent.h>
#include <RendererCore/Lights/SkyLightComponent.h>
#include <RendererCore/Lights/SphereReflectionProbeComponent.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Resources/Texture.h>

//////////////////////////////////////////////////////////////////////////
/// plReflectionPool::Data

plReflectionPool::Data* plReflectionPool::s_pData;

plReflectionPool::Data::Data()
{
  m_SkyIrradianceStorage.SetCount(64);
}

plReflectionPool::Data::~Data()
{
  if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyTexture(m_hFallbackReflectionSpecularTexture);
    m_hFallbackReflectionSpecularTexture.Invalidate();
  }

  plUInt32 uiWorldReflectionCount = m_WorldReflectionData.GetCount();
  for (plUInt32 i = 0; i < uiWorldReflectionCount; ++i)
  {
    WorldReflectionData* pData = m_WorldReflectionData[i].Borrow();
    PL_ASSERT_DEV(!pData || pData->m_Probes.IsEmpty(), "Not all probes were deregistered.");
  }
  m_WorldReflectionData.Clear();

  if (!m_hSkyIrradianceTexture.IsInvalidated())
  {
    plGALDevice::GetDefaultDevice()->DestroyTexture(m_hSkyIrradianceTexture);
    m_hSkyIrradianceTexture.Invalidate();
  }
}

plReflectionProbeId plReflectionPool::Data::AddProbe(const plWorld* pWorld, ProbeData&& probeData)
{
  const plUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex >= s_pData->m_WorldReflectionData.GetCount())
    s_pData->m_WorldReflectionData.SetCount(uiWorldIndex + 1);

  if (s_pData->m_WorldReflectionData[uiWorldIndex] == nullptr)
  {
    s_pData->m_WorldReflectionData[uiWorldIndex] = PL_DEFAULT_NEW(WorldReflectionData);
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId = s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.AddEventHandler([uiWorldIndex, this](const plReflectionProbeMappingEvent& e) {
      OnReflectionProbeMappingEvent(uiWorldIndex, e);
    });
  }

  plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  const plBitflags<plProbeFlags> flags = probeData.m_Flags;
  plReflectionProbeId id = data.m_Probes.Insert(std::move(probeData));

  if (probeData.m_Flags.IsSet(plProbeFlags::SkyLight))
  {
    data.m_SkyLight = id;
  }
  data.m_mapping.AddProbe(id, flags);

  return id;
}

plReflectionPool::Data::WorldReflectionData& plReflectionPool::Data::GetWorldData(const plWorld* pWorld)
{
  const plUInt32 uiWorldIndex = pWorld->GetIndex();
  return *s_pData->m_WorldReflectionData[uiWorldIndex];
}

void plReflectionPool::Data::RemoveProbe(const plWorld* pWorld, plReflectionProbeId id)
{
  const plUInt32 uiWorldIndex = pWorld->GetIndex();
  plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];

  data.m_mapping.RemoveProbe(id);

  if (data.m_SkyLight == id)
  {
    data.m_SkyLight.Invalidate();
  }

  data.m_Probes.Remove(id);

  if (data.m_Probes.IsEmpty())
  {
    s_pData->m_WorldReflectionData[uiWorldIndex]->m_mapping.m_Events.RemoveEventHandler(s_pData->m_WorldReflectionData[uiWorldIndex]->m_mappingSubscriptionId);
    s_pData->m_WorldReflectionData[uiWorldIndex].Clear();
  }
}

void plReflectionPool::Data::UpdateProbeData(ProbeData& ref_probeData, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent)
{
  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (const plSphereReflectionProbeComponent* pSphere = plDynamicCast<const plSphereReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = plProbeFlags::Sphere;
  }
  else if (const plBoxReflectionProbeComponent* pBox = plDynamicCast<const plBoxReflectionProbeComponent*>(pComponent))
  {
    ref_probeData.m_Flags = plProbeFlags::Box;
  }

  if (ref_probeData.m_desc.m_Mode == plReflectionProbeMode::Dynamic)
  {
    ref_probeData.m_Flags |= plProbeFlags::Dynamic;
  }
  else
  {
    plStringBuilder sComponentGuid, sCubeMapFile;
    plConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

    // this is where the editor will put the file for this probe
    sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.plTexture", sComponentGuid);

    ref_probeData.m_hCubeMap = plResourceManager::LoadResource<plTextureCubeResource>(sCubeMapFile);
  }
}

bool plReflectionPool::Data::UpdateSkyLightData(ProbeData& ref_probeData, const plReflectionProbeDesc& desc, const plSkyLightComponent* pComponent)
{
  bool bProbeTypeChanged = false;
  if (ref_probeData.m_desc.m_Mode != desc.m_Mode)
  {
    //#TODO any other reason to unmap a probe.
    bProbeTypeChanged = true;
  }

  ref_probeData.m_desc = desc;
  ref_probeData.m_GlobalTransform = pComponent->GetOwner()->GetGlobalTransform();

  if (auto pSkyLight = plDynamicCast<const plSkyLightComponent*>(pComponent))
  {
    ref_probeData.m_Flags = plProbeFlags::SkyLight;
    ref_probeData.m_hCubeMap = pSkyLight->GetCubeMap();
    if (ref_probeData.m_desc.m_Mode == plReflectionProbeMode::Dynamic)
    {
      ref_probeData.m_Flags |= plProbeFlags::Dynamic;
    }
    else
    {
      if (ref_probeData.m_hCubeMap.IsValid())
      {
        ref_probeData.m_Flags |= plProbeFlags::HasCustomCubeMap;
      }
      else
      {
        plStringBuilder sComponentGuid, sCubeMapFile;
        plConversionUtils::ToString(ref_probeData.m_desc.m_uniqueID, sComponentGuid);

        // this is where the editor will put the file for this probe
        sCubeMapFile.SetFormat(":project/AssetCache/Generated/{0}.plTexture", sComponentGuid);

        ref_probeData.m_hCubeMap = plResourceManager::LoadResource<plTextureCubeResource>(sCubeMapFile);
      }
    }
  }
  return bProbeTypeChanged;
}

void plReflectionPool::Data::OnReflectionProbeMappingEvent(const plUInt32 uiWorldIndex, const plReflectionProbeMappingEvent& e)
{
  switch (e.m_Type)
  {
    case plReflectionProbeMappingEvent::Type::ProbeMapped:
      break;
    case plReflectionProbeMappingEvent::Type::ProbeUnmapped:
    {
      plReflectionProbeRef probeUpdate = {uiWorldIndex, e.m_Id};
      if (m_PendingDynamicUpdate.Contains(probeUpdate))
      {
        m_PendingDynamicUpdate.Remove(probeUpdate);
        m_DynamicUpdateQueue.RemoveAndCopy(probeUpdate);
      }

      if (m_ActiveDynamicUpdate.Contains(probeUpdate))
      {
        m_ActiveDynamicUpdate.Remove(probeUpdate);
        m_ReflectionProbeUpdater.CancelUpdate(probeUpdate);
      }
    }
    break;
    case plReflectionProbeMappingEvent::Type::ProbeUpdateRequested:
    {
      // For now, we just manage a FIFO queue of all dynamic probes that have a high enough priority.
      const plReflectionProbeRef du = {uiWorldIndex, e.m_Id};
      plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
      if (!m_PendingDynamicUpdate.Contains(du))
      {
        m_PendingDynamicUpdate.Insert(du);
        m_DynamicUpdateQueue.PushBack(du);
      }
    }
    break;
  }
}

//////////////////////////////////////////////////////////////////////////
/// Dynamic Update

void plReflectionPool::Data::PreExtraction()
{
  PL_LOCK(s_pData->m_Mutex);
  const plUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();

  for (plUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;

    plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PreExtraction();
  }


  // Schedule new dynamic updates
  {
    plHybridArray<plReflectionProbeRef, 4> updatesFinished;
    const plUInt32 uiCount = plMath::Min(m_ReflectionProbeUpdater.GetFreeUpdateSlots(updatesFinished), m_DynamicUpdateQueue.GetCount());
    for (const plReflectionProbeRef& probe : updatesFinished)
    {
      m_ActiveDynamicUpdate.Remove(probe);

      if (s_pData->m_WorldReflectionData[probe.m_uiWorldIndex] == nullptr)
        continue;

      plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[probe.m_uiWorldIndex];
      data.m_mapping.ProbeUpdateFinished(probe.m_Id);
    }

    for (plUInt32 i = 0; i < uiCount; i++)
    {
      plReflectionProbeRef nextUpdate = m_DynamicUpdateQueue.PeekFront();
      m_DynamicUpdateQueue.PopFront();
      m_PendingDynamicUpdate.Remove(nextUpdate);

      if (s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex] == nullptr)
        continue;

      plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[nextUpdate.m_uiWorldIndex];
      ProbeData& probeData = data.m_Probes.GetValueUnchecked(nextUpdate.m_Id.m_InstanceIndex);

      plReflectionProbeUpdater::TargetSlot target;
      target.m_hSpecularOutputTexture = data.m_mapping.GetTexture();
      target.m_iSpecularOutputIndex = data.m_mapping.GetReflectionIndex(nextUpdate.m_Id);

      if (probeData.m_Flags.IsSet(plProbeFlags::SkyLight))
      {
        target.m_hIrradianceOutputTexture = m_hSkyIrradianceTexture;
        target.m_iIrradianceOutputIndex = nextUpdate.m_uiWorldIndex;
      }

      if (probeData.m_Flags.IsSet(plProbeFlags::HasCustomCubeMap))
      {
        PL_ASSERT_DEBUG(probeData.m_hCubeMap.IsValid(), "");
        PL_VERIFY(m_ReflectionProbeUpdater.StartFilterUpdate(nextUpdate, probeData.m_desc, probeData.m_hCubeMap, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      else
      {
        PL_VERIFY(m_ReflectionProbeUpdater.StartDynamicUpdate(nextUpdate, probeData.m_desc, probeData.m_GlobalTransform, target).Succeeded(), "GetFreeUpdateSlots returned incorrect result");
      }
      m_ActiveDynamicUpdate.Insert(nextUpdate);
    }
    m_ReflectionProbeUpdater.GenerateUpdateSteps();
  }
}

void plReflectionPool::Data::PostExtraction()
{
  PL_LOCK(s_pData->m_Mutex);
  const plUInt32 uiWorldCount = s_pData->m_WorldReflectionData.GetCount();
  for (plUInt32 uiWorld = 0; uiWorld < uiWorldCount; uiWorld++)
  {
    if (s_pData->m_WorldReflectionData[uiWorld] == nullptr)
      continue;
    plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorld];
    data.m_mapping.PostExtraction();
  }
}

//////////////////////////////////////////////////////////////////////////
/// Resource Creation

void plReflectionPool::Data::CreateReflectionViewsAndResources()
{
  if (m_hFallbackReflectionSpecularTexture.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    plGALTextureCreationDescription desc;
    desc.m_uiWidth = s_uiReflectionCubeMapSize;
    desc.m_uiHeight = s_uiReflectionCubeMapSize;
    desc.m_uiMipLevelCount = GetMipLevels();
    desc.m_uiArraySize = 1;
    desc.m_Format = plGALResourceFormat::RGBAHalf;
    desc.m_Type = plGALTextureType::TextureCube;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowUAV = true;
    desc.m_ResourceAccess.m_bReadBack = true;
    desc.m_ResourceAccess.m_bImmutable = false;

    m_hFallbackReflectionSpecularTexture = pDevice->CreateTexture(desc);
    if (!m_hFallbackReflectionSpecularTexture.IsInvalidated())
    {
      pDevice->GetTexture(m_hFallbackReflectionSpecularTexture)->SetDebugName("Reflection Fallback Specular Texture");
    }
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  if (!m_hDebugSphere.IsValid())
  {
    plGeometry geom;
    geom.AddSphere(s_fDebugSphereRadius, 32, 16);

    const char* szBufferResourceName = "ReflectionProbeDebugSphereBuffer";
    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szBufferResourceName);
    if (!hMeshBuffer.IsValid())
    {
      plMeshBufferResourceDescriptor desc;
      desc.AddStream(plGALVertexAttributeSemantic::Position, plGALResourceFormat::XYZFloat);
      desc.AddStream(plGALVertexAttributeSemantic::Normal, plGALResourceFormat::XYZFloat);
      desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

      hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szBufferResourceName, std::move(desc), szBufferResourceName);
    }

    const char* szMeshResourceName = "ReflectionProbeDebugSphere";
    m_hDebugSphere = plResourceManager::GetExistingResource<plMeshResource>(szMeshResourceName);
    if (!m_hDebugSphere.IsValid())
    {
      plMeshResourceDescriptor desc;
      desc.UseExistingMeshBuffer(hMeshBuffer);
      desc.AddSubMesh(geom.CalculateTriangleCount(), 0, 0);
      desc.ComputeBounds();

      m_hDebugSphere = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshResourceName, std::move(desc), szMeshResourceName);
    }
  }

  if (m_hDebugMaterial.IsEmpty())
  {
    const plUInt32 uiMipLevelCount = GetMipLevels();

    plMaterialResourceHandle hDebugMaterial = plResourceManager::LoadResource<plMaterialResource>(
      "{ 6f8067d0-ece8-44e1-af46-79b49266de41 }"); // ReflectionProbeVisualization.plMaterialAsset
    plResourceLock<plMaterialResource> pMaterial(hDebugMaterial, plResourceAcquireMode::BlockTillLoaded);
    if (pMaterial->GetLoadingState() != plResourceState::Loaded)
      return;

    plMaterialResourceDescriptor desc = pMaterial->GetCurrentDesc();
    plUInt32 uiMipLevel = desc.m_Parameters.GetCount();
    plUInt32 uiReflectionProbeIndex = desc.m_Parameters.GetCount();
    plTempHashedString sMipLevelParam = "MipLevel";
    plTempHashedString sReflectionProbeIndexParam = "ReflectionProbeIndex";
    for (plUInt32 i = 0; i < desc.m_Parameters.GetCount(); ++i)
    {
      if (desc.m_Parameters[i].m_Name == sMipLevelParam)
      {
        uiMipLevel = i;
      }
      if (desc.m_Parameters[i].m_Name == sReflectionProbeIndexParam)
      {
        uiReflectionProbeIndex = i;
      }
    }

    if (uiMipLevel >= desc.m_Parameters.GetCount() || uiReflectionProbeIndex >= desc.m_Parameters.GetCount())
      return;

    m_hDebugMaterial.SetCount(uiMipLevelCount * s_uiNumReflectionProbeCubeMaps);
    for (plUInt32 iReflectionProbeIndex = 0; iReflectionProbeIndex < s_uiNumReflectionProbeCubeMaps; iReflectionProbeIndex++)
    {
      for (plUInt32 iMipLevel = 0; iMipLevel < uiMipLevelCount; iMipLevel++)
      {
        desc.m_Parameters[uiMipLevel].m_Value = iMipLevel;
        desc.m_Parameters[uiReflectionProbeIndex].m_Value = iReflectionProbeIndex;
        plStringBuilder sMaterialName;
        sMaterialName.SetFormat("ReflectionProbeVisualization - MipLevel {}, Index {}", iMipLevel, iReflectionProbeIndex);

        plMaterialResourceDescriptor desc2 = desc;
        m_hDebugMaterial[iReflectionProbeIndex * uiMipLevelCount + iMipLevel] = plResourceManager::GetOrCreateResource<plMaterialResource>(sMaterialName, std::move(desc2));
      }
    }
  }
#endif
}

void plReflectionPool::Data::CreateSkyIrradianceTexture()
{
  if (m_hSkyIrradianceTexture.IsInvalidated())
  {
    plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

    plGALTextureCreationDescription desc;
    desc.m_uiWidth = 6;
    desc.m_uiHeight = 64;
    desc.m_Format = plGALResourceFormat::RGBAHalf;
    desc.m_Type = plGALTextureType::Texture2D;
    desc.m_bCreateRenderTarget = true;
    desc.m_bAllowUAV = true;

    m_hSkyIrradianceTexture = pDevice->CreateTexture(desc);
    pDevice->GetTexture(m_hSkyIrradianceTexture)->SetDebugName("Sky Irradiance Texture");
  }
}


