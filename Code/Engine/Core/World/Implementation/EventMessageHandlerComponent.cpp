#include <Core/CorePCH.h>

#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

namespace
{
  static plStaticArray<plDynamicArray<plComponentHandle>*, 64> s_GlobalEventHandlerPerWorld;

  static void RegisterGlobalEventHandler(plComponent* pComponent)
  {
    const plUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    s_GlobalEventHandlerPerWorld.EnsureCount(uiWorldIndex + 1);

    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    if (globalEventHandler == nullptr)
    {
      globalEventHandler = PLASMA_NEW(plStaticAllocatorWrapper::GetAllocator(), plDynamicArray<plComponentHandle>);

      s_GlobalEventHandlerPerWorld[uiWorldIndex] = globalEventHandler;
    }

    globalEventHandler->PushBack(pComponent->GetHandle());
  }

  static void DeregisterGlobalEventHandler(plComponent* pComponent)
  {
    plUInt32 uiWorldIndex = pComponent->GetWorld()->GetIndex();
    auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex];
    PLASMA_ASSERT_DEV(globalEventHandler != nullptr, "Implementation error.");

    globalEventHandler->RemoveAndSwap(pComponent->GetHandle());
  }
} // namespace

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plEventMessageHandlerComponent, 3)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("HandleGlobalEvents", GetGlobalEventHandlerMode, SetGlobalEventHandlerMode),
    PLASMA_ACCESSOR_PROPERTY("PassThroughUnhandledEvents", GetPassThroughUnhandledEvents, SetPassThroughUnhandledEvents),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE;
// clang-format on

plEventMessageHandlerComponent::plEventMessageHandlerComponent() = default;
plEventMessageHandlerComponent::~plEventMessageHandlerComponent() = default;

void plEventMessageHandlerComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();

  // version 2
  s << m_bIsGlobalEventHandler;

  // version 3
  s << m_bPassThroughUnhandledEvents;
}

void plEventMessageHandlerComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = stream.GetStream();

  if (uiVersion >= 2)
  {
    bool bGlobalEH;
    s >> bGlobalEH;

    SetGlobalEventHandlerMode(bGlobalEH);
  }

  if (uiVersion >= 3)
  {
    s >> m_bPassThroughUnhandledEvents;
  }
}

void plEventMessageHandlerComponent::Deinitialize()
{
  SetGlobalEventHandlerMode(false);

  SUPER::Deinitialize();
}

void plEventMessageHandlerComponent::SetDebugOutput(bool enable)
{
  m_bDebugOutput = enable;
}

bool plEventMessageHandlerComponent::GetDebugOutput() const
{
  return m_bDebugOutput;
}

void plEventMessageHandlerComponent::SetGlobalEventHandlerMode(bool enable)
{
  if (m_bIsGlobalEventHandler == enable)
    return;

  m_bIsGlobalEventHandler = enable;

  if (enable)
  {
    RegisterGlobalEventHandler(this);
  }
  else
  {
    DeregisterGlobalEventHandler(this);
  }
}

void plEventMessageHandlerComponent::SetPassThroughUnhandledEvents(bool bPassThrough)
{
  m_bPassThroughUnhandledEvents = bPassThrough;
}

// static
plArrayPtr<plComponentHandle> plEventMessageHandlerComponent::GetAllGlobalEventHandler(const plWorld* pWorld)
{
  plUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    if (auto globalEventHandler = s_GlobalEventHandlerPerWorld[uiWorldIndex])
    {
      return globalEventHandler->GetArrayPtr();
    }
  }

  return plArrayPtr<plComponentHandle>();
}


void plEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(const plWorld* pWorld)
{
  plUInt32 uiWorldIndex = pWorld->GetIndex();

  if (uiWorldIndex < s_GlobalEventHandlerPerWorld.GetCount())
  {
    s_GlobalEventHandlerPerWorld[uiWorldIndex]->Clear();
  }
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_EventMessageHandlerComponent);
