#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/World/World.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plComponent, 1, plRTTINoAllocator)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Active", GetActiveFlag, SetActiveFlag)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(IsActive),
    PL_SCRIPT_FUNCTION_PROPERTY(IsActiveAndInitialized),
    PL_SCRIPT_FUNCTION_PROPERTY(IsActiveAndSimulating),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_GetOwner),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_GetWorld),
    PL_SCRIPT_FUNCTION_PROPERTY(GetUniqueID),
    PL_SCRIPT_FUNCTION_PROPERTY(Initialize)->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::Initialize)),
    PL_SCRIPT_FUNCTION_PROPERTY(Deinitialize)->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::Deinitialize)),
    PL_SCRIPT_FUNCTION_PROPERTY(OnActivated)->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::OnActivated)),
    PL_SCRIPT_FUNCTION_PROPERTY(OnDeactivated)->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::OnDeactivated)),
    PL_SCRIPT_FUNCTION_PROPERTY(OnSimulationStarted)->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::OnSimulationStarted)),
    PL_SCRIPT_FUNCTION_PROPERTY(Reflection_Update, In, "DeltaTime")->AddAttributes(new plScriptBaseClassFunctionAttribute(plComponent_ScriptBaseClassFunctions::Update)),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plComponent::SetActiveFlag(bool bEnabled)
{
  if (m_ComponentFlags.IsSet(plObjectFlags::ActiveFlag) != bEnabled)
  {
    m_ComponentFlags.AddOrRemove(plObjectFlags::ActiveFlag, bEnabled);

    UpdateActiveState(GetOwner() == nullptr ? true : GetOwner()->IsActive());
  }
}

plWorld* plComponent::GetWorld()
{
  return m_pManager->GetWorld();
}

const plWorld* plComponent::GetWorld() const
{
  return m_pManager->GetWorld();
}

void plComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
}

void plComponent::DeserializeComponent(plWorldReader& inout_stream)
{
}

void plComponent::EnsureInitialized()
{
  PL_ASSERT_DEV(m_pOwner != nullptr, "Owner must not be null");

  if (IsInitializing())
  {
    plLog::Error("Recursive initialize call is ignored.");
    return;
  }

  if (!IsInitialized())
  {
    m_pMessageDispatchType = GetDynamicRTTI();

    m_ComponentFlags.Add(plObjectFlags::Initializing);

    Initialize();

    m_ComponentFlags.Remove(plObjectFlags::Initializing);
    m_ComponentFlags.Add(plObjectFlags::Initialized);
  }
}

void plComponent::EnsureSimulationStarted()
{
  PL_ASSERT_DEV(IsActiveAndInitialized(), "Must not be called on uninitialized or inactive components.");
  PL_ASSERT_DEV(GetWorld()->GetWorldSimulationEnabled(), "Must not be called when the world is not simulated.");

  if (m_ComponentFlags.IsSet(plObjectFlags::SimulationStarting))
  {
    plLog::Error("Recursive simulation started call is ignored.");
    return;
  }

  if (!IsSimulationStarted())
  {
    m_ComponentFlags.Add(plObjectFlags::SimulationStarting);

    OnSimulationStarted();

    m_ComponentFlags.Remove(plObjectFlags::SimulationStarting);
    m_ComponentFlags.Add(plObjectFlags::SimulationStarted);
  }
}

void plComponent::PostMessage(const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  GetWorld()->PostMessage(GetHandle(), msg, delay, queueType);
}

bool plComponent::HandlesMessage(const plMessage& msg) const
{
  return m_pMessageDispatchType->CanHandleMessage(msg.GetId());
}

void plComponent::SetUserFlag(plUInt8 uiFlagIndex, bool bSet)
{
  PL_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  m_ComponentFlags.AddOrRemove(static_cast<plObjectFlags::Enum>(plObjectFlags::UserFlag0 << uiFlagIndex), bSet);
}

bool plComponent::GetUserFlag(plUInt8 uiFlagIndex) const
{
  PL_ASSERT_DEBUG(uiFlagIndex < 8, "Flag index {0} is out of the valid range [0 - 7]", uiFlagIndex);

  return m_ComponentFlags.IsSet(static_cast<plObjectFlags::Enum>(plObjectFlags::UserFlag0 << uiFlagIndex));
}

void plComponent::Initialize() {}

void plComponent::Deinitialize()
{
  PL_ASSERT_DEV(m_pOwner != nullptr, "Owner must still be valid");

  SetActiveFlag(false);
}

void plComponent::OnActivated() {}

void plComponent::OnDeactivated() {}

void plComponent::OnSimulationStarted() {}

void plComponent::EnableUnhandledMessageHandler(bool enable)
{
  m_ComponentFlags.AddOrRemove(plObjectFlags::UnhandledMessageHandler, enable);
}

bool plComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg)
{
  return false;
}

bool plComponent::OnUnhandledMessage(plMessage& msg, bool bWasPostedMsg) const
{
  return false;
}

void plComponent::UpdateActiveState(bool bOwnerActive)
{
  const bool bSelfActive = bOwnerActive && m_ComponentFlags.IsSet(plObjectFlags::ActiveFlag);

  if (m_ComponentFlags.IsSet(plObjectFlags::ActiveState) != bSelfActive)
  {
    m_ComponentFlags.AddOrRemove(plObjectFlags::ActiveState, bSelfActive);

    if (IsInitialized())
    {
      if (bSelfActive)
      {
        // Don't call OnActivated & EnsureSimulationStarted here since there might be other components
        // that are needed in the OnSimulation callback but are activated right after this component.
        // Instead add the component to the initialization batch again.
        // There initialization will be skipped since the component is already initialized.
        GetWorld()->AddComponentToInitialize(GetHandle());
      }
      else
      {
        OnDeactivated();

        m_ComponentFlags.Remove(plObjectFlags::SimulationStarted);
      }
    }
  }
}

plGameObject* plComponent::Reflection_GetOwner() const
{
  return m_pOwner;
}

plWorld* plComponent::Reflection_GetWorld() const
{
  return m_pManager->GetWorld();
}

void plComponent::Reflection_Update(plTime deltaTime)
{
  // This is just a dummy function for the scripting reflection
}

bool plComponent::SendMessageInternal(plMessage& msg, bool bWasPostedMsg)
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      plLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(plObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    plLog::Warning("Component type '{0}' does not have a message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}

bool plComponent::SendMessageInternal(plMessage& msg, bool bWasPostedMsg) const
{
  if (!IsActiveAndInitialized() && !IsInitializing())
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
      plLog::Warning("Discarded message with ID {0} because component of type '{1}' is neither initialized nor active at the moment", msg.GetId(),
        GetDynamicRTTI()->GetTypeName());
#endif

    return false;
  }

  if (m_pMessageDispatchType->DispatchMessage(this, msg))
    return true;

  if (m_ComponentFlags.IsSet(plObjectFlags::UnhandledMessageHandler) && OnUnhandledMessage(msg, bWasPostedMsg))
    return true;

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  if (msg.GetDebugMessageRouting())
    plLog::Warning(
      "(const) Component type '{0}' does not have a CONST message handler for messages of type {1}", GetDynamicRTTI()->GetTypeName(), msg.GetId());
#endif

  return false;
}


PL_STATICLINK_FILE(Core, Core_World_Implementation_Component);
