#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/WorldRttiConverterContext.h>

void plWorldRttiConverterContext::Clear()
{
  plRttiConverterContext::Clear();

  m_pWorld = nullptr;
  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();

  m_OtherPickingMap.Clear();
  m_ComponentPickingMap.Clear();

  m_UnknownTypes.Clear();
}

void plWorldRttiConverterContext::DeleteExistingObjects()
{
  if (m_pWorld == nullptr)
    return;

  m_UnknownTypes.Clear();

  PLASMA_LOCK(m_pWorld->GetWriteMarker());

  const auto& map = m_GameObjectMap.GetHandleToGuidMap();
  while (!map.IsEmpty())
  {
    auto it = map.GetIterator();

    plGameObject* pGameObject = nullptr;
    if (m_pWorld->TryGetObject(it.Key(), pGameObject))
    {
      DeleteObject(it.Value());
    }
    else
    {
      m_GameObjectMap.UnregisterObject(it.Value());
    }
  }

  // call base class clear, not the overridden one
  plRttiConverterContext::Clear();

  m_GameObjectMap.Clear();
  m_ComponentMap.Clear();
  m_ComponentPickingMap.Clear();
  // Need to do this to make sure all deleted objects are actually deleted as singleton components are
  // still considered alive until Update actually deletes them.
  const bool bSim = m_pWorld->GetWorldSimulationEnabled();
  m_pWorld->SetWorldSimulationEnabled(false);
  m_pWorld->Update();
  m_pWorld->SetWorldSimulationEnabled(bSim);
  // m_OtherPickingMap.Clear(); // do not clear this
}

plInternal::NewInstance<void> plWorldRttiConverterContext::CreateObject(const plUuid& guid, const plRTTI* pRtti)
{
  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Object type is unknown");

  if (pRtti == plGetStaticRTTI<plGameObject>())
  {
    plStringBuilder tmp;

    plGameObjectDesc d;
    d.m_sName.Assign(plConversionUtils::ToString(guid, tmp).GetData());
    d.m_uiStableRandomSeed = plHashingUtils::xxHash32(tmp.GetData(), tmp.GetElementCount());

    plGameObjectHandle hObject = m_pWorld->CreateObject(d);
    plGameObject* pObject;
    if (m_pWorld->TryGetObject(hObject, pObject))
    {
      RegisterObject(guid, pRtti, pObject);

      Event e;
      e.m_Type = Event::Type::GameObjectCreated;
      e.m_ObjectGuid = guid;
      m_Events.Broadcast(e);

      return {pObject, nullptr};
    }
    else
    {
      plLog::Error("Failed to create plGameObject!");
      return nullptr;
    }
  }
  else if (pRtti->IsDerivedFrom<plComponent>())
  {
    plComponentManagerBase* pMan = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      plLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return nullptr;
    }

    // Component is added via reflection shortly so passing a nullptr as owner is fine here.
    plComponentHandle hComponent = pMan->CreateComponent(nullptr);
    plComponent* pComponent;
    if (pMan->TryGetComponent(hComponent, pComponent))
    {
      RegisterObject(guid, pRtti, pComponent);
      return {pComponent, nullptr};
    }
    else
    {
      plLog::Error("Component of type '{0}' cannot be found after creation", pRtti->GetTypeName());
      return nullptr;
    }
  }
  else
  {
    return plRttiConverterContext::CreateObject(guid, pRtti);
  }
}

void plWorldRttiConverterContext::DeleteObject(const plUuid& guid)
{
  plRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when manipulating scenes during simulation
  // and when creating two components of a type that acts like a singleton (and therefore ignores the second instance creation)
  if (object.m_pObject == nullptr)
    return;

  const plRTTI* pRtti = object.m_pType;
  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Object does not exist!");

  if (pRtti == plGetStaticRTTI<plGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    UnregisterObject(guid);
    m_pWorld->DeleteObjectNow(hObject, false);

    Event e;
    e.m_Type = Event::Type::GameObjectDeleted;
    e.m_ObjectGuid = guid;
    m_Events.Broadcast(e);
  }
  else if (pRtti->IsDerivedFrom<plComponent>())
  {
    plComponentHandle hComponent = m_ComponentMap.GetHandle(guid);
    plComponentManagerBase* pMan = m_pWorld->GetOrCreateManagerForComponentType(pRtti);
    if (pMan == nullptr)
    {
      plLog::Error("Component of type '{0}' cannot be created, no component manager is registered", pRtti->GetTypeName());
      return;
    }

    UnregisterObject(guid);
    pMan->DeleteComponent(hComponent);
  }
  else
  {
    plRttiConverterContext::DeleteObject(guid);
  }
}

void plWorldRttiConverterContext::RegisterObject(const plUuid& guid, const plRTTI* pRtti, void* pObject)
{
  if (pRtti == plGetStaticRTTI<plGameObject>())
  {
    plGameObject* pGameObject = static_cast<plGameObject*>(pObject);
    m_GameObjectMap.RegisterObject(guid, pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<plComponent>())
  {
    plComponent* pComponent = static_cast<plComponent*>(pObject);

    PLASMA_ASSERT_DEV(m_pWorld != nullptr && pComponent->GetWorld() == m_pWorld, "Invalid object to register");

    m_ComponentMap.RegisterObject(guid, pComponent->GetHandle());
    pComponent->SetUniqueID(m_uiNextComponentPickingID++);
    m_ComponentPickingMap.RegisterObject(guid, pComponent->GetUniqueID());
  }

  plRttiConverterContext::RegisterObject(guid, pRtti, pObject);
}

void plWorldRttiConverterContext::UnregisterObject(const plUuid& guid)
{
  plRttiConverterObject object = GetObjectByGUID(guid);

  // this can happen when running a game simulation and the object is destroyed by the game code
  // PLASMA_ASSERT_DEBUG(object.m_pObject, "Failed to retrieve object by guid!");

  if (object.m_pType != nullptr)
  {
    const plRTTI* pRtti = object.m_pType;
    if (pRtti == plGetStaticRTTI<plGameObject>())
    {
      m_GameObjectMap.UnregisterObject(guid);
    }
    else if (pRtti->IsDerivedFrom<plComponent>())
    {
      m_ComponentMap.UnregisterObject(guid);
      m_ComponentPickingMap.UnregisterObject(guid);
    }
  }

  plRttiConverterContext::UnregisterObject(guid);
}

plRttiConverterObject plWorldRttiConverterContext::GetObjectByGUID(const plUuid& guid) const
{
  plRttiConverterObject object = plRttiConverterContext::GetObjectByGUID(guid);

  if (!guid.IsValid() || object.m_pType == nullptr)
    return object;

  // We can't look up the ptr via the base class map as it keeps changing, we we need to use the handle.
  if (object.m_pType == plGetStaticRTTI<plGameObject>())
  {
    auto hObject = m_GameObjectMap.GetHandle(guid);
    plGameObject* pGameObject = nullptr;
    if (!m_pWorld->TryGetObject(hObject, pGameObject))
    {
      object.m_pObject = nullptr;
      object.m_pType = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // PLASMA_REPORT_FAILURE("Can't resolve game object GUID!");
      return object;
    }

    // Update new ptr of game object
    if (object.m_pObject != pGameObject)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pGameObject;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  else if (object.m_pType->IsDerivedFrom<plComponent>())
  {
    auto hComponent = m_ComponentMap.GetHandle(guid);
    plComponent* pComponent = nullptr;
    if (!m_pWorld->TryGetComponent(hComponent, pComponent))
    {
      object.m_pObject = nullptr;
      object.m_pType = nullptr;
      // this can happen when one manipulates a running scene, and an object just deleted itself
      // PLASMA_REPORT_FAILURE("Can't resolve component GUID!");
      return object;
    }

    // Update new ptr of component
    if (object.m_pObject != pComponent)
    {
      m_ObjectToGuid.Remove(object.m_pObject);
      object.m_pObject = pComponent;
      m_ObjectToGuid.Insert(object.m_pObject, guid);
    }
  }
  return object;
}

plUuid plWorldRttiConverterContext::GetObjectGUID(const plRTTI* pRtti, const void* pObject) const
{
  if (pRtti == plGetStaticRTTI<plGameObject>())
  {
    const plGameObject* pGameObject = static_cast<const plGameObject*>(pObject);
    return m_GameObjectMap.GetGuid(pGameObject->GetHandle());
  }
  else if (pRtti->IsDerivedFrom<plComponent>())
  {
    const plComponent* pComponent = static_cast<const plComponent*>(pObject);
    return m_ComponentMap.GetGuid(pComponent->GetHandle());
  }
  return plRttiConverterContext::GetObjectGUID(pRtti, pObject);
}

void plWorldRttiConverterContext::OnUnknownTypeError(plStringView sTypeName)
{
  plRttiConverterContext::OnUnknownTypeError(sTypeName);

  m_UnknownTypes.Insert(sTypeName);
}
