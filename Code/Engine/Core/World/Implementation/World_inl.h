
PLASMA_ALWAYS_INLINE plStringView plWorld::GetName() const
{
  return m_Data.m_sName;
}

PLASMA_ALWAYS_INLINE plUInt32 plWorld::GetIndex() const
{
  return m_uiIndex;
}

PLASMA_FORCE_INLINE plGameObjectHandle plWorld::CreateObject(const plGameObjectDesc& desc)
{
  plGameObject* pNewObject;
  return CreateObject(desc, pNewObject);
}

PLASMA_ALWAYS_INLINE const plEvent<const plGameObject*>& plWorld::GetObjectDeletionEvent() const
{
  return m_Data.m_ObjectDeletionEvent;
}

PLASMA_FORCE_INLINE bool plWorld::IsValidObject(const plGameObjectHandle& object) const
{
  CheckForReadAccess();
  PLASMA_ASSERT_DEV(object.IsInvalidated() || object.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, object.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.Contains(object);
}

PLASMA_FORCE_INLINE bool plWorld::TryGetObject(const plGameObjectHandle& object, plGameObject*& out_pObject)
{
  CheckForReadAccess();
  PLASMA_ASSERT_DEV(object.IsInvalidated() || object.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, object.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.TryGetValue(object, out_pObject);
}

PLASMA_FORCE_INLINE bool plWorld::TryGetObject(const plGameObjectHandle& object, const plGameObject*& out_pObject) const
{
  CheckForReadAccess();
  PLASMA_ASSERT_DEV(object.IsInvalidated() || object.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, object.m_InternalId.m_WorldIndex);

  plGameObject* pObject = nullptr;
  bool bResult = m_Data.m_Objects.TryGetValue(object, pObject);
  out_pObject = pObject;
  return bResult;
}

PLASMA_FORCE_INLINE bool plWorld::TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, plGameObject*& out_pObject)
{
  CheckForReadAccess();
  plGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

PLASMA_FORCE_INLINE bool plWorld::TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, const plGameObject*& out_pObject) const
{
  CheckForReadAccess();
  plGameObjectId id;
  if (m_Data.m_GlobalKeyToIdTable.TryGetValue(sGlobalKey.GetHash(), id))
  {
    out_pObject = m_Data.m_Objects[id];
    return true;
  }

  return false;
}

PLASMA_FORCE_INLINE plUInt32 plWorld::GetObjectCount() const
{
  CheckForReadAccess();
  // Subtract one to exclude dummy object with instance index 0
  return static_cast<plUInt32>(m_Data.m_Objects.GetCount() - 1);
}

PLASMA_FORCE_INLINE plInternal::WorldData::ObjectIterator plWorld::GetObjects()
{
  CheckForWriteAccess();
  return plInternal::WorldData::ObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

PLASMA_FORCE_INLINE plInternal::WorldData::ConstObjectIterator plWorld::GetObjects() const
{
  CheckForReadAccess();
  return plInternal::WorldData::ConstObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

PLASMA_FORCE_INLINE void plWorld::Traverse(VisitorFunc visitorFunc, TraversalMethod method /*= DepthFirst*/)
{
  CheckForWriteAccess();

  if (method == DepthFirst)
  {
    m_Data.TraverseDepthFirst(visitorFunc);
  }
  else // method == BreadthFirst
  {
    m_Data.TraverseBreadthFirst(visitorFunc);
  }
}

template <typename ModuleType>
PLASMA_ALWAYS_INLINE ModuleType* plWorld::GetOrCreateModule()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<ModuleType*>(GetOrCreateModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PLASMA_ALWAYS_INLINE void plWorld::DeleteModule()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  DeleteModule(plGetStaticRTTI<ModuleType>());
}

template <typename ModuleType>
PLASMA_ALWAYS_INLINE ModuleType* plWorld::GetModule()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<ModuleType*>(GetModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PLASMA_ALWAYS_INLINE const ModuleType* plWorld::GetModule() const
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<const ModuleType*>(GetModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PLASMA_ALWAYS_INLINE const ModuleType* plWorld::GetModuleReadOnly() const
{
  return GetModule<ModuleType>();
}

template <typename ManagerType>
ManagerType* plWorld::GetOrCreateComponentManager()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  if (pModule == nullptr)
  {
    pModule = PLASMA_NEW(&m_Data.m_Allocator, ManagerType, this);
    static_cast<plWorldModule*>(pModule)->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

PLASMA_ALWAYS_INLINE plComponentManagerBase* plWorld::GetOrCreateManagerForComponentType(const plRTTI* pComponentRtti)
{
  PLASMA_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '%s'", pComponentRtti->GetTypeName());

  return plStaticCast<plComponentManagerBase*>(GetOrCreateModule(pComponentRtti));
}

template <typename ManagerType>
void plWorld::DeleteComponentManager()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]))
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      static_cast<plWorldModule*>(pModule)->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      PLASMA_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

template <typename ManagerType>
PLASMA_FORCE_INLINE ManagerType* plWorld::GetComponentManager()
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return plStaticCast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

template <typename ManagerType>
PLASMA_FORCE_INLINE const ManagerType* plWorld::GetComponentManager() const
{
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForReadAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return plStaticCast<const ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

PLASMA_ALWAYS_INLINE plComponentManagerBase* plWorld::GetManagerForComponentType(const plRTTI* pComponentRtti)
{
  PLASMA_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return plStaticCast<plComponentManagerBase*>(GetModule(pComponentRtti));
}

PLASMA_ALWAYS_INLINE const plComponentManagerBase* plWorld::GetManagerForComponentType(const plRTTI* pComponentRtti) const
{
  PLASMA_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return plStaticCast<const plComponentManagerBase*>(GetModule(pComponentRtti));
}

inline bool plWorld::IsValidComponent(const plComponentHandle& component) const
{
  CheckForReadAccess();
  const plWorldModuleTypeId uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      return static_cast<const plComponentManagerBase*>(pModule)->IsValidComponent(component);
    }
  }

  return false;
}

template <typename ComponentType>
inline bool plWorld::TryGetComponent(const plComponentHandle& component, ComponentType*& out_pComponent)
{
  CheckForWriteAccess();
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponent, ComponentType), "Not a valid component type");

  const plWorldModuleTypeId uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      plComponent* pComponent = nullptr;
      bool bResult = static_cast<plComponentManagerBase*>(pModule)->TryGetComponent(component, pComponent);
      out_pComponent = plDynamicCast<ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

template <typename ComponentType>
inline bool plWorld::TryGetComponent(const plComponentHandle& component, const ComponentType*& out_pComponent) const
{
  CheckForReadAccess();
  PLASMA_CHECK_AT_COMPILETIME_MSG(PLASMA_IS_DERIVED_FROM_STATIC(plComponent, ComponentType), "Not a valid component type");

  const plWorldModuleTypeId uiTypeId = component.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      const plComponent* pComponent = nullptr;
      bool bResult = static_cast<const plComponentManagerBase*>(pModule)->TryGetComponent(component, pComponent);
      out_pComponent = plDynamicCast<const ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

PLASMA_FORCE_INLINE void plWorld::SendMessage(const plGameObjectHandle& receiverObject, plMessage& msg)
{
  CheckForWriteAccess();

  plGameObject* pReceiverObject = nullptr;
  if (TryGetObject(receiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessage(msg);
  }
  else
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessage: The receiver plGameObject for message of type '{0}' does not exist.", msg.GetId());
    }
#endif
  }
}

PLASMA_FORCE_INLINE void plWorld::SendMessageRecursive(const plGameObjectHandle& receiverObject, plMessage& msg)
{
  CheckForWriteAccess();

  plGameObject* pReceiverObject = nullptr;
  if (TryGetObject(receiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessageRecursive(msg);
  }
  else
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessageRecursive: The receiver plGameObject for message of type '{0}' does not exist.", msg.GetId());
    }
#endif
  }
}

PLASMA_ALWAYS_INLINE void plWorld::PostMessage(
  const plGameObjectHandle& receiverObject, const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(receiverObject, msg, queueType, delay, false);
}

PLASMA_ALWAYS_INLINE void plWorld::PostMessageRecursive(
  const plGameObjectHandle& receiverObject, const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(receiverObject, msg, queueType, delay, true);
}

PLASMA_FORCE_INLINE void plWorld::SendMessage(const plComponentHandle& receiverComponent, plMessage& msg)
{
  CheckForWriteAccess();

  plComponent* pReceiverComponent = nullptr;
  if (TryGetComponent(receiverComponent, pReceiverComponent))
  {
    pReceiverComponent->SendMessage(msg);
  }
  else
  {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
    if (msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessage: The receiver plComponent for message of type '{0}' does not exist.", msg.GetId());
    }
#endif
  }
}

PLASMA_ALWAYS_INLINE void plWorld::SetWorldSimulationEnabled(bool bEnable)
{
  m_Data.m_bSimulateWorld = bEnable;
}

PLASMA_ALWAYS_INLINE bool plWorld::GetWorldSimulationEnabled() const
{
  return m_Data.m_bSimulateWorld;
}

PLASMA_ALWAYS_INLINE const plSharedPtr<plTask>& plWorld::GetUpdateTask()
{
  return m_pUpdateTask;
}

PLASMA_ALWAYS_INLINE plUInt32 plWorld::GetUpdateCounter() const
{
  return m_Data.m_uiUpdateCounter;
}

PLASMA_FORCE_INLINE plSpatialSystem* plWorld::GetSpatialSystem()
{
  CheckForWriteAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

PLASMA_FORCE_INLINE const plSpatialSystem* plWorld::GetSpatialSystem() const
{
  CheckForReadAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

PLASMA_ALWAYS_INLINE void plWorld::GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_CoordinateSystem) const
{
  m_Data.m_pCoordinateSystemProvider->GetCoordinateSystem(vGlobalPosition, out_CoordinateSystem);
}

PLASMA_ALWAYS_INLINE plCoordinateSystemProvider& plWorld::GetCoordinateSystemProvider()
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

PLASMA_ALWAYS_INLINE const plCoordinateSystemProvider& plWorld::GetCoordinateSystemProvider() const
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

PLASMA_ALWAYS_INLINE plClock& plWorld::GetClock()
{
  return m_Data.m_Clock;
}

PLASMA_ALWAYS_INLINE const plClock& plWorld::GetClock() const
{
  return m_Data.m_Clock;
}

PLASMA_ALWAYS_INLINE plRandom& plWorld::GetRandomNumberGenerator()
{
  return m_Data.m_Random;
}

PLASMA_ALWAYS_INLINE plAllocatorBase* plWorld::GetAllocator()
{
  return &m_Data.m_Allocator;
}

PLASMA_ALWAYS_INLINE plInternal::WorldLargeBlockAllocator* plWorld::GetBlockAllocator()
{
  return &m_Data.m_BlockAllocator;
}

PLASMA_ALWAYS_INLINE plDoubleBufferedStackAllocator* plWorld::GetStackAllocator()
{
  return &m_Data.m_StackAllocator;
}

PLASMA_ALWAYS_INLINE plInternal::WorldData::ReadMarker& plWorld::GetReadMarker() const
{
  return m_Data.m_ReadMarker;
}

PLASMA_ALWAYS_INLINE plInternal::WorldData::WriteMarker& plWorld::GetWriteMarker()
{
  return m_Data.m_WriteMarker;
}

PLASMA_FORCE_INLINE void plWorld::SetUserData(void* pUserData)
{
  CheckForWriteAccess();

  m_Data.m_pUserData = pUserData;
}

PLASMA_FORCE_INLINE void* plWorld::GetUserData() const
{
  CheckForReadAccess();

  return m_Data.m_pUserData;
}

constexpr plUInt64 plWorld::GetMaxNumGameObjects()
{
  return plGameObjectId::MAX_INSTANCES - 2;
}

constexpr plUInt64 plWorld::GetMaxNumHierarchyLevels()
{
  return 1 << (sizeof(plGameObject::m_uiHierarchyLevel) * 8);
}

constexpr plUInt64 plWorld::GetMaxNumComponentsPerType()
{
  return plComponentId::MAX_INSTANCES - 1;
}

constexpr plUInt64 plWorld::GetMaxNumWorldModules()
{
  return PLASMA_MAX_WORLD_MODULE_TYPES;
}

constexpr plUInt64 plWorld::GetMaxNumComponentTypes()
{
  return PLASMA_MAX_COMPONENT_TYPES;
}

constexpr plUInt64 plWorld::GetMaxNumWorlds()
{
  return PLASMA_MAX_WORLDS;
}

// static
PLASMA_ALWAYS_INLINE plUInt32 plWorld::GetWorldCount()
{
  return s_Worlds.GetCount();
}

// static
PLASMA_ALWAYS_INLINE plWorld* plWorld::GetWorld(plUInt32 uiIndex)
{
  return s_Worlds[uiIndex];
}

// static
PLASMA_ALWAYS_INLINE plWorld* plWorld::GetWorld(const plGameObjectHandle& object)
{
  return s_Worlds[object.GetInternalID().m_WorldIndex];
}

// static
PLASMA_ALWAYS_INLINE plWorld* plWorld::GetWorld(const plComponentHandle& component)
{
  return s_Worlds[component.GetInternalID().m_WorldIndex];
}

PLASMA_ALWAYS_INLINE void plWorld::CheckForReadAccess() const
{
  PLASMA_ASSERT_DEV(m_Data.m_iReadCounter > 0, "Trying to read from World '{0}', but it is not marked for reading.", GetName());
}

PLASMA_ALWAYS_INLINE void plWorld::CheckForWriteAccess() const
{
  PLASMA_ASSERT_DEV(
    m_Data.m_WriteThreadID == plThreadUtils::GetCurrentThreadID(), "Trying to write to World '{0}', but it is not marked for writing.", GetName());
}

PLASMA_ALWAYS_INLINE plGameObject* plWorld::GetObjectUnchecked(plUInt32 uiIndex) const
{
  return m_Data.m_Objects.GetValueUnchecked(uiIndex);
}

PLASMA_ALWAYS_INLINE bool plWorld::ReportErrorWhenStaticObjectMoves() const
{
  return m_Data.m_bReportErrorWhenStaticObjectMoves;
}

PLASMA_ALWAYS_INLINE float plWorld::GetInvDeltaSeconds() const
{
  const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
  if (fDelta > 0.0f)
  {
    return 1.0f / fDelta;
  }

  // when the clock is paused just use zero
  return 0.0f;
}
