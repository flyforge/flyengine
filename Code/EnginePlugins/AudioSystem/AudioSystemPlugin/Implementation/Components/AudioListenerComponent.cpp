#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioListenerComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>
#include <AudioSystemPlugin/Core/AudioWorldModule.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioListenerComponent = 1;

static plUInt32 s_uiNextListenerId = 2; // 1 is reserved for the default listener

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAudioListenerComponent, kVersion_AudioListenerComponent, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("PositionGameObject", _DoNotCall, SetListenerPositionObject)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("RotationGameObject", _DoNotCall, SetListenerOrientationObject)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_ACCESSOR_PROPERTY("IsDefault", IsDefault, SetDefault)->AddAttributes(new plDefaultValueAttribute(false)),
    PLASMA_MEMBER_PROPERTY("PositionOffset", m_vListenerPositionOffset),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

void plAudioListenerComponent::OnActivated()
{
  SUPER::OnActivated();

  if (auto* const audioWorldModule = GetWorld()->GetOrCreateModule<plAudioWorldModule>(); audioWorldModule->GetDefaultListener() != this && m_bIsDefault)
  {
    audioWorldModule->SetDefaultListener(this);
  }

  plAudioSystemRequestRegisterListener request;

  request.m_uiListenerId = m_uiListenerId;
  request.m_sName = GetOwner()->GetName();

  // Prefer to send this request synchronously...
  plAudioSystem::GetSingleton()->SendRequestSync(request);
}

void plAudioListenerComponent::OnDeactivated()
{
  plAudioSystemRequestUnregisterListener request;

  request.m_uiListenerId = m_uiListenerId;

  // Prefer to send this request synchronously...
  plAudioSystem::GetSingleton()->SendRequestSync(request);

  if (auto* const audioWorldModule = GetWorld()->GetOrCreateModule<plAudioWorldModule>(); audioWorldModule->GetDefaultListener() == this)
  {
    audioWorldModule->SetDefaultListener(nullptr);
  }

  SUPER::OnDeactivated();
}

void plAudioListenerComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioListenerComponent);

  stream.WriteGameObjectHandle(m_hListenerPositionObject);
  stream.WriteGameObjectHandle(m_hListenerRotationObject);
  s << m_vListenerPositionOffset;
}

void plAudioListenerComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioListenerComponent);

  m_hListenerPositionObject = stream.ReadGameObjectHandle();
  m_hListenerRotationObject = stream.ReadGameObjectHandle();
  s >> m_vListenerPositionOffset;
}

plAudioListenerComponent::plAudioListenerComponent()
  : plAudioSystemComponent()
  , m_uiListenerId(s_uiNextListenerId++)
  , m_bIsDefault(false)
{
}

plAudioListenerComponent::~plAudioListenerComponent() = default;

void plAudioListenerComponent::SetListenerPositionObject(const char* szGuid)
{
  const auto& resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hListenerPositionObject = resolver(szGuid, GetHandle(), "PositionGameObject");
}

void plAudioListenerComponent::SetListenerOrientationObject(const char* szGuid)
{
  const auto& resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  m_hListenerRotationObject = resolver(szGuid, GetHandle(), "RotationGameObject");
}

void plAudioListenerComponent::SetDefault(bool bDefault)
{
  m_bIsDefault = bDefault;

  auto* const audioWorldModule = GetWorld()->GetOrCreateModule<plAudioWorldModule>(); 

  if (audioWorldModule->GetDefaultListener() != this && m_bIsDefault)
  {
    audioWorldModule->SetDefaultListener(this);
  }

  if (audioWorldModule->GetDefaultListener() == this && !m_bIsDefault)
  {
    audioWorldModule->SetDefaultListener(nullptr);
  }
}

plVec3 plAudioListenerComponent::GetListenerPosition() const
{
  plVec3 position;

  if (const plGameObject* pObject = nullptr; GetWorld()->TryGetObject(m_hListenerPositionObject, pObject))
  {
    position = pObject->GetGlobalPosition();
  }
  else
  {
    position = GetOwner()->GetGlobalPosition();
  }

  return position + m_vListenerPositionOffset;
}

plVec3 plAudioListenerComponent::GetListenerVelocity() const
{
  if (const plGameObject* pObject = nullptr; GetWorld()->TryGetObject(m_hListenerPositionObject, pObject))
  {
    return pObject->GetLinearVelocity();
  }

  return GetOwner()->GetLinearVelocity();
}

plQuat plAudioListenerComponent::GetListenerRotation() const
{
  if (const plGameObject* pObject = nullptr; GetWorld()->TryGetObject(m_hListenerRotationObject, pObject))
  {
    return pObject->GetGlobalRotation();
  }

  return GetOwner()->GetGlobalRotation();
}

bool plAudioListenerComponent::IsDefault() const
{
  return m_bIsDefault;
}

void plAudioListenerComponent::Update()
{
  const auto& position = GetListenerPosition();
  const auto& rotation = GetListenerRotation();
  const auto& velocity = GetListenerVelocity();

  const auto& fw = (rotation * plVec3::MakeAxisX()).GetNormalized();
  const auto& up = (rotation * plVec3::MakeAxisZ()).GetNormalized();

  {
    plAudioSystemTransform transform;
    transform.m_vForward = fw;
    transform.m_vPosition = position;
    transform.m_vUp = up;
    transform.m_vVelocity = velocity;

    if (m_LastTransform == transform)
      return;
  }

  plAudioSystem::GetSingleton()->SetListener(m_uiListenerId, position, fw, up, velocity);
}

const char* plAudioListenerComponent::_DoNotCall() const
{
  return nullptr;
}

PLASMA_STATICLINK_FILE(AudioSystem, AudioSystemPlugin_Implementation_Components_AudioListenerComponent);
