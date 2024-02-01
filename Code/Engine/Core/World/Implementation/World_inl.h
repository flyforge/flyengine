
PL_ALWAYS_INLINE plStringView plWorld::GetName() const
{
  return m_Data.m_sName;
}

PL_ALWAYS_INLINE plUInt32 plWorld::GetIndex() const
{
  return m_uiIndex;
}

PL_FORCE_INLINE plGameObjectHandle plWorld::CreateObject(const plGameObjectDesc& desc)
{
  plGameObject* pNewObject;
  return CreateObject(desc, pNewObject);
}

PL_ALWAYS_INLINE const plEvent<const plGameObject*>& plWorld::GetObjectDeletionEvent() const
{
  return m_Data.m_ObjectDeletionEvent;
}

PL_FORCE_INLINE bool plWorld::IsValidObject(const plGameObjectHandle& hObject) const
{
  CheckForReadAccess();
  PL_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.Contains(hObject);
}

PL_FORCE_INLINE bool plWorld::TryGetObject(const plGameObjectHandle& hObject, plGameObject*& out_pObject)
{
  CheckForReadAccess();
  PL_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  return m_Data.m_Objects.TryGetValue(hObject, out_pObject);
}

PL_FORCE_INLINE bool plWorld::TryGetObject(const plGameObjectHandle& hObject, const plGameObject*& out_pObject) const
{
  CheckForReadAccess();
  PL_ASSERT_DEV(hObject.IsInvalidated() || hObject.m_InternalId.m_WorldIndex == m_uiIndex,
    "Object does not belong to this world. Expected world id {0} got id {1}", m_uiIndex, hObject.m_InternalId.m_WorldIndex);

  plGameObject* pObject = nullptr;
  bool bResult = m_Data.m_Objects.TryGetValue(hObject, pObject);
  out_pObject = pObject;
  return bResult;
}

PL_FORCE_INLINE bool plWorld::TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, plGameObject*& out_pObject)
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

PL_FORCE_INLINE bool plWorld::TryGetObjectWithGlobalKey(const plTempHashedString& sGlobalKey, const plGameObject*& out_pObject) const
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

PL_FORCE_INLINE plUInt32 plWorld::GetObjectCount() const
{
  CheckForReadAccess();
  // Subtract one to exclude dummy object with instance index 0
  return static_cast<plUInt32>(m_Data.m_Objects.GetCount() - 1);
}

PL_FORCE_INLINE plInternal::WorldData::ObjectIterator plWorld::GetObjects()
{
  CheckForWriteAccess();
  return plInternal::WorldData::ObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

PL_FORCE_INLINE plInternal::WorldData::ConstObjectIterator plWorld::GetObjects() const
{
  CheckForReadAccess();
  return plInternal::WorldData::ConstObjectIterator(m_Data.m_ObjectStorage.GetIterator(0));
}

PL_FORCE_INLINE void plWorld::Traverse(VisitorFunc visitorFunc, TraversalMethod method /*= DepthFirst*/)
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
PL_ALWAYS_INLINE ModuleType* plWorld::GetOrCreateModule()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<ModuleType*>(GetOrCreateModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PL_ALWAYS_INLINE void plWorld::DeleteModule()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  DeleteModule(plGetStaticRTTI<ModuleType>());
}

template <typename ModuleType>
PL_ALWAYS_INLINE ModuleType* plWorld::GetModule()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<ModuleType*>(GetModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PL_ALWAYS_INLINE const ModuleType* plWorld::GetModule() const
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plWorldModule, ModuleType), "Not a valid module type");

  return plStaticCast<const ModuleType*>(GetModule(plGetStaticRTTI<ModuleType>()));
}

template <typename ModuleType>
PL_ALWAYS_INLINE const ModuleType* plWorld::GetModuleReadOnly() const
{
  return GetModule<ModuleType>();
}

template <typename ManagerType>
ManagerType* plWorld::GetOrCreateComponentManager()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  if (pModule == nullptr)
  {
    pModule = PL_NEW(&m_Data.m_Allocator, ManagerType, this);
    static_cast<plWorldModule*>(pModule)->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

PL_ALWAYS_INLINE plComponentManagerBase* plWorld::GetOrCreateManagerForComponentType(const plRTTI* pComponentRtti)
{
  PL_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '%s'", pComponentRtti->GetTypeName());

  return plStaticCast<plComponentManagerBase*>(GetOrCreateModule(pComponentRtti));
}

template <typename ManagerType>
void plWorld::DeleteComponentManager()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (ManagerType* pModule = static_cast<ManagerType*>(m_Data.m_Modules[uiTypeId]))
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      static_cast<plWorldModule*>(pModule)->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      PL_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

template <typename ManagerType>
PL_FORCE_INLINE ManagerType* plWorld::GetComponentManager()
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return plStaticCast<ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

template <typename ManagerType>
PL_FORCE_INLINE const ManagerType* plWorld::GetComponentManager() const
{
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponentManagerBase, ManagerType), "Not a valid component manager type");

  CheckForReadAccess();

  const plWorldModuleTypeId uiTypeId = ManagerType::TypeId();
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return plStaticCast<const ManagerType*>(m_Data.m_Modules[uiTypeId]);
  }

  return nullptr;
}

PL_ALWAYS_INLINE plComponentManagerBase* plWorld::GetManagerForComponentType(const plRTTI* pComponentRtti)
{
  PL_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return plStaticCast<plComponentManagerBase*>(GetModule(pComponentRtti));
}

PL_ALWAYS_INLINE const plComponentManagerBase* plWorld::GetManagerForComponentType(const plRTTI* pComponentRtti) const
{
  PL_ASSERT_DEV(pComponentRtti->IsDerivedFrom<plComponent>(), "Invalid component type '{0}'", pComponentRtti->GetTypeName());

  return plStaticCast<const plComponentManagerBase*>(GetModule(pComponentRtti));
}

inline bool plWorld::IsValidComponent(const plComponentHandle& hComponent) const
{
  CheckForReadAccess();
  const plWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      return static_cast<const plComponentManagerBase*>(pModule)->IsValidComponent(hComponent);
    }
  }

  return false;
}

template <typename ComponentType>
inline bool plWorld::TryGetComponent(const plComponentHandle& hComponent, ComponentType*& out_pComponent)
{
  CheckForWriteAccess();
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponent, ComponentType), "Not a valid component type");

  const plWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      plComponent* pComponent = nullptr;
      bool bResult = static_cast<plComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent = plDynamicCast<ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

template <typename ComponentType>
inline bool plWorld::TryGetComponent(const plComponentHandle& hComponent, const ComponentType*& out_pComponent) const
{
  CheckForReadAccess();
  PL_CHECK_AT_COMPILETIME_MSG(PL_IS_DERIVED_FROM_STATIC(plComponent, ComponentType), "Not a valid component type");

  const plWorldModuleTypeId uiTypeId = hComponent.m_InternalId.m_TypeId;

  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (const plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      const plComponent* pComponent = nullptr;
      bool bResult = static_cast<const plComponentManagerBase*>(pModule)->TryGetComponent(hComponent, pComponent);
      out_pComponent = plDynamicCast<const ComponentType*>(pComponent);
      return bResult && out_pComponent != nullptr;
    }
  }

  return false;
}

PL_FORCE_INLINE void plWorld::SendMessage(const plGameObjectHandle& hReceiverObject, plMessage& ref_msg)
{
  CheckForWriteAccess();

  plGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessage(ref_msg);
  }
  else
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessage: The receiver plGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

PL_FORCE_INLINE void plWorld::SendMessageRecursive(const plGameObjectHandle& hReceiverObject, plMessage& ref_msg)
{
  CheckForWriteAccess();

  plGameObject* pReceiverObject = nullptr;
  if (TryGetObject(hReceiverObject, pReceiverObject))
  {
    pReceiverObject->SendMessageRecursive(ref_msg);
  }
  else
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessageRecursive: The receiver plGameObject for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

PL_ALWAYS_INLINE void plWorld::PostMessage(
  const plGameObjectHandle& hReceiverObject, const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, false);
}

PL_ALWAYS_INLINE void plWorld::PostMessageRecursive(
  const plGameObjectHandle& hReceiverObject, const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.
  PostMessage(hReceiverObject, msg, queueType, delay, true);
}

PL_FORCE_INLINE void plWorld::SendMessage(const plComponentHandle& hReceiverComponent, plMessage& ref_msg)
{
  CheckForWriteAccess();

  plComponent* pReceiverComponent = nullptr;
  if (TryGetComponent(hReceiverComponent, pReceiverComponent))
  {
    pReceiverComponent->SendMessage(ref_msg);
  }
  else
  {
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
    if (ref_msg.GetDebugMessageRouting())
    {
      plLog::Warning("plWorld::SendMessage: The receiver plComponent for message of type '{0}' does not exist.", ref_msg.GetId());
    }
#endif
  }
}

PL_ALWAYS_INLINE void plWorld::SetWorldSimulationEnabled(bool bEnable)
{
  m_Data.m_bSimulateWorld = bEnable;
}

PL_ALWAYS_INLINE bool plWorld::GetWorldSimulationEnabled() const
{
  return m_Data.m_bSimulateWorld;
}

PL_ALWAYS_INLINE const plSharedPtr<plTask>& plWorld::GetUpdateTask()
{
  return m_pUpdateTask;
}

PL_ALWAYS_INLINE plUInt32 plWorld::GetUpdateCounter() const
{
  return m_Data.m_uiUpdateCounter;
}

PL_FORCE_INLINE plSpatialSystem* plWorld::GetSpatialSystem()
{
  CheckForWriteAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

PL_FORCE_INLINE const plSpatialSystem* plWorld::GetSpatialSystem() const
{
  CheckForReadAccess();

  return m_Data.m_pSpatialSystem.Borrow();
}

PL_ALWAYS_INLINE void plWorld::GetCoordinateSystem(const plVec3& vGlobalPosition, plCoordinateSystem& out_coordinateSystem) const
{
  m_Data.m_pCoordinateSystemProvider->GetCoordinateSystem(vGlobalPosition, out_coordinateSystem);
}

PL_ALWAYS_INLINE plCoordinateSystemProvider& plWorld::GetCoordinateSystemProvider()
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

PL_ALWAYS_INLINE const plCoordinateSystemProvider& plWorld::GetCoordinateSystemProvider() const
{
  return *(m_Data.m_pCoordinateSystemProvider.Borrow());
}

PL_ALWAYS_INLINE plClock& plWorld::GetClock()
{
  return m_Data.m_Clock;
}

PL_ALWAYS_INLINE const plClock& plWorld::GetClock() const
{
  return m_Data.m_Clock;
}

PL_ALWAYS_INLINE plRandom& plWorld::GetRandomNumberGenerator()
{
  return m_Data.m_Random;
}

PL_ALWAYS_INLINE plAllocator* plWorld::GetAllocator()
{
  return &m_Data.m_Allocator;
}

PL_ALWAYS_INLINE plInternal::WorldLargeBlockAllocator* plWorld::GetBlockAllocator()
{
  return &m_Data.m_BlockAllocator;
}

PL_ALWAYS_INLINE plDoubleBufferedLinearAllocator* plWorld::GetStackAllocator()
{
  return &m_Data.m_StackAllocator;
}

PL_ALWAYS_INLINE plInternal::WorldData::ReadMarker& plWorld::GetReadMarker() const
{
  return m_Data.m_ReadMarker;
}

PL_ALWAYS_INLINE plInternal::WorldData::WriteMarker& plWorld::GetWriteMarker()
{
  return m_Data.m_WriteMarker;
}

PL_FORCE_INLINE void plWorld::SetUserData(void* pUserData)
{
  CheckForWriteAccess();

  m_Data.m_pUserData = pUserData;
}

PL_FORCE_INLINE void* plWorld::GetUserData() const
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
  return PL_MAX_WORLD_MODULE_TYPES;
}

constexpr plUInt64 plWorld::GetMaxNumComponentTypes()
{
  return PL_MAX_COMPONENT_TYPES;
}

constexpr plUInt64 plWorld::GetMaxNumWorlds()
{
  return PL_MAX_WORLDS;
}

// static
PL_ALWAYS_INLINE plUInt32 plWorld::GetWorldCount()
{
  return s_Worlds.GetCount();
}

// static
PL_ALWAYS_INLINE plWorld* plWorld::GetWorld(plUInt32 uiIndex)
{
  return s_Worlds[uiIndex];
}

// static
PL_ALWAYS_INLINE plWorld* plWorld::GetWorld(const plGameObjectHandle& hObject)
{
  return s_Worlds[hObject.GetInternalID().m_WorldIndex];
}

// static
PL_ALWAYS_INLINE plWorld* plWorld::GetWorld(const plComponentHandle& hComponent)
{
  return s_Worlds[hComponent.GetInternalID().m_WorldIndex];
}

PL_ALWAYS_INLINE void plWorld::CheckForReadAccess() const
{
  PL_ASSERT_DEV(m_Data.m_iReadCounter > 0, "Trying to read from World '{0}', but it is not marked for reading.", GetName());
}

PL_ALWAYS_INLINE void plWorld::CheckForWriteAccess() const
{
  PL_ASSERT_DEV(
    m_Data.m_WriteThreadID == plThreadUtils::GetCurrentThreadID(), "Trying to write to World '{0}', but it is not marked for writing.", GetName());
}

PL_ALWAYS_INLINE plGameObject* plWorld::GetObjectUnchecked(plUInt32 uiIndex) const
{
  return m_Data.m_Objects.GetValueUnchecked(uiIndex);
}

PL_ALWAYS_INLINE bool plWorld::ReportErrorWhenStaticObjectMoves() const
{
  return m_Data.m_bReportErrorWhenStaticObjectMoves;
}

PL_ALWAYS_INLINE float plWorld::GetInvDeltaSeconds() const
{
  const float fDelta = (float)m_Data.m_Clock.GetTimeDiff().GetSeconds();
  if (fDelta > 0.0f)
  {
    return 1.0f / fDelta;
  }

  // when the clock is paused just use zero
  return 0.0f;
}
