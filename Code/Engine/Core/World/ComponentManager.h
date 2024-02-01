#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Containers/IdTable.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Memory/BlockStorage.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/Delegate.h>

#include <Core/World/Component.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>

/// \brief Base class for all component managers. Do not derive directly from this class, but derive from plComponentManager instead.
///
/// Every component type has its corresponding manager type. The manager stores the components in memory blocks to minimize overhead
/// on creation and deletion of components. Each manager can also register update functions to update its components during
/// the different update phases of plWorld.
/// Use plWorld::CreateComponentManager to create an instance of a component manager within a specific world.
class PL_CORE_DLL plComponentManagerBase : public plWorldModule
{
  PL_ADD_DYNAMIC_REFLECTION(plComponentManagerBase, plWorldModule);

protected:
  plComponentManagerBase(plWorld* pWorld);
  virtual ~plComponentManagerBase();

public:
  /// \brief Checks whether the given handle references a valid component.
  bool IsValidComponent(const plComponentHandle& hComponent) const;

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const plComponentHandle& hComponent, plComponent*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const plComponentHandle& hComponent, const plComponent*& out_pComponent) const;

  /// \brief Returns the number of components managed by this manager.
  plUInt32 GetComponentCount() const;

  /// \brief Create a new component instance and returns a handle to it.
  plComponentHandle CreateComponent(plGameObject* pOwnerObject);

  /// \brief Create a new component instance and returns a handle to it.
  template <typename ComponentType>
  plComponentHandle CreateComponent(plGameObject* pOwnerObject, ComponentType*& out_pComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(const plComponentHandle& hComponent);

  /// \brief Deletes the given component. Note that the component will be invalidated first and the actual deletion is postponed.
  void DeleteComponent(plComponent* pComponent);

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a plComponentManagerBase pointer.
  virtual void CollectAllComponents(plDynamicArray<plComponentHandle>& out_allComponents, bool bOnlyActive) = 0;

  /// \brief Adds all components that this manager handles to the given array (array is not cleared).
  /// Prefer to use more efficient methods on derived classes, only use this if you need to go through a plComponentManagerBase pointer.
  virtual void CollectAllComponents(plDynamicArray<plComponent*>& out_allComponents, bool bOnlyActive) = 0;

protected:
  /// \cond
  // internal methods
  friend class plWorld;
  friend class plInternal::WorldData;

  virtual void Deinitialize() override;

protected:
  friend class plWorldReader;

  plComponentHandle CreateComponentNoInit(plGameObject* pOwnerObject, plComponent*& out_pComponent);
  void InitializeComponent(plComponent* pComponent);
  void DeinitializeComponent(plComponent* pComponent);
  void PatchIdTable(plComponent* pComponent);

  virtual plComponent* CreateComponentStorage() = 0;
  virtual void DeleteComponentStorage(plComponent* pComponent, plComponent*& out_pMovedComponent) = 0;

  /// \endcond

  plIdTable<plComponentId, plComponent*> m_Components;
};

template <typename T, plBlockStorageType::Enum StorageType>
class plComponentManager : public plComponentManagerBase
{
public:
  using ComponentType = T;
  using SUPER = plComponentManagerBase;

  /// \brief Although the constructor is public always use plWorld::CreateComponentManager to create an instance.
  plComponentManager(plWorld* pWorld);
  virtual ~plComponentManager();

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const plComponentHandle& hComponent, ComponentType*& out_pComponent);

  /// \brief Returns if a component with the given handle exists and if so writes out the corresponding pointer to out_pComponent.
  bool TryGetComponent(const plComponentHandle& hComponent, const ComponentType*& out_pComponent) const;

  /// \brief Returns an iterator over all components.
  typename plBlockStorage<ComponentType, plInternal::DEFAULT_BLOCK_SIZE, StorageType>::Iterator GetComponents(plUInt32 uiStartIndex = 0);

  /// \brief Returns an iterator over all components.
  typename plBlockStorage<ComponentType, plInternal::DEFAULT_BLOCK_SIZE, StorageType>::ConstIterator GetComponents(plUInt32 uiStartIndex = 0) const;

  /// \brief Returns the type id corresponding to the component type managed by this manager.
  static plWorldModuleTypeId TypeId();

  virtual void CollectAllComponents(plDynamicArray<plComponentHandle>& out_allComponents, bool bOnlyActive) override;
  virtual void CollectAllComponents(plDynamicArray<plComponent*>& out_allComponents, bool bOnlyActive) override;

protected:
  friend ComponentType;
  friend class plComponentManagerFactory;

  virtual plComponent* CreateComponentStorage() override;
  virtual void DeleteComponentStorage(plComponent* pComponent, plComponent*& out_pMovedComponent) override;

  void RegisterUpdateFunction(UpdateFunctionDesc& desc);

  plBlockStorage<ComponentType, plInternal::DEFAULT_BLOCK_SIZE, StorageType> m_ComponentStorage;
};


//////////////////////////////////////////////////////////////////////////

struct plComponentUpdateType
{
  enum Enum
  {
    Always,
    WhenSimulating
  };
};

/// \brief Simple component manager implementation that calls an update method on all components every frame.
template <typename ComponentType, plComponentUpdateType::Enum UpdateType, plBlockStorageType::Enum StorageType = plBlockStorageType::FreeList>
class plComponentManagerSimple final : public plComponentManager<ComponentType, StorageType>
{
public:
  plComponentManagerSimple(plWorld* pWorld);

  virtual void Initialize() override;

  /// \brief A simple update function that iterates over all components and calls Update() on every component
  void SimpleUpdate(const plWorldModule::UpdateContext& context);

private:
  static void SimpleUpdateName(plStringBuilder& out_sName);
};

//////////////////////////////////////////////////////////////////////////

#define PL_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType)                        \
public:                                                                                             \
  using ComponentManagerType = managerType;                                                         \
  virtual plWorldModuleTypeId GetTypeId() const override { return s_TypeId; }                       \
  static PL_ALWAYS_INLINE plWorldModuleTypeId TypeId() { return s_TypeId; }                         \
  virtual plComponentMode::Enum GetMode() const override;                                           \
  static plComponentHandle CreateComponent(plGameObject* pOwnerObject, componentType*& pComponent); \
  static void DeleteComponent(componentType* pComponent);                                           \
  void DeleteComponent();                                                                           \
                                                                                                    \
private:                                                                                            \
  friend managerType;                                                                               \
  static plWorldModuleTypeId s_TypeId

#define PL_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType)                     \
public:                                                                                      \
  virtual plWorldModuleTypeId GetTypeId() const override { return plWorldModuleTypeId(-1); } \
  static PL_ALWAYS_INLINE plWorldModuleTypeId TypeId() { return plWorldModuleTypeId(-1); }

/// \brief Add this macro to a custom component type inside the type declaration.
#define PL_DECLARE_COMPONENT_TYPE(componentType, baseType, managerType) \
  PL_ADD_DYNAMIC_REFLECTION(componentType, baseType);                   \
  PL_ADD_COMPONENT_FUNCTIONALITY(componentType, baseType, managerType);

/// \brief Add this macro to a custom abstract component type inside the type declaration.
#define PL_DECLARE_ABSTRACT_COMPONENT_TYPE(componentType, baseType) \
  PL_ADD_DYNAMIC_REFLECTION(componentType, baseType);               \
  PL_ADD_ABSTRACT_COMPONENT_FUNCTIONALITY(componentType, baseType);


/// \brief Implements rtti and component specific functionality. Add this macro to a cpp file.
///
/// \see PL_BEGIN_DYNAMIC_REFLECTED_TYPE
#define PL_BEGIN_COMPONENT_TYPE(componentType, version, mode)                                                                                  \
  plWorldModuleTypeId componentType::s_TypeId =                                                                                                \
    plWorldModuleFactory::GetInstance()->RegisterWorldModule<typename componentType::ComponentManagerType, componentType>();                   \
  plComponentMode::Enum componentType::GetMode() const { return mode; }                                                                        \
  plComponentHandle componentType::CreateComponent(plGameObject* pOwnerObject, componentType*& out_pComponent)                                 \
  {                                                                                                                                            \
    return pOwnerObject->GetWorld()->GetOrCreateComponentManager<ComponentManagerType>()->CreateComponent(pOwnerObject, out_pComponent);       \
  }                                                                                                                                            \
  void componentType::DeleteComponent(componentType* pComponent) { pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle()); } \
  void componentType::DeleteComponent() { GetOwningManager()->DeleteComponent(GetHandle()); }                                                  \
  PL_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, plRTTINoAllocator)

/// \brief Implements rtti and abstract component specific functionality. Add this macro to a cpp file.
///
/// \see PL_BEGIN_DYNAMIC_REFLECTED_TYPE
#define PL_BEGIN_ABSTRACT_COMPONENT_TYPE(componentType, version)             \
  PL_BEGIN_DYNAMIC_REFLECTED_TYPE(componentType, version, plRTTINoAllocator) \
    flags.Add(plTypeFlags::Abstract);

/// \brief Ends the component implementation code block that was opened with PL_BEGIN_COMPONENT_TYPE.
#define PL_END_COMPONENT_TYPE PL_END_DYNAMIC_REFLECTED_TYPE
#define PL_END_ABSTRACT_COMPONENT_TYPE PL_END_DYNAMIC_REFLECTED_TYPE

#include <Core/World/Implementation/ComponentManager_inl.h>
