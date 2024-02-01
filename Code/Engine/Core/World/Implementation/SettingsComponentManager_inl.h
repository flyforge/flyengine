
template <typename ComponentType>
plSettingsComponentManager<ComponentType>::plSettingsComponentManager(plWorld* pWorld)
  : plComponentManagerBase(pWorld)
{
}

template <typename ComponentType>
plSettingsComponentManager<ComponentType>::~plSettingsComponentManager()
{
  for (auto& component : m_Components)
  {
    DeinitializeComponent(component.Borrow());
  }
}

template <typename ComponentType>
PL_ALWAYS_INLINE ComponentType* plSettingsComponentManager<ComponentType>::GetSingletonComponent()
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

template <typename ComponentType>
PL_ALWAYS_INLINE const ComponentType* plSettingsComponentManager<ComponentType>::GetSingletonComponent() const
{
  for (const auto& pComponent : m_Components)
  {
    // retrieve the first component that is active
    if (pComponent->IsActive())
      return pComponent.Borrow();
  }

  return nullptr;
}

// static
template <typename ComponentType>
PL_ALWAYS_INLINE plWorldModuleTypeId plSettingsComponentManager<ComponentType>::TypeId()
{
  return ComponentType::TypeId();
}

template <typename ComponentType>
void plSettingsComponentManager<ComponentType>::CollectAllComponents(plDynamicArray<plComponentHandle>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component->GetHandle());
    }
  }
}

template <typename ComponentType>
void plSettingsComponentManager<ComponentType>::CollectAllComponents(plDynamicArray<plComponent*>& out_allComponents, bool bOnlyActive)
{
  for (auto& component : m_Components)
  {
    if (!bOnlyActive || component->IsActive())
    {
      out_allComponents.PushBack(component.Borrow());
    }
  }
}

template <typename ComponentType>
plComponent* plSettingsComponentManager<ComponentType>::CreateComponentStorage()
{
  if (!m_Components.IsEmpty())
  {
    plLog::Warning("A component of type '{0}' is already present in this world. Having more than one is not allowed.", plGetStaticRTTI<ComponentType>()->GetTypeName());
  }

  m_Components.PushBack(PL_NEW(GetAllocator(), ComponentType));
  return m_Components.PeekBack().Borrow();
}

template <typename ComponentType>
void plSettingsComponentManager<ComponentType>::DeleteComponentStorage(plComponent* pComponent, plComponent*& out_pMovedComponent)
{
  out_pMovedComponent = pComponent;

  for (plUInt32 i = 0; i < m_Components.GetCount(); ++i)
  {
    if (m_Components[i].Borrow() == pComponent)
    {
      m_Components.RemoveAtAndCopy(i);
      break;
    }
  }
}
