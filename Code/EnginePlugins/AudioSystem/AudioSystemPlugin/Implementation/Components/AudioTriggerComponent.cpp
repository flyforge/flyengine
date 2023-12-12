#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioProxyComponent.h>
#include <AudioSystemPlugin/Components/AudioTriggerComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/Physics/SurfaceResource.h>
#include <Core/ResourceManager/Resource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>

constexpr plTypeVersion kVersion_AudioTriggerComponent = 1;

/// \brief The last used event ID for all audio trigger components.
static plAudioSystemDataID s_uiNextEventId = 2;

/// \brief A set of generated points distributed in a sphere. This is used
/// for casting rays during the obstruction/occlusion calculation.
static plVec3 s_InSpherePositions[k_MaxOcclusionRaysCount];

/// \brief Specifies if the s_InSpherePositions array has been initialized.
static bool s_bInSpherePositionsInitialized = false;

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
#  include <RendererCore/Debug/DebugRenderer.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>

extern plCVarBool cvar_AudioSystemDebug;
#endif

/// \brief The number of rays to send each time we should calculate obstruction and occlusion values.
/// \note This value is used only when the trigger's SoundObstructionType property is set to
/// MultipleRay.
plCVarInt cvar_AudioSystemOcclusionNumRays("Audio.Occlusion.NumRays", 2, plCVarFlags::Save, "Number of occlusion rays per triggers per frames.");

/// \brief The seed used when generating points in the s_InSpherePositions array.
plCVarInt cvar_AudioSystemOcclusionRaysSeed("Audio.Occlusion.Seed", 24, plCVarFlags::Save, "The seed used to generate directions for the trigger's rays.");

/// \brief The maximum distance in world units beyond which the sound Obstruction/Occlusion calculations are disabled.
plCVarFloat cvar_AudioSystemOcclusionMaxDistance("Audio.Occlusion.MaxDistance", 150.0f, plCVarFlags::Save, "The maximum distance in world units beyond which the sound obstruction/occlusion calculations are disabled.");

/// \brief The maximum distance after which the Obstruction value starts to decrease with distance.
plCVarFloat cvar_AudioSystemFullObstructionMaxDistance("Audio.Obstruction.MaxDistance", 5.0f, plCVarFlags::Save, "The maximum distance after which the obstruction value starts to decrease with distance.");

/// \brief The smooth factor to use when updating the occlusion/obstruction value to the new target over time.
plCVarFloat cvar_AudioSystemOcclusionSmoothFactor("Audio.Occlusion.SmoothFactor", 5.0f, plCVarFlags::Save, "How slowly the smoothing of obstruction/occlusion values should smooth to target.");

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAudioTriggerComponent, kVersion_AudioTriggerComponent, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PlayTrigger", m_sPlayTrigger),
    PLASMA_MEMBER_PROPERTY("StopTrigger", m_sStopTrigger),
    PLASMA_ENUM_MEMBER_PROPERTY("SoundObstructionType", plAudioSystemSoundObstructionType, m_eObstructionType),
    PLASMA_ACCESSOR_PROPERTY("OcclusionCollisionLayer", GetOcclusionCollisionLayer, SetOcclusionCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
    PLASMA_MEMBER_PROPERTY("LoadOnInit", m_bLoadOnInit),
    PLASMA_MEMBER_PROPERTY("PlayOnActivate", m_bPlayOnActivate),

    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsLoading", IsLoading)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsReady", IsReady)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsStarting", IsStarting)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsPlaying", IsPlaying)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsStopping", IsStopping)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsStopped", IsStopped)->AddAttributes(new plHiddenAttribute()),
    PLASMA_ACCESSOR_PROPERTY_READ_ONLY("IsUnloading", IsUnloading)->AddAttributes(new plHiddenAttribute()),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Play, In, "Sync"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Stop, In, "Sync"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetPlayTrigger, In, "PlayTrigger"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetStopTrigger, In, "StopTrigger"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetPlayTrigger),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetStopTrigger),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plAudioTriggerComponentManager::plAudioTriggerComponentManager(plWorld* pWorld)
  : plComponentManager(pWorld)
{
  if (!s_bInSpherePositionsInitialized)
  {
    s_bInSpherePositionsInitialized = true;

    plRandom rngPhi;
    rngPhi.Initialize(cvar_AudioSystemOcclusionRaysSeed);

    for (auto& pos : s_InSpherePositions)
    {
      pos = plVec3::CreateRandomPointInSphere(rngPhi);
      pos.SetLength(cvar_AudioSystemOcclusionMaxDistance).IgnoreResult();
    }
  }
}

void plAudioTriggerComponentManager::Initialize()
{
  SUPER::Initialize();

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plAudioTriggerComponentManager::ProcessOcclusion, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }

  {
    auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plAudioTriggerComponentManager::Update, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostTransform;
    desc.m_bOnlyUpdateWhenSimulating = true;

    this->RegisterUpdateFunction(desc);
  }
}

void plAudioTriggerComponentManager::Deinitialize()
{
  SUPER::Deinitialize();
}

float plAudioTriggerComponentManager::ObstructionOcclusionValue::GetValue() const
{
  return m_fValue;
}

void plAudioTriggerComponentManager::ObstructionOcclusionValue::SetTarget(const float fTarget, const bool bReset)
{
  if (bReset)
  {
    Reset(fTarget);
  }
  else if (plMath::Abs(fTarget - m_fTarget) > plMath::HugeEpsilon<float>())
  {
    m_fTarget = fTarget;
  }
}

void plAudioTriggerComponentManager::ObstructionOcclusionValue::Update(const float fSmoothFactor)
{
  if (plMath::Abs(m_fTarget - m_fValue) > plMath::HugeEpsilon<float>())
  {
    // Move to the target
    const float smoothFactor = (fSmoothFactor < 0.f) ? cvar_AudioSystemOcclusionSmoothFactor : fSmoothFactor;
    m_fValue += (m_fTarget - m_fValue) / (smoothFactor * smoothFactor + 1.f);
  }
  else
  {
    // Target reached
    m_fValue = m_fTarget;
  }
}

void plAudioTriggerComponentManager::ObstructionOcclusionValue::Reset(const float fInitialValue)
{
  m_fTarget = m_fValue = fInitialValue;
}

plUInt32 plAudioTriggerComponentManager::AddObstructionOcclusionState(plAudioTriggerComponent* pComponent)
{
  auto& occlusionState = m_ObstructionOcclusionStates.ExpandAndGetRef();
  occlusionState.m_pComponent = pComponent;

  const plUInt32 uiNumRays = plMath::Max<int>(cvar_AudioSystemOcclusionNumRays, 1);

  if (const auto* pPhysicsWorldModule = GetWorld()->GetModule<plPhysicsWorldModuleInterface>())
  {
    if (const auto* listenerManager = GetWorld()->GetComponentManager<plAudioListenerComponentManager>())
    {
      for (auto it = listenerManager->GetComponents(); it.IsValid(); ++it)
      {
        if (const plAudioListenerComponent* component = it; component->IsDefault())
        {
          if (const plVec3 listenerPos = component->GetListenerPosition(); !listenerPos.IsNaN())
          {
            ShootOcclusionRays(occlusionState, listenerPos, uiNumRays, pPhysicsWorldModule);
            break;
          }
        }
      }
    }
  }

  return m_ObstructionOcclusionStates.GetCount() - 1;
}

void plAudioTriggerComponentManager::RemoveObstructionOcclusionState(plUInt32 uiIndex)
{
  if (uiIndex >= m_ObstructionOcclusionStates.GetCount())
    return;

  m_ObstructionOcclusionStates.RemoveAtAndSwap(uiIndex);

  if (uiIndex != m_ObstructionOcclusionStates.GetCount())
  {
    m_ObstructionOcclusionStates[uiIndex].m_pComponent->m_uiObstructionOcclusionStateIndex = uiIndex;
  }
}

void plAudioTriggerComponentManager::ShootOcclusionRays(ObstructionOcclusionState& state, plVec3 listenerPos, plUInt32 uiNumRays, const plPhysicsWorldModuleInterface* pPhysicsWorldModule)
{
  if (state.m_pComponent->m_eObstructionType == plAudioSystemSoundObstructionType::None)
  {
    state.m_ObstructionValue.Reset();
    state.m_OcclusionValue.Reset();
    return;
  }

  const plVec3 sourcePos = state.m_pComponent->GetOwner()->GetGlobalPosition();

  // When the source position is invalid for unknown and weird reasons
  if (sourcePos.IsNaN())
    return;

  const plVec3 directRay = listenerPos - sourcePos;
  const float fDirectRayLength = directRay.GetLength();

  // If the distance between the source and the listener is greater than the maximum allowed distance
  if (fDirectRayLength >= cvar_AudioSystemOcclusionMaxDistance.GetValue())
    return;

  const plUInt8 uiCollisionLayer = state.m_pComponent->m_uiOcclusionCollisionLayer;

  // Cast direct (obstruction) ray
  CastRay(state, sourcePos, directRay, uiCollisionLayer, pPhysicsWorldModule, 0);
  state.m_ObstructionValue.SetTarget(state.m_ObstructionRaysValues[0]);

  // When multiple rays, compute both obstruction and occlusion
  if (state.m_pComponent->m_eObstructionType == plAudioSystemSoundObstructionType::MultipleRay)
  {
    float averageOcclusion = 0.0f;

    // Cast indirect (occlusion) rays
    for (plUInt32 i = 1; i < uiNumRays; ++i)
    {
      const plUInt32 uiRayIndex = state.m_uiNextRayIndex;

      CastRay(state, sourcePos, s_InSpherePositions[uiRayIndex], uiCollisionLayer, pPhysicsWorldModule, i);
      averageOcclusion += state.m_ObstructionRaysValues[i];

      state.m_uiNextRayIndex = (state.m_uiNextRayIndex + 1) % k_MaxOcclusionRaysCount;
    }

    averageOcclusion /= static_cast<float>(uiNumRays - 1);
    state.m_OcclusionValue.SetTarget(averageOcclusion);

    // Obstruction should be taken into account if the average value of indirect rays is different than the value of the direct ray in the same ray cast,
    // in the other case, the computed obstruction value become the occlusion and obstruction is set to 0
    if (plMath::Abs(averageOcclusion - state.m_ObstructionRaysValues[0]) <= plMath::HugeEpsilon<float>())
    {
      state.m_ObstructionValue.Reset();
      state.m_OcclusionValue.SetTarget(state.m_ObstructionRaysValues[0]);
    }
  }
  // Otherwise take the obstruction as the occlusion and set the obstruction to 0
  else
  {
    state.m_ObstructionValue.Reset();
    state.m_OcclusionValue.SetTarget(state.m_ObstructionRaysValues[0]);
  }

  state.m_ObstructionValue.Update();
  state.m_OcclusionValue.Update();

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
  if (cvar_AudioSystemDebug)
  {
    if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
    {
      plDebugRenderer::Draw3DText(pView->GetHandle(), plFmt("Occlusion: {0}\nObstruction: {1}", state.m_OcclusionValue.GetValue(), state.m_ObstructionValue.GetValue()), sourcePos, plColor::White);
    }
  }
#endif
}

void plAudioTriggerComponentManager::CastRay(ObstructionOcclusionState& state, plVec3 sourcePos, plVec3 direction, plUInt8 collisionLayer, const plPhysicsWorldModuleInterface* pPhysicsWorldModule, plUInt32 rayIndex)
{
  float averageObstruction = 0.0f;

  if (!direction.IsZero())
  {
    const float fDistance = direction.GetLengthAndNormalize();

    plPhysicsQueryParameters query(collisionLayer);
    query.m_bIgnoreInitialOverlap = true;
    query.m_ShapeTypes = plPhysicsShapeType::Static | plPhysicsShapeType::Dynamic | plPhysicsShapeType::Query;

    plPhysicsCastResultArray results;

    if (pPhysicsWorldModule->RaycastAll(results, sourcePos, direction, fDistance, query))
    {
      const float fMaxDistance = cvar_AudioSystemOcclusionMaxDistance.GetValue();

      for (const auto& hitResult : results.m_Results)
      {
        if (!hitResult.m_hSurface.IsValid())
          continue;

        plResourceLock hitSurface(hitResult.m_hSurface, plResourceAcquireMode::PointerOnly);
        if (hitSurface.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
          continue;

        float obstructionContribution = hitSurface->GetDescriptor().m_fSoundObstruction;

        if (hitResult.m_fDistance > cvar_AudioSystemFullObstructionMaxDistance)
        {
          const float fClampedDistance = plMath::Clamp(hitResult.m_fDistance, 0.0f, fMaxDistance);
          const float fDistanceScale = 1.0f - (fClampedDistance / fMaxDistance);

          obstructionContribution *= fDistanceScale;
        }

        averageObstruction += obstructionContribution;
      }

      averageObstruction /= static_cast<float>(results.m_Results.GetCount());
    }

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
    if (cvar_AudioSystemDebug)
    {
      if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
      {
        plDebugRenderer::Line ray[1] = {{sourcePos, sourcePos + direction * fDistance}};
        plDebugRenderer::DrawLines(pView->GetHandle(), plMakeArrayPtr(ray), averageObstruction == 0.0f ? plColor::Red : plColor::Green);
      }
    }
#endif
  }

  if (state.m_ObstructionRaysValues.GetCount() <= rayIndex)
  {
    state.m_ObstructionRaysValues.Insert(averageObstruction, rayIndex);
  }
  else
  {
    state.m_ObstructionRaysValues[rayIndex] = averageObstruction;
  }
}

void plAudioTriggerComponentManager::ProcessOcclusion(const plWorldModule::UpdateContext& context)
{
  if (const auto pPhysicsWorldModule = GetWorld()->GetModuleReadOnly<plPhysicsWorldModuleInterface>())
  {
    const plUInt32 uiNumRays = plMath::Max<int>(cvar_AudioSystemOcclusionNumRays, 1);

    for (auto& occlusionState : m_ObstructionOcclusionStates)
    {
      if (const auto* audioWorldModule = GetWorld()->GetModuleReadOnly<plAudioWorldModule>())
      {
        if (const plAudioListenerComponent* listener = audioWorldModule->GetDefaultListener())
        {
          if (const plVec3 listenerPos = listener->GetListenerPosition(); !listenerPos.IsNaN())
          {
            ShootOcclusionRays(occlusionState, listenerPos, uiNumRays, pPhysicsWorldModule);
            break;
          }
        }
      }
    }
  }
}

void plAudioTriggerComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (ComponentType* pComponent = it; pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

plAudioTriggerComponent::plAudioTriggerComponent()
  : plAudioSystemProxyDependentComponent()
  , m_eState(plAudioSystemTriggerState::Invalid)
  , m_uiPlayEventId(s_uiNextEventId++)
  , m_uiStopEventId(s_uiNextEventId++)
  , m_eObstructionType(plAudioSystemSoundObstructionType::SingleRay)
  , m_uiOcclusionCollisionLayer(0)
  , m_uiObstructionOcclusionStateIndex(plInvalidIndex)
  , m_bLoadOnInit(false)
  , m_bPlayOnActivate(false)
{
}

plAudioTriggerComponent::~plAudioTriggerComponent() = default;

void plAudioTriggerComponent::SetOcclusionCollisionLayer(plUInt8 uiCollisionLayer)
{
  m_uiOcclusionCollisionLayer = uiCollisionLayer;
}

void plAudioTriggerComponent::SetPlayTrigger(plString sName)
{
  if (sName == m_sPlayTrigger)
    return;

  if (IsPlaying())
  {
    Stop();
  }

  m_sPlayTrigger = std::move(sName);
}

const plString& plAudioTriggerComponent::GetPlayTrigger() const
{
  return m_sPlayTrigger;
}

void plAudioTriggerComponent::SetStopTrigger(plString sName)
{
  if (sName == m_sStopTrigger)
    return;

  m_sStopTrigger = std::move(sName);
}

const plString& plAudioTriggerComponent::GetStopTrigger() const
{
  return m_sStopTrigger;
}

void plAudioTriggerComponent::Play(bool bSync)
{
  if (!m_bCanPlay || m_sPlayTrigger.IsEmpty() || IsPlaying() || IsStarting())
    return;

  if (!m_bPlayTriggerLoaded)
    LoadPlayTrigger(true); // Need to be sync if data was not loaded before

  m_eState = plAudioSystemTriggerState::Starting;

  plAudioSystemRequestActivateTrigger request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
  request.m_uiEventId = m_uiPlayEventId;

  request.m_Callback = [this](const plAudioSystemRequestActivateTrigger& e)
  {
    if (e.m_eStatus.Succeeded())
      m_eState = plAudioSystemTriggerState::Playing;
    else
      m_eState = plAudioSystemTriggerState::Invalid;
  };

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }
}

void plAudioTriggerComponent::Stop(bool bSync)
{
  StopInternal(bSync, false);
}

const plEnum<plAudioSystemTriggerState>& plAudioTriggerComponent::GetState() const
{
  return m_eState;
}

bool plAudioTriggerComponent::IsLoading() const
{
  return m_eState == plAudioSystemTriggerState::Loading;
}

bool plAudioTriggerComponent::IsReady() const
{
  return m_eState == plAudioSystemTriggerState::Ready;
}

bool plAudioTriggerComponent::IsStarting() const
{
  return m_eState == plAudioSystemTriggerState::Starting;
}

bool plAudioTriggerComponent::IsPlaying() const
{
  return m_eState == plAudioSystemTriggerState::Playing;
}

bool plAudioTriggerComponent::IsStopping() const
{
  return m_eState == plAudioSystemTriggerState::Stopping;
}

bool plAudioTriggerComponent::IsStopped() const
{
  return m_eState == plAudioSystemTriggerState::Stopped;
}

bool plAudioTriggerComponent::IsUnloading() const
{
  return m_eState == plAudioSystemTriggerState::Unloading;
}

void plAudioTriggerComponent::Initialize()
{
  SUPER::Initialize();

  if (m_bLoadOnInit)
  {
    LoadPlayTrigger(false);
    LoadStopTrigger(false, false);
  }
}

void plAudioTriggerComponent::OnActivated()
{
  SUPER::OnActivated();

  if (m_bCanPlay && m_bPlayOnActivate && !m_bHasPlayedOnActivate)
  {
    Play();
    m_bHasPlayedOnActivate = true;
  }
}

void plAudioTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_bCanPlay = true;

  if (m_bCanPlay && m_bPlayOnActivate && !m_bHasPlayedOnActivate)
  {
    Play();
    m_bHasPlayedOnActivate = true;
  }
}

void plAudioTriggerComponent::OnDeactivated()
{
  plStaticCast<plAudioTriggerComponentManager*>(GetOwningManager())->RemoveObstructionOcclusionState(m_uiObstructionOcclusionStateIndex);
  m_uiObstructionOcclusionStateIndex = plInvalidIndex;

  if (IsPlaying())
    StopInternal(false, true);

  m_bHasPlayedOnActivate = false;
  SUPER::OnDeactivated();
}

void plAudioTriggerComponent::Deinitialize()
{
  if (m_bStopTriggerLoaded)
    UnloadStopTrigger(false, true);

  if (m_bPlayTriggerLoaded)
    UnloadPlayTrigger(false, true);

  SUPER::Deinitialize();
}

void plAudioTriggerComponent::LoadPlayTrigger(bool bSync)
{
  if (m_sPlayTrigger.IsEmpty())
    return;

  if (m_bPlayTriggerLoaded)
  {
    m_eState = plAudioSystemTriggerState::Ready;
    return;
  }

  m_eState = plAudioSystemTriggerState::Loading;

  plAudioSystemRequestLoadTrigger request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
  request.m_uiEventId = m_uiPlayEventId;

  request.m_Callback = [this](const plAudioSystemRequestLoadTrigger& m)
  {
    if (m.m_eStatus.Failed())
    {
      m_eState = plAudioSystemTriggerState::Invalid;
      return;
    }

    m_bPlayTriggerLoaded = true;
    m_eState = plAudioSystemTriggerState::Ready;
  };

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }
}

void plAudioTriggerComponent::LoadStopTrigger(bool bSync, bool bDeinit)
{
  if (m_sStopTrigger.IsEmpty())
    return;

  if (m_bStopTriggerLoaded)
    return;

  plAudioSystemRequestLoadTrigger request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);
  request.m_uiEventId = m_uiStopEventId;

  if (!bDeinit)
  {
    request.m_Callback = [this](const plAudioSystemRequestLoadTrigger& m)
    {
      if (m.m_eStatus.Failed())
        return;

      m_bStopTriggerLoaded = true;
    };
  }

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }

  if (bDeinit)
  {
    m_bStopTriggerLoaded = true;
  }
}

void plAudioTriggerComponent::UnloadPlayTrigger(bool bSync, bool bDeinit)
{
  if (!m_bPlayTriggerLoaded)
    return;

  m_eState = plAudioSystemTriggerState::Unloading;

  plAudioSystemRequestUnloadTrigger request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);

  if (!bDeinit)
  {
    request.m_Callback = [this](const plAudioSystemRequestUnloadTrigger& m)
    {
      if (m.m_eStatus.Failed())
      {
        m_eState = plAudioSystemTriggerState::Invalid;
        return;
      }

      m_bPlayTriggerLoaded = false;
      m_eState = plAudioSystemTriggerState::Invalid;
    };
  }

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }
}

void plAudioTriggerComponent::UnloadStopTrigger(bool bSync, bool bDeinit)
{
  if (!m_bStopTriggerLoaded)
    return;

  plAudioSystemRequestUnloadTrigger request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);

  if (!bDeinit)
  {
    request.m_Callback = [this](const plAudioSystemRequestUnloadTrigger& m)
    {
      if (m.m_eStatus.Failed())
        return;

      m_bStopTriggerLoaded = false;
    };
  }

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(request);
  }
}

void plAudioTriggerComponent::UpdateOcclusion()
{
  if (m_pProxyComponent == nullptr)
    return;

  if (m_eObstructionType == plAudioSystemSoundObstructionType::None)
    return;

  auto* pComponentManager = plStaticCast<plAudioTriggerComponentManager*>(GetOwningManager());

  if (m_uiObstructionOcclusionStateIndex == plInvalidIndex)
    m_uiObstructionOcclusionStateIndex = pComponentManager->AddObstructionOcclusionState(this);

  const auto& occlusionState = pComponentManager->GetObstructionOcclusionState(m_uiObstructionOcclusionStateIndex);

  plAudioSystemRequestSetObstructionOcclusion request;

  request.m_uiEntityId = m_pProxyComponent->GetEntityId();
  request.m_fObstruction = occlusionState.m_ObstructionValue.GetValue();
  request.m_fOcclusion = occlusionState.m_OcclusionValue.GetValue();

  plAudioSystem::GetSingleton()->SendRequest(request);
}

void plAudioTriggerComponent::Update()
{
  if (IsPlaying())
  {
    UpdateOcclusion();
  }
}

void plAudioTriggerComponent::StopInternal(bool bSync, bool bDeinit)
{
  m_eState = plAudioSystemTriggerState::Stopping;

  if (m_sStopTrigger.IsEmpty())
  {
    plAudioSystemRequestStopEvent request;

    request.m_uiEntityId = GetEntityId();
    request.m_uiTriggerId = plAudioSystem::GetSingleton()->GetTriggerId(m_sPlayTrigger);
    request.m_uiObjectId = m_uiPlayEventId;

    // In case of deinitialization, we don't need to run the callback
    if (!bDeinit)
    {
      request.m_Callback = [this](const plAudioSystemRequestStopEvent& e)
      {
        if (e.m_eStatus.Succeeded())
          m_eState = plAudioSystemTriggerState::Stopped;
        else
          m_eState = plAudioSystemTriggerState::Invalid;
      };
    }

    if (bSync)
    {
      plAudioSystem::GetSingleton()->SendRequestSync(request);
    }
    else
    {
      plAudioSystem::GetSingleton()->SendRequest(request);
    }
  }
  else
  {
    if (!m_bStopTriggerLoaded)
      LoadStopTrigger(true, bDeinit); // Need to be sync if data was not loaded before

    plAudioSystemRequestActivateTrigger request;

    request.m_uiEntityId = GetEntityId();
    request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sStopTrigger);
    request.m_uiEventId = m_uiStopEventId;

    // In case of deinitialization, we don't need to run the callback
    if (!bDeinit)
    {
      request.m_Callback = [this](const plAudioSystemRequestActivateTrigger& e)
      {
        if (e.m_eStatus.Succeeded())
          m_eState = plAudioSystemTriggerState::Stopped;
        else
          m_eState = plAudioSystemTriggerState::Invalid;
      };
    }

    if (bSync)
    {
      plAudioSystem::GetSingleton()->SendRequestSync(request);
    }
    else
    {
      plAudioSystem::GetSingleton()->SendRequest(request);
    }
  }

  if (bDeinit)
  {
    m_eState = plAudioSystemTriggerState::Stopped;
  }
}

void plAudioTriggerComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioTriggerComponent);

  s << m_sPlayTrigger;
  s << m_sStopTrigger;
  s << m_eObstructionType;
  s << m_bLoadOnInit;
  s << m_bPlayOnActivate;
  s << m_uiOcclusionCollisionLayer;
}

void plAudioTriggerComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioTriggerComponent);

  s >> m_sPlayTrigger;
  s >> m_sStopTrigger;
  s >> m_eObstructionType;
  s >> m_bLoadOnInit;
  s >> m_bPlayOnActivate;
  s >> m_uiOcclusionCollisionLayer;
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioTriggerComponent);
