#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Components/AudioAnimationComponent.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/Messages/CommonMessages.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/Skeleton.h>

constexpr plTypeVersion kVersion_AudioAnimationComponent = 1;
constexpr plTypeVersion kVersion_AudioAnimationEntry = 1;

/// \brief The last used event ID for all audio triggers.
extern plAudioSystemDataID s_uiNextEventId;
extern plAudioSystemDataID s_uiNextEntityId;

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioAnimationEntry, kVersion_AudioAnimationEntry, plRTTIDefaultAllocator<plAudioAnimationEntry>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("Event", m_sEventName),
    PLASMA_MEMBER_PROPERTY("Trigger", m_sTriggerName),
    PLASMA_ACCESSOR_PROPERTY("Joint", GetJointName, SetJointName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;

PLASMA_BEGIN_COMPONENT_TYPE(plAudioAnimationComponent, kVersion_AudioAnimationComponent, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Sound"),
    new plColorAttribute(plColorScheme::Sound),
  }
  PLASMA_END_ATTRIBUTES;

  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ARRAY_MEMBER_PROPERTY("Events", m_EventEntries),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
    PLASMA_MESSAGE_HANDLER(plMsgGenericEvent, OnAnimationEvent),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plAudioAnimationEntry::plAudioAnimationEntry()
  : m_uiEntityId(s_uiNextEntityId++)
  , m_uiEventId(s_uiNextEventId++)
  , m_uiJointIndex(plInvalidJointIndex)
  , m_bTriggerLoaded(false)
{
}

void plAudioAnimationEntry::SetJointName(const char* szName)
{
  m_sJointName.Assign(szName);
  m_uiJointIndex = plInvalidJointIndex;
}

const char* plAudioAnimationEntry::GetJointName() const
{
  return m_sJointName.GetData();
}

void plAudioAnimationEntry::ActivateTrigger() const
{
  plAudioSystemRequestActivateTrigger request;

  request.m_uiEntityId = m_uiEntityId;
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sTriggerName);
  request.m_uiEventId = m_uiEventId;

  plAudioSystem::GetSingleton()->SendRequest(request);
}

void plAudioAnimationEntry::Initialize(bool bSync)
{
  if (m_bTriggerLoaded)
    return;

  if (m_sTriggerName.IsEmpty())
    return;

  plAudioSystemRequestRegisterEntity registerEntity;

  {
    plStringBuilder name;
    name.Format("AudioAnimation Entity: {}", m_sEventName);

    registerEntity.m_uiEntityId = m_uiEntityId;
    registerEntity.m_sName = name;
  }

  plAudioSystemRequestLoadTrigger loadTrigger;

  loadTrigger.m_uiEntityId = m_uiEntityId;
  loadTrigger.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sTriggerName);
  loadTrigger.m_uiEventId = m_uiEventId;

  loadTrigger.m_Callback = [this](const plAudioSystemRequestLoadTrigger& m)
  {
    if (m.m_eStatus.Failed())
    {
      return;
    }

    m_bTriggerLoaded = true;
    plLog::Debug("[AudioSystem] Loaded Trigger '{0}'.", m_sTriggerName);
  };

  if (bSync)
  {
    plAudioSystem::GetSingleton()->SendRequestSync(registerEntity);
    plAudioSystem::GetSingleton()->SendRequestSync(loadTrigger);
  }
  else
  {
    plAudioSystem::GetSingleton()->SendRequest(registerEntity);
    plAudioSystem::GetSingleton()->SendRequest(loadTrigger);
  }
}

void plAudioAnimationEntry::UnloadTrigger()
{
  if (m_bTriggerLoaded)
    return;

  if (m_sTriggerName.IsEmpty())
    return;

  plAudioSystemRequestUnloadTrigger request;

  request.m_uiEntityId = m_uiEntityId;
  request.m_uiObjectId = plAudioSystem::GetSingleton()->GetTriggerId(m_sTriggerName);

  request.m_Callback = [this](const plAudioSystemRequestUnloadTrigger& m)
  {
    if (m.m_eStatus.Failed())
      return;

    m_bTriggerLoaded = false;
    plLog::Debug("[AudioSystem] Unloaded Trigger '{0}'.", m_sTriggerName);
  };

  plAudioSystem::GetSingleton()->SendRequest(request);
}

void plAudioAnimationComponent::Initialize()
{
  SUPER::Initialize();

  for (auto& entry : m_EventEntries)
  {
    entry.Initialize(false);
  }
}

void plAudioAnimationComponent::Deinitialize()
{
  for (auto& entry : m_EventEntries)
  {
    entry.UnloadTrigger();
  }

  SUPER::Deinitialize();
}

void plAudioAnimationComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioAnimationComponent);

  s << m_EventEntries.GetCount();
  for (auto& entry : m_EventEntries)
  {
    s.WriteVersion(kVersion_AudioAnimationEntry);
    s << entry.m_sEventName;
    s << entry.m_sTriggerName;
    s << entry.m_sJointName;
  }
}

void plAudioAnimationComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioAnimationComponent);

  plUInt32 count;
  s >> count;

  if (count == 0)
    return;

  m_EventEntries.Reserve(count);
  for (plUInt32 i = 0; i < count; i++)
  {
    plAudioAnimationEntry entry{};
    s.ReadVersion(kVersion_AudioAnimationEntry);
    s >> entry.m_sEventName;
    s >> entry.m_sTriggerName;
    s >> entry.m_sJointName;

    m_EventEntries.PushBack(entry);
  }
}

void plAudioAnimationComponent::Update()
{
  plAudioSystemRequestsQueue queue;

  for (auto& entry : m_EventEntries)
  {
    if (entry.m_sJointName.IsEmpty())
    {
      const auto& rotation = GetOwner()->GetGlobalRotation();

      plAudioSystemTransform transform;
      transform.m_vPosition = GetOwner()->GetGlobalPosition();
      transform.m_vForward = (rotation * plVec3::UnitXAxis()).GetNormalized();
      transform.m_vUp = (rotation * plVec3::UnitZAxis()).GetNormalized();
      transform.m_vVelocity = GetOwner()->GetLinearVelocity();

      if (transform == entry.m_LastTransform)
        return;

      plAudioSystemRequestSetEntityTransform request;

      request.m_uiEntityId = entry.m_uiEntityId;
      request.m_Transform = transform;

      request.m_Callback = [&entry](const plAudioSystemRequestSetEntityTransform& m)
      {
        if (m.m_eStatus.Failed())
          return;

        entry.m_LastTransform = m.m_Transform;
      };

      queue.PushBack(request);
    }
  }

  plAudioSystem::GetSingleton()->SendRequests(queue);
}

void plAudioAnimationComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg)
{
  plAudioSystemRequestsQueue queue;

  for (auto& entry : m_EventEntries)
  {
    if (!entry.m_sJointName.IsEmpty() && entry.m_uiJointIndex == plInvalidJointIndex)
    {
      entry.m_uiJointIndex = msg.m_pSkeleton->FindJointByName(entry.m_sJointName);
    }

    if (entry.m_uiJointIndex == plInvalidJointIndex)
      continue;

    plMat4 bone;
    plQuat boneRot;

    msg.ComputeFullBoneTransform(entry.m_uiJointIndex, bone, boneRot);

    plAudioSystemTransform transform;
    transform.m_vPosition = GetOwner()->GetGlobalPosition() + bone.GetTranslationVector();
    transform.m_vForward = (boneRot * plVec3::UnitXAxis()).GetNormalized();
    transform.m_vUp = (boneRot * plVec3::UnitZAxis()).GetNormalized();
    transform.m_vVelocity = transform.m_vPosition - entry.m_LastTransform.m_vPosition; // We can just mimic a velocity, since we have not this data in the bone transform

    if (transform == entry.m_LastTransform)
      continue;

    plAudioSystemRequestSetEntityTransform request;

    request.m_uiEntityId = entry.m_uiEntityId;
    request.m_Transform = transform;

    request.m_Callback = [&entry](const plAudioSystemRequestSetEntityTransform& m)
    {
      if (m.m_eStatus.Failed())
        return;

      entry.m_LastTransform = m.m_Transform;
    };

    queue.PushBack(request);
  }

  plAudioSystem::GetSingleton()->SendRequests(queue);
}

void plAudioAnimationComponent::OnAnimationEvent(plMsgGenericEvent& msg) const
{
  for (auto& entry : m_EventEntries)
  {
    if (entry.m_sEventName != msg.m_sMessage)
      continue;

    if (entry.m_sTriggerName.IsEmpty())
      continue;

    if (!entry.m_sJointName.IsEmpty() && entry.m_uiJointIndex == plInvalidJointIndex)
      continue;

    entry.ActivateTrigger();

    plLog::Debug("[AudioSystem] Trigger '{0}' activated for event {1}.", entry.m_sTriggerName, msg.m_sMessage);
  }
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Components_AudioAnimationComponent);
