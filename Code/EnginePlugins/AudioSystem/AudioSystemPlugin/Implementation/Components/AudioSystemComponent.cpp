#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioProxyComponent.h>
#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioWorldModule.h>

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plAudioSystemComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Audio"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_ABSTRACT_COMPONENT_TYPE;

PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plAudioSystemProxyDependentComponent, 1)
PL_END_ABSTRACT_COMPONENT_TYPE;

PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plAudioSystemEnvironmentComponent, 1)
{
  PL_BEGIN_ATTRIBUTES
  {
    new plSphereManipulatorAttribute("MaxDistance"),
  }
  PL_END_ATTRIBUTES;

  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Environment", m_sEnvironmentName),
    PL_ACCESSOR_PROPERTY("MaxDistance", GetMaxDistance, SetMaxDistance)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.01f, plVariant()), new plSuffixAttribute(" m")),
    PL_MEMBER_PROPERTY("Color", m_ShapeColor),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgAudioSystemSetEnvironmentAmount, OnSetAmount),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

void plAudioSystemProxyDependentComponent::Initialize()
{
  SUPER::Initialize();

  if (m_pProxyComponent == nullptr)
  {
    GetOwner()->TryGetComponentOfBaseType(m_pProxyComponent);
    if (m_pProxyComponent == nullptr)
    {
      GetOwner()->GetWorld()->GetOrCreateComponentManager<plAudioProxyComponentManager>()->CreateComponent(GetOwner(), m_pProxyComponent);
      if (m_pProxyComponent == nullptr)
      {
        plLog::Error("Unable to create an Audio Proxy component on GameObject {0}", GetOwner()->GetName());
      }
    }

    if (m_pProxyComponent != nullptr)
    {
      m_pProxyComponent->AddRef();
    }
  }

  if (m_pProxyComponent != nullptr)
  {
    m_pProxyComponent->EnsureInitialized();
  }
}

void plAudioSystemProxyDependentComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (m_pProxyComponent != nullptr)
  {
    m_pProxyComponent->EnsureSimulationStarted();
  }
}

void plAudioSystemProxyDependentComponent::Deinitialize()
{
  if (m_pProxyComponent != nullptr)
  {
    m_pProxyComponent->ReleaseRef();

    if (!m_pProxyComponent->IsReferenced())
    {
      m_pProxyComponent->Unregister(true);
      GetOwner()->GetWorld()->GetOrCreateComponentManager<plAudioProxyComponentManager>()->DeleteComponent(m_pProxyComponent);
    }

    m_pProxyComponent = nullptr;
  }

  SUPER::Deinitialize();
}

plAudioSystemDataID plAudioSystemProxyDependentComponent::GetEntityId() const
{
  if (m_pProxyComponent == nullptr)
    return kInvalidAudioSystemId;

  return m_pProxyComponent->GetEntityId();
}

plAudioSystemEnvironmentComponent::plAudioSystemEnvironmentComponent()
  : plAudioSystemProxyDependentComponent()
  , m_fMaxDistance(1)
{
}

void plAudioSystemEnvironmentComponent::OnActivated()
{
  SUPER::OnActivated();

  GetWorld()->GetOrCreateModule<plAudioWorldModule>()->AddEnvironment(this);
}

void plAudioSystemEnvironmentComponent::OnDeactivated()
{
  GetWorld()->GetOrCreateModule<plAudioWorldModule>()->RemoveEnvironment(this);

  SUPER::OnDeactivated();
}

float plAudioSystemEnvironmentComponent::GetMaxDistance() const
{
  return m_fMaxDistance;
}

plAudioSystemDataID plAudioSystemEnvironmentComponent::GetEnvironmentId() const
{
  return plAudioSystem::GetSingleton()->GetEnvironmentId(m_sEnvironmentName);
}

void plAudioSystemEnvironmentComponent::SetMaxDistance(float fFadeDistance)
{
  m_fMaxDistance = fFadeDistance;
}

void plAudioSystemEnvironmentComponent::OverrideEnvironmentAmount(float fValue)
{
  m_bOverrideValue = fValue >= 0;
  m_fOverrideValue = m_bOverrideValue ? fValue : m_fOverrideValue;
}

void plAudioSystemEnvironmentComponent::OnSetAmount(plMsgAudioSystemSetEnvironmentAmount& msg)
{
  OverrideEnvironmentAmount(msg.m_fAmount);

  if (!m_bOverrideValue)
    return;

  plAudioSystemRequestSetEnvironmentAmount request;

  request.m_uiEntityId = GetEntityId();
  request.m_uiObjectId = GetEnvironmentId();
  request.m_fAmount = m_fOverrideValue;

  if (msg.m_bSync)
    plAudioSystem::GetSingleton()->SendRequestSync(request);
  else
    plAudioSystem::GetSingleton()->SendRequest(request);
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioSystemComponent);
