#include <GameEngine/GameEnginePCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/StateMachine/StateMachineComponent.h>

// clang-format off
PLASMA_IMPLEMENT_MESSAGE_TYPE(plMsgStateMachineStateChanged);
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plMsgStateMachineStateChanged, 1, plRTTIDefaultAllocator<plMsgStateMachineStateChanged>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("OldStateName", GetOldStateName, SetOldStateName),
    PLASMA_ACCESSOR_PROPERTY("NewStateName", GetNewStateName, SetNewStateName),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_SendMsg, 1, plRTTIDefaultAllocator<plStateMachineState_SendMsg>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("MessageDelay", m_MessageDelay),
    PLASMA_MEMBER_PROPERTY("SendMessageOnEnter", m_bSendMessageOnEnter)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("SendMessageOnExit", m_bSendMessageOnExit),
    PLASMA_MEMBER_PROPERTY("LogOnEnter", m_bLogOnEnter),
    PLASMA_MEMBER_PROPERTY("LogOnExit", m_bLogOnExit),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState_SendMsg::plStateMachineState_SendMsg(plStringView sName)
  : plStateMachineState(sName)
{
}

plStateMachineState_SendMsg::~plStateMachineState_SendMsg() = default;

void plStateMachineState_SendMsg::OnEnter(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pFromState) const
{
  plHashedString sFromState = (pFromState != nullptr) ? pFromState->GetNameHashed() : plHashedString();

  if (m_bSendMessageOnEnter)
  {
    if (auto pOwner = plDynamicCast<plStateMachineComponent*>(&instance.GetOwner()))
    {
      plMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = sFromState;
      msg.m_sNewStateName = GetNameHashed();

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnEnter)
  {
    plLog::Info("State Machine: Entering '{}' State from '{}'", GetNameHashed(), sFromState);
  }
}

void plStateMachineState_SendMsg::OnExit(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pToState) const
{
  plHashedString sToState = (pToState != nullptr) ? pToState->GetNameHashed() : plHashedString();

  if (m_bSendMessageOnExit)
  {
    if (auto pOwner = plDynamicCast<plStateMachineComponent*>(&instance.GetOwner()))
    {
      plMsgStateMachineStateChanged msg;
      msg.m_sOldStateName = GetNameHashed();
      msg.m_sNewStateName = sToState;

      pOwner->SendStateChangedMsg(msg, m_MessageDelay);
    }
  }

  if (m_bLogOnExit)
  {
    plLog::Info("State Machine: Exiting '{}' State to '{}'", GetNameHashed(), sToState);
  }
}

plResult plStateMachineState_SendMsg::Serialize(plStreamWriter& stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_MessageDelay;
  stream << m_bSendMessageOnEnter;
  stream << m_bSendMessageOnExit;
  stream << m_bLogOnEnter;
  stream << m_bLogOnExit;
  return PLASMA_SUCCESS;
}

plResult plStateMachineState_SendMsg::Deserialize(plStreamReader& stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  stream >> m_MessageDelay;
  stream >> m_bSendMessageOnEnter;
  stream >> m_bSendMessageOnExit;
  stream >> m_bLogOnEnter;
  stream >> m_bLogOnExit;
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plStateMachineState_SwitchObject, 1, plRTTIDefaultAllocator<plStateMachineState_SwitchObject>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("PathToGroup", m_sGroupPath),
    PLASMA_MEMBER_PROPERTY("ObjectToEnable", m_sObjectToEnable),
    PLASMA_MEMBER_PROPERTY("DeactivateOthers", m_bDeactivateOthers)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plStateMachineState_SwitchObject::plStateMachineState_SwitchObject(plStringView sName)
  : plStateMachineState(sName)
{
}

plStateMachineState_SwitchObject::~plStateMachineState_SwitchObject() = default;

void plStateMachineState_SwitchObject::OnEnter(plStateMachineInstance& instance, void* pInstanceData, const plStateMachineState* pFromState) const
{
  if (auto pOwner = plDynamicCast<plStateMachineComponent*>(&instance.GetOwner()))
  {
    if (plGameObject* pOwnerGO = pOwner->GetOwner()->FindChildByPath(m_sGroupPath))
    {
      for (auto it = pOwnerGO->GetChildren(); it.IsValid(); ++it)
      {
        if (it->GetName() == m_sObjectToEnable)
        {
          it->SetActiveFlag(true);
        }
        else if (m_bDeactivateOthers)
        {
          it->SetActiveFlag(false);
        }
      }
    }
  }
}

plResult plStateMachineState_SwitchObject::Serialize(plStreamWriter& stream) const
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Serialize(stream));

  stream << m_sGroupPath;
  stream << m_sObjectToEnable;
  stream << m_bDeactivateOthers;
  return PLASMA_SUCCESS;
}

plResult plStateMachineState_SwitchObject::Deserialize(plStreamReader& stream)
{
  PLASMA_SUCCEED_OR_RETURN(SUPER::Deserialize(stream));

  stream >> m_sGroupPath;
  stream >> m_sObjectToEnable;
  stream >> m_bDeactivateOthers;
  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

plStateMachineComponentManager::plStateMachineComponentManager(plWorld* pWorld)
  : plComponentManager<ComponentType, plBlockStorageType::Compact>(pWorld)
{
  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plStateMachineComponentManager::ResourceEventHandler, this));
}

plStateMachineComponentManager::~plStateMachineComponentManager()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plStateMachineComponentManager::ResourceEventHandler, this));
}

void plStateMachineComponentManager::Initialize()
{
  auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plStateMachineComponentManager::Update, this);

  RegisterUpdateFunction(desc);
}

void plStateMachineComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  // reload
  {
    for (auto hComponent : m_ComponentsToReload)
    {
      plStateMachineComponent* pComponent = nullptr;
      if (TryGetComponent(hComponent, pComponent) && pComponent->IsActive())
      {
        pComponent->InstantiateStateMachine();
      }
    }
    m_ComponentsToReload.Clear();
  }

  // update
  if (GetWorld()->GetWorldSimulationEnabled())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
    {
      ComponentType* pComponent = it;
      if (pComponent->IsActiveAndSimulating())
      {
        pComponent->Update();
      }
    }
  }
}

void plStateMachineComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plStateMachineResource>())
  {
    plStateMachineResourceHandle hResource((plStateMachineResource*)(e.m_pResource));

    for (auto it = GetComponents(); it.IsValid(); it.Next())
    {
      if (it->m_hResource == hResource)
      {
        m_ComponentsToReload.Insert(it->GetHandle());
      }
    }
  }
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plStateMachineComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Resource", GetResourceFile, SetResourceFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_StateMachine")),
    PLASMA_ACCESSOR_PROPERTY("InitialState", GetInitialState, SetInitialState),
    PLASMA_ACCESSOR_PROPERTY("BlackboardName", GetBlackboardName, SetBlackboardName),
  }
  PLASMA_END_PROPERTIES;

  PLASMA_BEGIN_MESSAGESENDERS
  {
    PLASMA_MESSAGE_SENDER(m_StateChangedSender)
  }
  PLASMA_END_MESSAGESENDERS;

  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetState, In, "Name"),
  }
  PLASMA_END_FUNCTIONS;

  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Gameplay/Logic"),
    new plColorAttribute(plColorScheme::Logic),
  }
  PLASMA_END_ATTRIBUTES;
}

PLASMA_END_DYNAMIC_REFLECTED_TYPE
// clang-format on

plStateMachineComponent::plStateMachineComponent() = default;
plStateMachineComponent::plStateMachineComponent(plStateMachineComponent&& other) = default;
plStateMachineComponent::~plStateMachineComponent() = default;
plStateMachineComponent& plStateMachineComponent::operator=(plStateMachineComponent&& other) = default;

void plStateMachineComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  plStreamWriter& s = stream.GetStream();

  s << m_hResource;
  s << m_sInitialState;
  s << m_sBlackboardName;
}

void plStateMachineComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_hResource;
  s >> m_sInitialState;

  if (uiVersion >= 2)
  {
    s >> m_sBlackboardName;
  }
}

void plStateMachineComponent::OnActivated()
{
  SUPER::OnActivated();

  InstantiateStateMachine();
}

void plStateMachineComponent::OnDeactivated()
{
  SUPER::OnDeactivated();

  m_pStateMachineInstance = nullptr;
}

void plStateMachineComponent::SetResource(const plStateMachineResourceHandle& hResource)
{
  if (m_hResource == hResource)
    return;

  m_hResource = hResource;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void plStateMachineComponent::SetResourceFile(const char* szFile)
{
  plStateMachineResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plStateMachineResource>(szFile);
    plResourceManager::PreloadResource(hResource);
  }

  SetResource(hResource);
}

const char* plStateMachineComponent::GetResourceFile() const
{
  if (!m_hResource.IsValid())
    return "";

  return m_hResource.GetResourceID();
}

void plStateMachineComponent::SetInitialState(const char* szName)
{
  plHashedString sInitialState;
  sInitialState.Assign(szName);

  if (m_sInitialState == sInitialState)
    return;

  m_sInitialState = sInitialState;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

void plStateMachineComponent::SetBlackboardName(const char* szName)
{
  plHashedString sBlackboardName;
  sBlackboardName.Assign(szName);

  if (m_sBlackboardName == sBlackboardName)
    return;

  m_sBlackboardName = sBlackboardName;

  if (IsActiveAndInitialized())
  {
    InstantiateStateMachine();
  }
}

bool plStateMachineComponent::SetState(plStringView sName)
{
  if (m_pStateMachineInstance != nullptr)
  {
    plHashedString sStateName;
    sStateName.Assign(sName);

    return m_pStateMachineInstance->SetState(sStateName).Succeeded();
  }

  return false;
}


void plStateMachineComponent::SendStateChangedMsg(plMsgStateMachineStateChanged& msg, plTime delay)
{
  if (delay > plTime::Zero())
  {
    m_StateChangedSender.PostEventMessage(msg, this, GetOwner(), delay, plObjectMsgQueueType::NextFrame);
  }
  else
  {
    m_StateChangedSender.SendEventMessage(msg, this, GetOwner());
  }
}

void plStateMachineComponent::InstantiateStateMachine()
{
  m_pStateMachineInstance = nullptr;

  if (m_hResource.IsValid() == false)
    return;

  plResourceLock<plStateMachineResource> pStateMachineResource(m_hResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pStateMachineResource.GetAcquireResult() != plResourceAcquireResult::Final)
  {
    plLog::Error("Failed to load state machine '{}'", GetResourceFile());
    return;
  }

  m_pStateMachineInstance = pStateMachineResource->CreateInstance(*this);
  m_pStateMachineInstance->SetBlackboard(plBlackboardComponent::FindBlackboard(GetOwner(), m_sBlackboardName.GetView()));
  m_pStateMachineInstance->SetStateOrFallback(m_sInitialState).IgnoreResult();
}

void plStateMachineComponent::Update()
{
  if (m_pStateMachineInstance != nullptr)
  {
    m_pStateMachineInstance->Update(GetWorld()->GetClock().GetTimeDiff());
  }
}
