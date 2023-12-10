
PLASMA_FORCE_INLINE bool plComponentManagerBase::IsValidComponent(const plComponentHandle& component) const
{
  return m_Components.Contains(component);
}

PLASMA_FORCE_INLINE bool plComponentManagerBase::TryGetComponent(const plComponentHandle& component, plComponent*& out_pComponent)
{
  return m_Components.TryGetValue(component, out_pComponent);
}

PLASMA_FORCE_INLINE bool plComponentManagerBase::TryGetComponent(const plComponentHandle& component, const plComponent*& out_pComponent) const
{
  plComponent* pComponent = nullptr;
  bool res = m_Components.TryGetValue(component, pComponent);
  out_pComponent = pComponent;
  return res;
}

PLASMA_ALWAYS_INLINE plUInt32 plComponentManagerBase::GetComponentCount() const
{
  return static_cast<plUInt32>(m_Components.GetCount());
}

template <typename ComponentType>
PLASMA_ALWAYS_INLINE plComponentHandle plComponentManagerBase::CreateComponent(plGameObject* pOwnerObject, ComponentType*& out_pComponent)
{
  plComponent* pComponent = nullptr;
  plComponentHandle hComponent = CreateComponentNoInit(pOwnerObject, pComponent);

  if (pComponent != nullptr)
  {
    InitializeComponent(pComponent);
  }

  out_pComponent = plStaticCast<ComponentType*>(pComponent);
  return hComponent;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, plBlockStorageType::Enum StorageType>
plComponentManager<T, StorageType>::plComponentManager(plWorld* pWorld)
  : plComponentManagerBase(pWorld)
  , m_ComponentStorage(GetBlockAllocator(), GetAllocator())
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponent, ComponentType), "Not a valid component type");
}

template <typename T, plBlockStorageType::Enum StorageType>
plComponentManager<T, StorageType>::~plComponentManager()
{
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_FORCE_INLINE bool plComponentManager<T, StorageType>::TryGetComponent(const plComponentHandle& component, ComponentType*& out_pComponent)
{
  PLASMA_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    component.GetInternalID().m_TypeId);
  PLASMA_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), component.GetInternalID().m_WorldIndex);

  plComponent* pComponent = nullptr;
  bool bResult = plComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<ComponentType*>(pComponent);
  return bResult;
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_FORCE_INLINE bool plComponentManager<T, StorageType>::TryGetComponent(
  const plComponentHandle& component, const ComponentType*& out_pComponent) const
{
  PLASMA_ASSERT_DEV(ComponentType::TypeId() == component.GetInternalID().m_TypeId,
    "The given component handle is not of the expected type. Expected type id {0}, got type id {1}", ComponentType::TypeId(),
    component.GetInternalID().m_TypeId);
  PLASMA_ASSERT_DEV(component.GetInternalID().m_WorldIndex == GetWorldIndex(),
    "Component does not belong to this world. Expected world id {0} got id {1}", GetWorldIndex(), component.GetInternalID().m_WorldIndex);

  const plComponent* pComponent = nullptr;
  bool bResult = plComponentManagerBase::TryGetComponent(component, pComponent);
  out_pComponent = static_cast<const ComponentType*>(pComponent);
  return bResult;
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_ALWAYS_INLINE typename plBlockStorage<T, plInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator plComponentManager<T, StorageType>::GetComponents(plUInt32 uiStartIndex /*= 0*/)
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_ALWAYS_INLINE typename plBlockStorage<T, plInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator
plComponentManager<T, StorageType>::GetComponents(plUInt32 uiStartIndex /*= 0*/) const
{
  return m_ComponentStorage.GetIterator(uiStartIndex);
}

// static
template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_ALWAYS_INLINE plWorldModuleTypeId plComponentManager<T, StorageType>::TypeId()
{
  return T::TypeId();
}

template <typename T, plBlockStorageType::Enum StorageType>
void plComponentManager<T, StorageType>::CollectAllComponents(plDynamicArray<plComponentHandle>& out_AllComponents, bool bOnlyActive)
{
  out_AllComponents.Reserve(out_AllComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_AllComponents.PushBack(it->GetHandle());
    }
  }
}

template <typename T, plBlockStorageType::Enum StorageType>
void plComponentManager<T, StorageType>::CollectAllComponents(plDynamicArray<plComponent*>& out_AllComponents, bool bOnlyActive)
{
  out_AllComponents.Reserve(out_AllComponents.GetCount() + m_ComponentStorage.GetCount());

  for (auto it = GetComponents(); it.IsValid(); it.Next())
  {
    if (!bOnlyActive || it->IsActive())
    {
      out_AllComponents.PushBack(it);
    }
  }
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_ALWAYS_INLINE plComponent* plComponentManager<T, StorageType>::CreateComponentStorage()
{
  return m_ComponentStorage.Create();
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_FORCE_INLINE void plComponentManager<T, StorageType>::DeleteComponentStorage(plComponent* pComponent, plComponent*& out_pMovedComponent)
{
  T* pMovedComponent = nullptr;
  m_ComponentStorage.Delete(static_cast<T*>(pComponent), pMovedComponent);
  out_pMovedComponent = pMovedComponent;
}

template <typename T, plBlockStorageType::Enum StorageType>
PLASMA_FORCE_INLINE void plComponentManager<T, StorageType>::RegisterUpdateFunction(UpdateFunctionDesc& desc)
{
  // round up to multiple of data block capacity so tasks only have to deal with complete data blocks
  if (desc.m_uiGranularity != 0)
    desc.m_uiGranularity = static_cast<plUInt16>(
      plMath::RoundUp(static_cast<plInt32>(desc.m_uiGranularity), plDataBlock<ComponentType, plInternal::DEFAULT_BLOCK_SIZE>::CAPACITY));

  plComponentManagerBase::RegisterUpdateFunction(desc);
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename ComponentType, plComponentUpdateType::Enum UpdateType, plBlockStorageType::Enum StorageType>
plComponentManagerSimple<ComponentType, UpdateType, StorageType>::plComponentManagerSimple(plWorld* pWorld)
  : plComponentManager<ComponentType, StorageType>(pWorld)
{
}

template <typename ComponentType, plComponentUpdateType::Enum UpdateType, plBlockStorageType::Enum StorageType>
void plComponentManagerSimple<ComponentType, UpdateType, StorageType>::Initialize()
{
  typedef plComponentManagerSimple<ComponentType, UpdateType, StorageType> OwnType;

  plStringBuilder functionName;
  SimpleUpdateName(functionName);

  auto desc = plWorldModule::UpdateFunctionDesc(plWorldModule::UpdateFunction(&OwnType::SimpleUpdate, this), functionName);
  desc.m_bOnlyUpdateWhenSimulating = (UpdateType == plComponentUpdateType::WhenSimulating);

  this->RegisterUpdateFunction(desc);
}

template <typename ComponentType, plComponentUpdateType::Enum UpdateType, plBlockStorageType::Enum StorageType>
void plComponentManagerSimple<ComponentType, UpdateType, StorageType>::SimpleUpdate(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    ComponentType* pComponent = it;
    if (pComponent->IsActiveAndInitialized())
    {
      pComponent->Update();
    }
  }
}

// static
template <typename ComponentType, plComponentUpdateType::Enum UpdateType, plBlockStorageType::Enum StorageType>
void plComponentManagerSimple<ComponentType, UpdateType, StorageType>::SimpleUpdateName(plStringBuilder& out_sName)
{
  plStringView sName(PLASMA_SOURCE_FUNCTION);
  const char* szEnd = sName.FindSubString(",");

  if (szEnd != nullptr && sName.StartsWith("plComponentManagerSimple<class "))
  {
    plStringView sChoppedName(sName.GetStartPointer() + plStringUtils::GetStringElementCount("plComponentManagerSimple<class "), szEnd);

    PLASMA_ASSERT_DEV(!sChoppedName.IsEmpty(), "Chopped name is empty: '{0}'", sName);

    out_sName = sChoppedName;
    out_sName.Append("::SimpleUpdate");
  }
  else
  {
    out_sName = sName;
  }
}
