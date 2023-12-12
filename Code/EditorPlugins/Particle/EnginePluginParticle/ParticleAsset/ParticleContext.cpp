#include <EnginePluginParticle/EnginePluginParticlePCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <EnginePluginParticle/ParticleAsset/ParticleContext.h>
#include <EnginePluginParticle/ParticleAsset/ParticleView.h>
#include <ParticlePlugin/Components/ParticleComponent.h>
#include <RendererCore/Meshes/MeshComponent.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleContext, 1, plRTTIDefaultAllocator<plParticleContext>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_CONSTANT_PROPERTY("DocumentType", (const char*) "Particle Effect"),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleContext::plParticleContext()
  : PlasmaEngineProcessDocumentContext(PlasmaEngineProcessDocumentContextFlags::CreateWorld)
{
}

plParticleContext::~plParticleContext() = default;

void plParticleContext::HandleMessage(const PlasmaEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<plSimulationSettingsMsgToEngine>())
  {
    // this message comes exactly once per 'update', afterwards there will be 1 to n redraw messages

    auto msg = static_cast<const plSimulationSettingsMsgToEngine*>(pMsg);

    m_pWorld->SetWorldSimulationEnabled(msg->m_bSimulateWorld);
    m_pWorld->GetClock().SetSpeed(msg->m_fSimulationSpeed);
    return;
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<PlasmaEditorEngineRestartSimulationMsg>())
  {
    RestartEffect();
  }

  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<PlasmaEditorEngineLoopAnimationMsg>())
  {
    SetAutoRestartEffect(((const PlasmaEditorEngineLoopAnimationMsg*)pMsg)->m_bLoop);
  }

  PlasmaEngineProcessDocumentContext::HandleMessage(pMsg);
}

void plParticleContext::OnInitialize()
{
  auto pWorld = m_pWorld;
  PLASMA_LOCK(pWorld->GetWriteMarker());

  plParticleComponentManager* pCompMan = pWorld->GetOrCreateComponentManager<plParticleComponentManager>();


  // Preview Effect
  {
    plGameObjectDesc obj;
    plGameObject* pObj;
    obj.m_sName.Assign("ParticlePreview");
    pWorld->CreateObject(obj, pObj);

    pCompMan->CreateComponent(pObj, m_pComponent);
    m_pComponent->m_OnFinishedAction = plOnComponentFinishedAction2::Restart;
    m_pComponent->m_MinRestartDelay = plTime::Seconds(0.5);

    plStringBuilder sParticleGuid;
    plConversionUtils::ToString(GetDocumentGuid(), sParticleGuid);
    m_hParticle = plResourceManager::LoadResource<plParticleEffectResource>(sParticleGuid);

    m_pComponent->SetParticleEffect(m_hParticle);
  }

  const char* szMeshName = "ParticlePreviewBackgroundMesh";
  m_hPreviewMeshResource = plResourceManager::GetExistingResource<plMeshResource>(szMeshName);

  if (!m_hPreviewMeshResource.IsValid())
  {
    const char* szMeshBufferName = "ParticlePreviewBackgroundMeshBuffer";

    plMeshBufferResourceHandle hMeshBuffer = plResourceManager::GetExistingResource<plMeshBufferResource>(szMeshBufferName);

    if (!hMeshBuffer.IsValid())
    {
      // Build geometry
      plGeometry geom;

      geom.AddBox(plVec3(4, 4, 4), true);
      geom.ComputeTangents();

      plMeshBufferResourceDescriptor desc;
      desc.AddCommonStreams();
      desc.AllocateStreamsFromGeometry(geom, plGALPrimitiveTopology::Triangles);

      hMeshBuffer = plResourceManager::GetOrCreateResource<plMeshBufferResource>(szMeshBufferName, std::move(desc), szMeshBufferName);
    }
    {
      plResourceLock<plMeshBufferResource> pMeshBuffer(hMeshBuffer, plResourceAcquireMode::AllowLoadingFallback);

      plMeshResourceDescriptor md;
      md.UseExistingMeshBuffer(hMeshBuffer);
      md.AddSubMesh(pMeshBuffer->GetPrimitiveCount(), 0, 0);
      md.SetMaterial(0, "{ f4b1a490-e549-4968-a16d-a74629154a22 }"); // Pattern.plMaterialAsset
      md.ComputeBounds();

      m_hPreviewMeshResource = plResourceManager::GetOrCreateResource<plMeshResource>(szMeshName, std::move(md), pMeshBuffer->GetResourceDescription());
    }
  }

  plPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();

  // Background Mesh
  {
    plGameObjectDesc obj;
    obj.m_sName.Assign("ParticleBackground");

    const plColor bgColor(0.3f, 0.3f, 0.3f);

    for (int y = -1; y <= 5; ++y)
    {
      for (int x = -5; x <= 5; ++x)
      {
        plGameObject* pObj;
        obj.m_LocalPosition.Set(6, (float)x * 4, 1 + (float)y * 4);
        pWorld->CreateObject(obj, pObj);

        plMeshComponent* pMesh;
        plMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, plVec3(4, 4, 4));
      }
    }

    for (int y = -5; y <= 5; ++y)
    {
      for (int x = -5; x <= 1; ++x)
      {
        plGameObject* pObj;
        obj.m_LocalPosition.Set((float)x * 4, (float)y * 4, -3);
        pWorld->CreateObject(obj, pObj);

        plMeshComponent* pMesh;
        plMeshComponent::CreateComponent(pObj, pMesh);
        pMesh->SetMesh(m_hPreviewMeshResource);
        pMesh->SetColor(bgColor);

        if (pPhysicsInterface)
          pPhysicsInterface->AddStaticCollisionBox(pObj, plVec3(4, 4, 4));
      }
    }

    for (int x = -5; x <= 5; ++x)
    {
      plGameObject* pObj;
      obj.m_LocalPosition.Set(4, (float)x * 4, -2);
      obj.m_LocalRotation.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(45));
      pWorld->CreateObject(obj, pObj);

      plMeshComponent* pMesh;
      plMeshComponent::CreateComponent(pObj, pMesh);
      pMesh->SetMesh(m_hPreviewMeshResource);
      pMesh->SetColor(bgColor);

      if (pPhysicsInterface)
        pPhysicsInterface->AddStaticCollisionBox(pObj, plVec3(4, 4, 4));
    }
  }
}

PlasmaEngineProcessViewContext* plParticleContext::CreateViewContext()
{
  return PLASMA_DEFAULT_NEW(plParticleViewContext, this);
}

void plParticleContext::DestroyViewContext(PlasmaEngineProcessViewContext* pContext)
{
  PLASMA_DEFAULT_DELETE(pContext);
}

void plParticleContext::OnThumbnailViewContextRequested()
{
  m_ThumbnailBoundingVolume.SetInvalid();
}

bool plParticleContext::UpdateThumbnailViewContext(PlasmaEngineProcessViewContext* pThumbnailViewContext)
{
  plParticleViewContext* pParticleViewContext = static_cast<plParticleViewContext*>(pThumbnailViewContext);

  if (!m_ThumbnailBoundingVolume.IsValid())
  {
    PLASMA_LOCK(m_pWorld->GetWriteMarker());

    const bool bWorldPaused = m_pWorld->GetClock().GetPaused();

    // make sure the component restarts as soon as possible
    {
      const plTime restartDelay = m_pComponent->m_MinRestartDelay;
      const auto onFinished = m_pComponent->m_OnFinishedAction;
      const double fClockSpeed = m_pWorld->GetClock().GetSpeed();

      m_pComponent->m_MinRestartDelay.SetZero();
      m_pComponent->m_OnFinishedAction = plOnComponentFinishedAction2::Restart;

      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(false);
      m_pWorld->GetClock().SetSpeed(10);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->GetClock().SetSpeed(fClockSpeed);
      m_pWorld->SetWorldSimulationEnabled(false);

      m_pComponent->m_MinRestartDelay = restartDelay;
      m_pComponent->m_OnFinishedAction = onFinished;
    }

    if (m_pComponent && !m_pComponent->m_EffectController.IsAlive())
    {
      // if this happens, the effect has finished and we need to wait for it to restart, so that it can be reconfigured for the screenshot
      // not very clean solution

      pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
      return false;
    }

    if (m_pComponent && m_pComponent->m_EffectController.IsAlive())
    {
      // set a fixed random seed
      m_pComponent->m_uiRandomSeed = 11;

      const plUInt32 uiMinSimSteps = 3;
      plUInt32 uiSimStepsNeeded = uiMinSimSteps;

      if (m_pComponent->m_EffectController.IsContinuousEffect())
      {
        uiSimStepsNeeded = 30;
      }
      else
      {
        m_pComponent->InterruptEffect();
        m_pComponent->StartEffect();

        plUInt64 uiMostParticles = 0;
        plUInt64 uiMostParticlesStep = 0;

        for (plUInt32 step = 0; step < 30; ++step)
        {
          // step once, to get the initial bbox out of the way
          m_pComponent->m_EffectController.ForceVisible();
          m_pComponent->m_EffectController.Tick(plTime::Seconds(0.05));

          if (!m_pComponent->m_EffectController.IsAlive())
            break;

          const plUInt64 numParticles = m_pComponent->m_EffectController.GetNumActiveParticles();

          if (step == uiMinSimSteps && numParticles > 0)
          {
            uiMostParticles = 0;
          }

          if (numParticles > uiMostParticles)
          {
            // this is the step with the largest number of particles
            // but usually a few steps later is the best step to capture

            uiMostParticles = numParticles;
            uiMostParticlesStep = step;
            uiSimStepsNeeded = step;
          }
          else if ((numParticles > uiMostParticles * 0.8f) && (step < uiMostParticlesStep + 5))
          {
            // if a few steps later we still have a decent amount of particles (so it didn't drop significantly),
            // prefer to use that step
            uiSimStepsNeeded = step;
          }
        }
      }

      m_pComponent->InterruptEffect();
      m_pComponent->StartEffect();

      for (plUInt32 step = 0; step < uiSimStepsNeeded; ++step)
      {
        m_pComponent->m_EffectController.ForceVisible();
        m_pComponent->m_EffectController.Tick(plTime::Seconds(0.05));

        if (m_pComponent->m_EffectController.IsAlive())
        {
          m_pComponent->m_EffectController.GetBoundingVolume(m_ThumbnailBoundingVolume);

          // shrink the bbox to zoom in
          m_ThumbnailBoundingVolume.m_fSphereRadius *= 0.7f;
          m_ThumbnailBoundingVolume.m_vBoxHalfExtends *= 0.7f;
        }
      }

      m_pComponent->m_uiRandomSeed = 0;

      // tick the world once more, so that the effect passes on its bounding box to the culling system
      // otherwise the effect is not rendered, when only the thumbnail is updated, but the document is not open
      m_pWorld->SetWorldSimulationEnabled(true);
      m_pWorld->GetClock().SetPaused(true);
      m_pWorld->Update();
      m_pWorld->GetClock().SetPaused(bWorldPaused);
      m_pWorld->SetWorldSimulationEnabled(false);
    }
  }

  pParticleViewContext->PositionThumbnailCamera(m_ThumbnailBoundingVolume);
  return true;
}

void plParticleContext::RestartEffect()
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->InterruptEffect();
    m_pComponent->StartEffect();
  }
}

void plParticleContext::SetAutoRestartEffect(bool loop)
{
  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  if (m_pComponent)
  {
    m_pComponent->m_OnFinishedAction = loop ? plOnComponentFinishedAction2::Restart : plOnComponentFinishedAction2::None;
  }
}
