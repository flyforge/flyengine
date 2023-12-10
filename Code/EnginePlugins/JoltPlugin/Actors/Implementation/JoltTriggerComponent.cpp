#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Profiling/Profiling.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

plJoltTriggerComponentManager::plJoltTriggerComponentManager(plWorld* pWorld)
  : plComponentManager<plJoltTriggerComponent, plBlockStorageType::FreeList>(pWorld)
{
}

plJoltTriggerComponentManager::~plJoltTriggerComponentManager() = default;

void plJoltTriggerComponentManager::UpdateMovingTriggers()
{
  PLASMA_PROFILE_SCOPE("MovingTriggers");

  plJoltWorldModule* pModule = GetWorld()->GetModule<plJoltWorldModule>();
  auto& bodyInterface = pModule->GetJoltSystem()->GetBodyInterface();

  for (auto pTrigger : m_MovingTriggers)
  {
    JPH::BodyID bodyId(pTrigger->m_uiJoltBodyID);

    plSimdTransform trans = pTrigger->GetOwner()->GetGlobalTransformSimd();

    bodyInterface.SetPositionAndRotation(bodyId, plJoltConversionUtils::ToVec3(trans.m_Position), plJoltConversionUtils::ToQuat(trans.m_Rotation), JPH::EActivation::Activate);
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltTriggerComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("TriggerMessage", GetTriggerMessage, SetTriggerMessage)
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGESENDERS
  {
    PLASMA_MESSAGE_SENDER(m_TriggerEventSender)
  }
  PLASMA_END_MESSAGESENDERS
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltTriggerComponent::plJoltTriggerComponent() = default;
plJoltTriggerComponent::~plJoltTriggerComponent() = default;

void plJoltTriggerComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();

  s << m_sTriggerMessage;
}

void plJoltTriggerComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = inout_stream.GetStream();

  s >> m_sTriggerMessage;
}

void plJoltTriggerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  plJoltUserData* pUserData = nullptr;
  m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
  pUserData->Init(this);

  JPH::BodyCreationSettings bodyCfg;
  if (CreateShape(&bodyCfg, 1.0f, nullptr).Failed())
  {
    plLog::Error("Jolt trigger actor component has no valid shape.");
    return;
  }

  const plSimdTransform trans = GetOwner()->GetGlobalTransformSimd();

  auto* pSystem = pModule->GetJoltSystem();
  auto* pBodies = &pSystem->GetBodyInterface();

  bodyCfg.mIsSensor = true;
  bodyCfg.mPosition = plJoltConversionUtils::ToVec3(trans.m_Position);
  bodyCfg.mRotation = plJoltConversionUtils::ToQuat(trans.m_Rotation);
  bodyCfg.mMotionType = JPH::EMotionType::Kinematic;
  bodyCfg.mObjectLayer = plJoltCollisionFiltering::ConstructObjectLayer(m_uiCollisionLayer, plJoltBroadphaseLayer::Trigger);
  bodyCfg.mCollisionGroup.SetGroupID(m_uiObjectFilterID);
  // bodyCfg.mCollisionGroup.SetGroupFilter(pModule->GetGroupFilter()); // the group filter is only needed for objects constrained via joints
  bodyCfg.mUserData = reinterpret_cast<plUInt64>(pUserData);

  JPH::Body* pBody = pBodies->CreateBody(bodyCfg);
  m_uiJoltBodyID = pBody->GetID().GetIndexAndSequenceNumber();

  pModule->QueueBodyToAdd(pBody, true);

  if (GetOwner()->IsDynamic())
  {
    plJoltTriggerComponentManager* pManager = static_cast<plJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Insert(this);
  }
}

void plJoltTriggerComponent::OnDeactivated()
{
  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();

  plJoltContactListener* pContactListener = pModule->GetContactListener();
  pContactListener->RemoveTrigger(this);

  if (GetOwner()->IsDynamic())
  {
    plJoltTriggerComponentManager* pManager = static_cast<plJoltTriggerComponentManager*>(GetOwningManager());
    pManager->m_MovingTriggers.Remove(this);
  }

  SUPER::OnDeactivated();
}

void plJoltTriggerComponent::PostTriggerMessage(const plGameObjectHandle& hOtherObject, plTriggerState::Enum triggerState) const
{
  plMsgTriggerTriggered msg;

  msg.m_TriggerState = triggerState;
  msg.m_sMessage = m_sTriggerMessage;
  msg.m_hTriggeringObject = hOtherObject;

  m_TriggerEventSender.PostEventMessage(msg, this, GetOwner(), plTime::MakeZero(), plObjectMsgQueueType::PostTransform);
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Actors_Implementation_JoltTriggerComponent);

