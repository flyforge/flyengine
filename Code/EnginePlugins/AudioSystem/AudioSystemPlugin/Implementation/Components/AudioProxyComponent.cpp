#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioProxyComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>
#include <AudioSystemPlugin/Core/AudioWorldModule.h>

static plAudioSystemDataID s_uiNextEntityId = 2; // 1 is reserved for the global entity.

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAudioProxyComponent, 1, plComponentMode::Static)
PLASMA_END_COMPONENT_TYPE;
// clang-format on

void plAudioProxyComponent::Initialize()
{
  SUPER::Initialize();

  plAudioSystemRequestRegisterEntity request;

  request.m_uiEntityId = m_uiEntityId;
  request.m_sName = GetOwner()->GetName();

  plAudioSystem::GetSingleton()->SendRequestSync(request);

  plLog::Info("Audio Proxy Component Initialized ({0})", m_uiEntityId);
}

void plAudioProxyComponent::Deinitialize()
{
  if (IsReferenced())
    return; // Some components are still depending on this proxy.

  Unregister(true);

  SUPER::Deinitialize();
}

plAudioProxyComponent::plAudioProxyComponent()
  : m_uiEntityId(s_uiNextEntityId++)
{
}

plAudioProxyComponent::~plAudioProxyComponent() = default;

plAudioSystemDataID plAudioProxyComponent::GetEntityId() const
{
  return m_uiEntityId;
}

void plAudioProxyComponent::Unregister(bool bForce) const
{
  if (!bForce && IsReferenced())
    return; // Some components are still depending on this proxy.

  plAudioSystemRequestUnregisterEntity request;

  request.m_uiEntityId = m_uiEntityId;

  plAudioSystem::GetSingleton()->SendRequest(request);
}

void plAudioProxyComponent::Update()
{
  plAudioSystemRequestsQueue rq;

  // Position update
  {
    const auto& rotation = GetOwner()->GetGlobalRotation();

    plAudioSystemTransform transform;
    transform.m_vPosition = GetOwner()->GetGlobalPosition();
    transform.m_vForward = (rotation * plVec3::UnitXAxis()).GetNormalized();
    transform.m_vUp = (rotation * plVec3::UnitZAxis()).GetNormalized();
    transform.m_vVelocity = GetOwner()->GetLinearVelocity();

    if (transform == m_LastTransform)
      return;

    plAudioSystemRequestSetEntityTransform request;

    request.m_uiEntityId = m_uiEntityId;
    request.m_Transform = transform;

    request.m_Callback = [this](const plAudioSystemRequestSetEntityTransform& m)
    {
      if (m.m_eStatus.Failed())
        return;

      m_LastTransform = m.m_Transform;
    };

    rq.PushBack(request);
  }

  // Collect environments amounts
  {
    const auto* pAudioWorldModule = GetWorld()->GetOrCreateModule<plAudioWorldModule>();
    if (pAudioWorldModule == nullptr)
      return;

    for (auto it = pAudioWorldModule->GetEnvironments(); it.IsValid(); ++it)
    {
      const auto& pComponent = it.Key();
      if (!pComponent->IsActiveAndInitialized())
        continue;

      const plAudioSystemDataID id = pComponent->GetEnvironmentId();
      if (id == kInvalidAudioSystemId)
        continue;

      m_mEnvironmentAmounts[id].m_fNextAmount = pComponent->GetEnvironmentAmount(this);

      if (plMath::IsZero(m_mEnvironmentAmounts[id].m_fNextAmount - m_mEnvironmentAmounts[id].m_fPreviousAmount, plMath::DefaultEpsilon<float>()))
        continue;

      plAudioSystemRequestSetEnvironmentAmount request;

      request.m_uiEntityId = m_uiEntityId;
      request.m_uiObjectId = id;
      request.m_fAmount = m_mEnvironmentAmounts[id].m_fNextAmount;

      rq.PushBack(request);
    }
  }

  // Send requests
  {
    plAudioSystem::GetSingleton()->SendRequests(rq);
  }
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioProxyComponent);
