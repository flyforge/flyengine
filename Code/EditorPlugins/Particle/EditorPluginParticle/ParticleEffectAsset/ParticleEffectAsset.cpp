#include <EditorPluginParticle/EditorPluginParticlePCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>
#include <EditorPluginParticle/ParticleEffectAsset/ParticleEffectAsset.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/PropertyGrid/VisualizerManager.h>
#include <ParticlePlugin/Behavior/ParticleBehavior_ColorGradient.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_CylinderPosition.h>
#include <ParticlePlugin/Initializer/ParticleInitializer_SpherePosition.h>
#include <ParticlePlugin/System/ParticleSystemDescriptor.h>
#include <ParticlePlugin/Type/Quad/ParticleTypeQuad.h>
#include <ParticlePlugin/Type/Trail/ParticleTypeTrail.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plParticleEffectAssetDocument, 6, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plParticleEffectAssetDocument::plParticleEffectAssetDocument(plStringView sDocumentPath)
  : plSimpleAssetDocument<plParticleEffectDescriptor>(sDocumentPath, plAssetDocEngineConnection::Simple, true)
{
  plVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);
}

void plParticleEffectAssetDocument::PropertyMetaStateEventHandler(plPropertyMetaStateEvent& e)
{
  if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleEffectDescriptor>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bShared = e.m_pObject->GetTypeAccessor().GetValue("AlwaysShared").ConvertTo<bool>();

    props["SimulateInLocalSpace"].m_Visibility = bShared ? plPropertyUiState::Disabled : plPropertyUiState::Default;
    props["ApplyOwnerVelocity"].m_Visibility = bShared ? plPropertyUiState::Disabled : plPropertyUiState::Default;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleTypeQuadFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    plInt64 orientation = e.m_pObject->GetTypeAccessor().GetValue("Orientation").ConvertTo<plInt64>();
    plInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<plInt64>();
    plInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<plInt64>();

    props["Deviation"].m_Visibility = plPropertyUiState::Invisible;
    props["DistortionTexture"].m_Visibility = plPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = plPropertyUiState::Invisible;
    props["ParticleStretch"].m_Visibility =
      (orientation == plQuadParticleOrientation::FixedAxis_EmitterDir || orientation == plQuadParticleOrientation::FixedAxis_ParticleDir)
        ? plPropertyUiState::Default
        : plPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)plParticleTextureAtlasType::None) ? plPropertyUiState::Invisible : plPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)plParticleTextureAtlasType::None) ? plPropertyUiState::Invisible : plPropertyUiState::Default;


    if (orientation == plQuadParticleOrientation::Fixed_EmitterDir || orientation == plQuadParticleOrientation::Fixed_WorldUp)
    {
      props["Deviation"].m_Visibility = plPropertyUiState::Default;
    }

    if (renderMode == plParticleTypeRenderMode::Distortion)
    {
      props["DistortionTexture"].m_Visibility = plPropertyUiState::Default;
      props["DistortionStrength"].m_Visibility = plPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleTypeTrailFactory>())
  {
    auto& props = *e.m_pPropertyStates;

    plInt64 renderMode = e.m_pObject->GetTypeAccessor().GetValue("RenderMode").ConvertTo<plInt64>();
    plInt64 textureAtlas = e.m_pObject->GetTypeAccessor().GetValue("TextureAtlas").ConvertTo<plInt64>();

    props["DistortionTexture"].m_Visibility = plPropertyUiState::Invisible;
    props["DistortionStrength"].m_Visibility = plPropertyUiState::Invisible;
    props["NumSpritesX"].m_Visibility =
      (textureAtlas == (int)plParticleTextureAtlasType::None) ? plPropertyUiState::Invisible : plPropertyUiState::Default;
    props["NumSpritesY"].m_Visibility =
      (textureAtlas == (int)plParticleTextureAtlasType::None) ? plPropertyUiState::Invisible : plPropertyUiState::Default;

    if (renderMode == plParticleTypeRenderMode::Distortion)
    {
      props["DistortionTexture"].m_Visibility = plPropertyUiState::Default;
      props["DistortionStrength"].m_Visibility = plPropertyUiState::Default;
    }
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleBehaviorFactory_ColorGradient>())
  {
    auto& props = *e.m_pPropertyStates;

    plInt64 mode = e.m_pObject->GetTypeAccessor().GetValue("ColorGradientMode").ConvertTo<plInt64>();

    props["GradientMaxSpeed"].m_Visibility = (mode == plParticleColorGradientMode::Speed) ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleInitializerFactory_CylinderPosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  }
  else if (e.m_pObject->GetTypeAccessor().GetType() == plGetStaticRTTI<plParticleInitializerFactory_SpherePosition>())
  {
    auto& props = *e.m_pPropertyStates;

    bool bSetVelocity = e.m_pObject->GetTypeAccessor().GetValue("SetVelocity").ConvertTo<bool>();

    props["Speed"].m_Visibility = bSetVelocity ? plPropertyUiState::Default : plPropertyUiState::Invisible;
  }
}

void plParticleEffectAssetDocument::WriteResource(plStreamWriter& inout_stream) const
{
  const plParticleEffectDescriptor* pProp = GetProperties();

  pProp->Save(inout_stream);
}


void plParticleEffectAssetDocument::TriggerRestartEffect()
{
  plParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plParticleEffectAssetEvent::RestartEffect;

  m_Events.Broadcast(e);
}


void plParticleEffectAssetDocument::SetAutoRestart(bool bEnable)
{
  if (m_bAutoRestart == bEnable)
    return;

  m_bAutoRestart = bEnable;

  plParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plParticleEffectAssetEvent::AutoRestartChanged;

  m_Events.Broadcast(e);
}


void plParticleEffectAssetDocument::SetSimulationPaused(bool bPaused)
{
  if (m_bSimulationPaused == bPaused)
    return;

  m_bSimulationPaused = bPaused;

  plParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}

void plParticleEffectAssetDocument::SetSimulationSpeed(float fSpeed)
{
  if (m_fSimulationSpeed == fSpeed)
    return;

  m_fSimulationSpeed = fSpeed;

  plParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plParticleEffectAssetEvent::SimulationSpeedChanged;

  m_Events.Broadcast(e);
}


void plParticleEffectAssetDocument::SetRenderVisualizers(bool b)
{
  if (m_bRenderVisualizers == b)
    return;

  m_bRenderVisualizers = b;

  plVisualizerManager::GetSingleton()->SetVisualizersActive(this, m_bRenderVisualizers);

  plParticleEffectAssetEvent e;
  e.m_pDocument = this;
  e.m_Type = plParticleEffectAssetEvent::RenderVisualizersChanged;

  m_Events.Broadcast(e);
}

plResult plParticleEffectAssetDocument::ComputeObjectTransformation(const plDocumentObject* pObject, plTransform& out_result) const
{
  // currently the preview particle effect is always at the origin
  out_result.SetIdentity();
  return PL_SUCCESS;
}

void plParticleEffectAssetDocument::UpdateAssetDocumentInfo(plAssetDocumentInfo* pInfo) const
{
  SUPER::UpdateAssetDocumentInfo(pInfo);

  auto* desc = GetProperties();

  for (const auto& system : desc->GetParticleSystems())
  {
    for (const auto& type : system->GetTypeFactories())
    {
      if (auto* pType = plDynamicCast<plParticleTypeQuadFactory*>(type))
      {
        if (pType->m_RenderMode != plParticleTypeRenderMode::Distortion)
        {
          // remove unused dependencies
          pInfo->m_TransformDependencies.Remove(pType->m_sDistortionTexture);
        }
      }

      if (auto* pType = plDynamicCast<plParticleTypeTrailFactory*>(type))
      {
        if (pType->m_RenderMode != plParticleTypeRenderMode::Distortion)
        {
          // remove unused dependencies
          pInfo->m_TransformDependencies.Remove(pType->m_sDistortionTexture);
        }
      }
    }
  }

  // shared effects do not support parameters
  if (!desc->m_bAlwaysShared)
  {
    plExposedParameters* pExposedParams = PL_DEFAULT_NEW(plExposedParameters);
    for (auto it = desc->m_FloatParameters.GetIterator(); it.IsValid(); ++it)
    {
      plExposedParameter* param = PL_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
    }
    for (auto it = desc->m_ColorParameters.GetIterator(); it.IsValid(); ++it)
    {
      plExposedParameter* param = PL_DEFAULT_NEW(plExposedParameter);
      pExposedParams->m_Parameters.PushBack(param);
      param->m_sName = it.Key();
      param->m_DefaultValue = it.Value();
    }

    // Info takes ownership of meta data.
    pInfo->m_MetaInfo.PushBack(pExposedParams);
  }
}

plTransformStatus plParticleEffectAssetDocument::InternalTransformAsset(plStreamWriter& stream, plStringView sOutputTag,
  const plPlatformProfile* pAssetProfile, const plAssetFileHeader& AssetHeader, plBitflags<plTransformFlags> transformFlags)
{
  WriteResource(stream);
  return plStatus(PL_SUCCESS);
}

plTransformStatus plParticleEffectAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& ThumbnailInfo)
{
  plStatus status = plAssetDocument::RemoteCreateThumbnail(ThumbnailInfo);
  return status;
}
