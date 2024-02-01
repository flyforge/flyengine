#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Configuration/Startup.h>
#include <Foundation/Math/Color16f.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/GPUResourcePool/GPUResourcePool.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Lights/Implementation/ReflectionPoolData.h>
#include <RendererCore/Lights/Implementation/ReflectionProbeData.h>
#include <RendererCore/Meshes/MeshComponentBase.h>
#include <RendererCore/Pipeline/Declarations.h>
#include <RendererCore/RenderContext/RenderContext.h>
#include <RendererCore/RenderWorld/RenderWorld.h>
#include <RendererCore/Textures/TextureCubeResource.h>
#include <RendererFoundation/CommandEncoder/ComputeCommandEncoder.h>
#include <RendererFoundation/Device/Device.h>
#include <RendererFoundation/Device/Pass.h>
#include <RendererFoundation/Profiling/Profiling.h>
#include <RendererFoundation/Resources/Texture.h>

// clang-format off
PL_BEGIN_SUBSYSTEM_DECLARATION(RendererCore, ReflectionPool)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation",
    "Core",
    "RenderWorld"
  END_SUBSYSTEM_DEPENDENCIES

  ON_HIGHLEVELSYSTEMS_STARTUP
  {
    plReflectionPool::OnEngineStartup();
  }

  ON_HIGHLEVELSYSTEMS_SHUTDOWN
  {
    plReflectionPool::OnEngineShutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

//////////////////////////////////////////////////////////////////////////
/// plReflectionPool

plReflectionProbeId plReflectionPool::RegisterReflectionProbe(const plWorld* pWorld, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent)
{
  PL_LOCK(s_pData->m_Mutex);

  Data::ProbeData probe;
  s_pData->UpdateProbeData(probe, desc, pComponent);
  return s_pData->AddProbe(pWorld, std::move(probe));
}

void plReflectionPool::DeregisterReflectionProbe(const plWorld* pWorld, plReflectionProbeId id)
{
  PL_LOCK(s_pData->m_Mutex);
  s_pData->RemoveProbe(pWorld, id);
}

void plReflectionPool::UpdateReflectionProbe(const plWorld* pWorld, plReflectionProbeId id, const plReflectionProbeDesc& desc, const plReflectionProbeComponentBase* pComponent)
{
  PL_LOCK(s_pData->m_Mutex);
  plReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  s_pData->UpdateProbeData(probeData, desc, pComponent);
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

void plReflectionPool::ExtractReflectionProbe(const plComponent* pComponent, plMsgExtractRenderData& ref_msg, plReflectionProbeRenderData* pRenderData0, const plWorld* pWorld, plReflectionProbeId id, float fPriority)
{
  PL_LOCK(s_pData->m_Mutex);
  s_pData->m_ReflectionProbeUpdater.ScheduleUpdateSteps();

  const plUInt32 uiWorldIndex = pWorld->GetIndex();
  plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[uiWorldIndex];
  data.m_mapping.AddWeight(id, fPriority);
  const plInt32 iMappedIndex = data.m_mapping.GetReflectionIndex(id, true);

  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);

  if (pComponent->GetOwner()->IsDynamic())
  {
    plTransform globalTransform = pComponent->GetOwner()->GetGlobalTransform();
    if (!probeData.m_Flags.IsSet(plProbeFlags::Dynamic) && probeData.m_GlobalTransform != globalTransform)
    {
      data.m_mapping.UpdateProbe(id, probeData.m_Flags);
    }
    probeData.m_GlobalTransform = globalTransform;
  }

  // The sky light is always active and not added to the render data (always passes in nullptr as pRenderData).
  if (pRenderData0 && iMappedIndex > 0)
  {
    // Index and flags are stored in m_uiIndex so we can't just overwrite it.
    pRenderData0->m_uiIndex |= (plUInt32)iMappedIndex;
    ref_msg.AddRenderData(pRenderData0, plDefaultRenderDataCategories::ReflectionProbe, plRenderData::Caching::Never);
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  const plUInt32 uiMipLevels = GetMipLevels();
  if (probeData.m_desc.m_bShowDebugInfo && s_pData->m_hDebugMaterial.GetCount() == uiMipLevels * s_uiNumReflectionProbeCubeMaps)
  {
    if (ref_msg.m_OverrideCategory == plInvalidRenderDataCategory)
    {
      plInt32 activeIndex = 0;
      if (s_pData->m_ActiveDynamicUpdate.Contains(plReflectionProbeRef{uiWorldIndex, id}))
      {
        activeIndex = 1;
      }

      plStringBuilder sEnum;
      plReflectionUtils::BitflagsToString(probeData.m_Flags, sEnum, plReflectionUtils::EnumConversionMode::ValueNameOnly);
      plStringBuilder s;
      s.SetFormat("\n RefIdx: {}\nUpdating: {}\nFlags: {}\n", iMappedIndex, activeIndex, sEnum);
      plDebugRenderer::Draw3DText(pWorld, s, pComponent->GetOwner()->GetGlobalPosition(), plColorScheme::LightUI(plColorScheme::Violet));
    }

    // Not mapped in the atlas - cannot render it.
    if (iMappedIndex < 0)
      return;

    const plGameObject* pOwner = pComponent->GetOwner();
    const plTransform ownerTransform = pOwner->GetGlobalTransform();

    plUInt32 uiMipLevelsToRender = probeData.m_desc.m_bShowMipMaps ? uiMipLevels : 1;
    for (plUInt32 i = 0; i < uiMipLevelsToRender; i++)
    {
      plMeshRenderData* pRenderData = plCreateRenderDataForThisFrame<plMeshRenderData>(pOwner);
      pRenderData->m_GlobalTransform.m_vPosition = ownerTransform * probeData.m_desc.m_vCaptureOffset;
      pRenderData->m_GlobalTransform.m_vScale = plVec3(1.0f);
      if (!probeData.m_Flags.IsSet(plProbeFlags::SkyLight))
      {
        pRenderData->m_GlobalTransform.m_qRotation = ownerTransform.m_qRotation;
      }
      pRenderData->m_GlobalTransform.m_vPosition.z += s_fDebugSphereRadius * i * 2;
      pRenderData->m_GlobalBounds = pOwner->GetGlobalBounds();
      pRenderData->m_hMesh = s_pData->m_hDebugSphere;
      pRenderData->m_hMaterial = s_pData->m_hDebugMaterial[iMappedIndex * uiMipLevels + i];
      pRenderData->m_Color = plColor::White;
      pRenderData->m_uiSubMeshIndex = 0;
      pRenderData->m_uiUniqueID = plRenderComponent::GetUniqueIdForRendering(*pComponent, 0);

      pRenderData->FillBatchIdAndSortingKey();
      ref_msg.AddRenderData(pRenderData, plDefaultRenderDataCategories::LitOpaque, plRenderData::Caching::Never);
    }
  }
#endif
}

//////////////////////////////////////////////////////////////////////////
/// SkyLight

plReflectionProbeId plReflectionPool::RegisterSkyLight(const plWorld* pWorld, plReflectionProbeDesc& ref_desc, const plSkyLightComponent* pComponent)
{
  PL_LOCK(s_pData->m_Mutex);
  const plUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight |= PL_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= PL_BIT(uiWorldIndex);

  Data::ProbeData probe;
  s_pData->UpdateSkyLightData(probe, ref_desc, pComponent);

  plReflectionProbeId id = s_pData->AddProbe(pWorld, std::move(probe));
  return id;
}

void plReflectionPool::DeregisterSkyLight(const plWorld* pWorld, plReflectionProbeId id)
{
  PL_LOCK(s_pData->m_Mutex);

  s_pData->RemoveProbe(pWorld, id);

  const plUInt32 uiWorldIndex = pWorld->GetIndex();
  s_pData->m_uiWorldHasSkyLight &= ~PL_BIT(uiWorldIndex);
  s_pData->m_uiSkyIrradianceChanged |= PL_BIT(uiWorldIndex);
}

void plReflectionPool::UpdateSkyLight(const plWorld* pWorld, plReflectionProbeId id, const plReflectionProbeDesc& desc, const plSkyLightComponent* pComponent)
{
  PL_LOCK(s_pData->m_Mutex);
  plReflectionPool::Data::WorldReflectionData& data = s_pData->GetWorldData(pWorld);
  Data::ProbeData& probeData = data.m_Probes.GetValueUnchecked(id.m_InstanceIndex);
  if (s_pData->UpdateSkyLightData(probeData, desc, pComponent))
  {
    // s_pData->UnmapProbe(pWorld->GetIndex(), data, id);
  }
  data.m_mapping.UpdateProbe(id, probeData.m_Flags);
}

//////////////////////////////////////////////////////////////////////////
/// Misc

// static
void plReflectionPool::SetConstantSkyIrradiance(const plWorld* pWorld, const plAmbientCube<plColor>& skyIrradiance)
{
  plUInt32 uiWorldIndex = pWorld->GetIndex();
  plAmbientCube<plColorLinear16f> skyIrradiance16f = skyIrradiance;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != skyIrradiance16f)
  {
    skyIrradianceStorage[uiWorldIndex] = skyIrradiance16f;

    s_pData->m_uiSkyIrradianceChanged |= PL_BIT(uiWorldIndex);
  }
}

void plReflectionPool::ResetConstantSkyIrradiance(const plWorld* pWorld)
{
  plUInt32 uiWorldIndex = pWorld->GetIndex();

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;
  if (skyIrradianceStorage[uiWorldIndex] != plAmbientCube<plColorLinear16f>())
  {
    skyIrradianceStorage[uiWorldIndex] = plAmbientCube<plColorLinear16f>();

    s_pData->m_uiSkyIrradianceChanged |= PL_BIT(uiWorldIndex);
  }
}

// static
plUInt32 plReflectionPool::GetReflectionCubeMapSize()
{
  return s_uiReflectionCubeMapSize;
}

// static
plGALTextureHandle plReflectionPool::GetReflectionSpecularTexture(plUInt32 uiWorldIndex, plEnum<plCameraUsageHint> cameraUsageHint)
{
  if (uiWorldIndex < s_pData->m_WorldReflectionData.GetCount() && cameraUsageHint != plCameraUsageHint::Reflection)
  {
    Data::WorldReflectionData* pData = s_pData->m_WorldReflectionData[uiWorldIndex].Borrow();
    if (pData)
      return pData->m_mapping.GetTexture();
  }
  return s_pData->m_hFallbackReflectionSpecularTexture;
}

// static
plGALTextureHandle plReflectionPool::GetSkyIrradianceTexture()
{
  return s_pData->m_hSkyIrradianceTexture;
}

//////////////////////////////////////////////////////////////////////////
/// Private Functions

// static
void plReflectionPool::OnEngineStartup()
{
  s_pData = PL_DEFAULT_NEW(plReflectionPool::Data);

  plRenderWorld::GetExtractionEvent().AddEventHandler(OnExtractionEvent);
  plRenderWorld::GetRenderEvent().AddEventHandler(OnRenderEvent);
}

// static
void plReflectionPool::OnEngineShutdown()
{
  plRenderWorld::GetExtractionEvent().RemoveEventHandler(OnExtractionEvent);
  plRenderWorld::GetRenderEvent().RemoveEventHandler(OnRenderEvent);

  PL_DEFAULT_DELETE(s_pData);
}

// static
void plReflectionPool::OnExtractionEvent(const plRenderWorldExtractionEvent& e)
{
  if (e.m_Type == plRenderWorldExtractionEvent::Type::BeginExtraction)
  {
    PL_PROFILE_SCOPE("Reflection Pool BeginExtraction");
    s_pData->CreateSkyIrradianceTexture();
    s_pData->CreateReflectionViewsAndResources();
    s_pData->PreExtraction();
  }

  if (e.m_Type == plRenderWorldExtractionEvent::Type::EndExtraction)
  {
    PL_PROFILE_SCOPE("Reflection Pool EndExtraction");
    s_pData->PostExtraction();
  }
}

// static
void plReflectionPool::OnRenderEvent(const plRenderWorldRenderEvent& e)
{
  if (e.m_Type != plRenderWorldRenderEvent::Type::BeginRender)
    return;

  if (s_pData->m_hSkyIrradianceTexture.IsInvalidated())
    return;

  PL_LOCK(s_pData->m_Mutex);

  plUInt64 uiWorldHasSkyLight = s_pData->m_uiWorldHasSkyLight;
  plUInt64 uiSkyIrradianceChanged = s_pData->m_uiSkyIrradianceChanged;
  if ((~uiWorldHasSkyLight & uiSkyIrradianceChanged) == 0)
    return;

  auto& skyIrradianceStorage = s_pData->m_SkyIrradianceStorage;

  plGALDevice* pDevice = plGALDevice::GetDefaultDevice();

  auto pGALPass = pDevice->BeginPass("Sky Irradiance Texture Update");
  plHybridArray<plGALTextureHandle, 4> atlasToClear;

  {
    auto pGALCommandEncoder = pGALPass->BeginCompute();
    for (plUInt32 i = 0; i < skyIrradianceStorage.GetCount(); ++i)
    {
      if ((uiWorldHasSkyLight & PL_BIT(i)) == 0 && (uiSkyIrradianceChanged & PL_BIT(i)) != 0)
      {
        plBoundingBoxu32 destBox;
        destBox.m_vMin.Set(0, i, 0);
        destBox.m_vMax.Set(6, i + 1, 1);
        plGALSystemMemoryDescription memDesc;
        memDesc.m_pData = &skyIrradianceStorage[i].m_Values[0];
        memDesc.m_uiRowPitch = sizeof(plAmbientCube<plColorLinear16f>);
        pGALCommandEncoder->UpdateTexture(s_pData->m_hSkyIrradianceTexture, plGALTextureSubresource(), destBox, memDesc);

        uiSkyIrradianceChanged &= ~PL_BIT(i);

        if (i < s_pData->m_WorldReflectionData.GetCount() && s_pData->m_WorldReflectionData[i] != nullptr)
        {
          plReflectionPool::Data::WorldReflectionData& data = *s_pData->m_WorldReflectionData[i];
          atlasToClear.PushBack(data.m_mapping.GetTexture());
        }
      }
    }
    pGALPass->EndCompute(pGALCommandEncoder);
  }

  {
    // Clear specular sky reflection to black.
    const plUInt32 uiNumMipMaps = GetMipLevels();
    for (plGALTextureHandle atlas : atlasToClear)
    {
      for (plUInt32 uiMipMapIndex = 0; uiMipMapIndex < uiNumMipMaps; ++uiMipMapIndex)
      {
        for (plUInt32 uiFaceIndex = 0; uiFaceIndex < 6; ++uiFaceIndex)
        {
          plGALRenderingSetup renderingSetup;
          plGALRenderTargetViewCreationDescription desc;
          desc.m_hTexture = atlas;
          desc.m_uiMipLevel = uiMipMapIndex;
          desc.m_uiFirstSlice = uiFaceIndex;
          desc.m_uiSliceCount = 1;
          renderingSetup.m_RenderTargetSetup.SetRenderTarget(0, pDevice->CreateRenderTargetView(desc));
          renderingSetup.m_ClearColor = plColor(0, 0, 0, 1);
          renderingSetup.m_uiRenderTargetClearMask = 0xFFFFFFFF;

          auto pGALCommandEncoder = pGALPass->BeginRendering(renderingSetup, "ClearSkySpecular");
          pGALCommandEncoder->Clear(plColor::Black);
          pGALPass->EndRendering(pGALCommandEncoder);
        }
      }
    }
  }

  pDevice->EndPass(pGALPass);
}


PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionPool);
