#include <Core/CorePCH.h>

#include <Core/World/World.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plComponentManagerBase, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plComponentManagerBase::plComponentManagerBase(plWorld* pWorld)
  : plWorldModule(pWorld)
  , m_Components(pWorld->GetAllocator())
{
}

plComponentManagerBase::~plComponentManagerBase() {}

plComponentHandle plComponentManagerBase::CreateComponent(plGameObject* pOwnerObject)
{
  plComponent* pDummy;
  return CreateComponent(pOwnerObject, pDummy);
}

void plComponentManagerBase::DeleteComponent(const plComponentHandle& component)
{
  plComponent* pComponent = nullptr;
  if (!m_Components.TryGetValue(component, pComponent))
    return;

  DeleteComponent(pComponent);
}

void plComponentManagerBase::DeleteComponent(plComponent* pComponent)
{
  if (pComponent == nullptr)
    return;

  DeinitializeComponent(pComponent);

  m_Components.Remove(pComponent->m_InternalId);

  pComponent->m_InternalId.Invalidate();
  pComponent->m_ComponentFlags.Remove(plObjectFlags::ActiveFlag | plObjectFlags::ActiveState);

  GetWorld()->m_Data.m_DeadComponents.Insert(pComponent);
}

void plComponentManagerBase::Deinitialize()
{
  for (auto it = m_Components.GetIterator(); it.IsValid(); ++it)
  {
    DeinitializeComponent(it.Value());
  }

  SUPER::Deinitialize();
}

plComponentHandle plComponentManagerBase::CreateComponentNoInit(plGameObject* pOwnerObject, plComponent*& out_pComponent)
{
  PLASMA_ASSERT_DEV(m_Components.GetCount() < plWorld::GetMaxNumComponentsPerType(), "Max number of components per type reached: {}",
    plWorld::GetMaxNumComponentsPerType());

  plComponent* pComponent = CreateComponentStorage();
  if (pComponent == nullptr)
  {
    return plComponentHandle();
  }

  plComponentId newId = m_Components.Insert(pComponent);
  newId.m_WorldIndex = GetWorldIndex();
  newId.m_TypeId = pComponent->GetTypeId();

  pComponent->m_pManager = this;
  pComponent->m_InternalId = newId;
  pComponent->m_ComponentFlags.AddOrRemove(plObjectFlags::Dynamic, pComponent->GetMode() == plComponentMode::Dynamic);

  // In Editor we add components via reflection so it is fine to have a nullptr here.
  // We check for a valid owner before the Initialize() callback.
  if (pOwnerObject != nullptr)
  {
    // AddComponent will update the active state internally
    pOwnerObject->AddComponent(pComponent);
  }
  else
  {
    pComponent->UpdateActiveState(true);
  }

  out_pComponent = pComponent;
  return pComponent->GetHandle();
}

void plComponentManagerBase::InitializeComponent(plComponent* pComponent)
{
  GetWorld()->AddComponentToInitialize(pComponent->GetHandle());
}

void plComponentManagerBase::DeinitializeComponent(plComponent* pComponent)
{
  if (pComponent->IsInitialized())
  {
    pComponent->Deinitialize();
    pComponent->m_ComponentFlags.Remove(plObjectFlags::Initialized);
  }

  if (plGameObject* pOwner = pComponent->GetOwner())
  {
    pOwner->RemoveComponent(pComponent);
  }
}

void plComponentManagerBase::PatchIdTable(plComponent* pComponent)
{
  plComponentId id = pComponent->m_InternalId;
  if (id.m_InstanceIndex != plComponentId::INVALID_INSTANCE_INDEX)
    m_Components[id] = pComponent;
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_ComponentManager);
