#include <Core/CorePCH.h>

#include <Core/Messages/DeleteObjectMessage.h>
#include <Core/Messages/HierarchyChangedMessages.h>
#include <Core/World/EventMessageHandlerComponent.h>
#include <Core/World/World.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Utilities/Stats.h>

plStaticArray<plWorld*, plWorld::GetMaxNumWorlds()> plWorld::s_Worlds;

static plGameObjectHandle DefaultGameObjectReferenceResolver(const void* pData, plComponentHandle hThis, plStringView sProperty)
{
  const char* szRef = reinterpret_cast<const char*>(pData);

  if (plStringUtils::IsNullOrEmpty(szRef))
    return plGameObjectHandle();

  // this is a convention used by plPrefabReferenceComponent:
  // a string starting with this means a 'global game object reference', ie a reference that is valid within the current world
  // what follows is an integer that is the internal storage of an plGameObjectHandle
  // thus parsing the int and casting it to an plGameObjectHandle gives the desired result
  if (plStringUtils::StartsWith(szRef, "#!GGOR-"))
  {
    plInt64 id;
    if (plConversionUtils::StringToInt64(szRef + 7, id).Succeeded())
    {
      return plGameObjectHandle(plGameObjectId(reinterpret_cast<plUInt64&>(id)));
    }
  }

  return plGameObjectHandle();
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plWorld, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(DeleteObjectDelayed, In, "GameObject", In, "DeleteEmptyParents")->AddAttributes(
      new plFunctionArgumentAttributes(1, new plDefaultValueAttribute(true))),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_TryGetObjectWithGlobalKey, In, "GlobalKey")->AddFlags(plPropertyFlags::Const),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Reflection_GetClock)->AddFlags(plPropertyFlags::Const),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

plWorld::plWorld(plWorldDesc& ref_desc)
  : m_Data(ref_desc)
{
  m_pUpdateTask = PLASMA_DEFAULT_NEW(plDelegateTask<void>, "WorldUpdate", plTaskNesting::Never, plMakeDelegate(&plWorld::UpdateFromThread, this));
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;

  plStringBuilder sb = ref_desc.m_sName.GetString();
  sb.Append(".Update");
  m_pUpdateTask->ConfigureTask(sb, plTaskNesting::Maybe);

  m_uiIndex = plInvalidIndex;

  // find a free world slot
  const plUInt32 uiWorldCount = s_Worlds.GetCount();
  for (plUInt32 i = 0; i < uiWorldCount; i++)
  {
    if (s_Worlds[i] == nullptr)
    {
      s_Worlds[i] = this;
      m_uiIndex = i;
      break;
    }
  }

  if (m_uiIndex == plInvalidIndex)
  {
    m_uiIndex = s_Worlds.GetCount();
    PLASMA_ASSERT_DEV(m_uiIndex < GetMaxNumWorlds(), "Max world index reached: {}", GetMaxNumWorlds());
    static_assert(GetMaxNumWorlds() == PLASMA_MAX_WORLDS);

    s_Worlds.PushBack(this);
  }

  SetGameObjectReferenceResolver(DefaultGameObjectReferenceResolver);
}

plWorld::~plWorld()
{
  SetWorldSimulationEnabled(false);

  PLASMA_LOCK(GetWriteMarker());
  m_Data.Clear();

  s_Worlds[m_uiIndex] = nullptr;
  m_uiIndex = plInvalidIndex;
}


void plWorld::Clear()
{
  CheckForWriteAccess();

  while (GetObjectCount() > 0)
  {
    for (auto it = GetObjects(); it.IsValid(); ++it)
    {
      DeleteObjectNow(it->GetHandle());
    }

    if (GetObjectCount() > 0)
    {
      plLog::Dev("Remaining objects after plWorld::Clear: {}", GetObjectCount());
    }
  }

  for (plWorldModule* pModule : m_Data.m_Modules)
  {
    if (pModule != nullptr)
    {
      pModule->WorldClear();
    }
  }

  // make sure all dead objects and components are cleared right now
  DeleteDeadObjects();
  DeleteDeadComponents();

  plEventMessageHandlerComponent::ClearGlobalEventHandlersForWorld(this);
}

void plWorld::SetCoordinateSystemProvider(const plSharedPtr<plCoordinateSystemProvider>& pProvider)
{
  PLASMA_ASSERT_DEV(pProvider != nullptr, "Coordinate System Provider must not be null");

  m_Data.m_pCoordinateSystemProvider = pProvider;
  m_Data.m_pCoordinateSystemProvider->m_pOwnerWorld = this;
}

void plWorld::SetGameObjectReferenceResolver(const ReferenceResolver& resolver)
{
  m_Data.m_GameObjectReferenceResolver = resolver;
}

const plWorld::ReferenceResolver& plWorld::GetGameObjectReferenceResolver() const
{
  return m_Data.m_GameObjectReferenceResolver;
}

// a super simple, but also efficient random number generator
inline static plUInt32 NextStableRandomSeed(plUInt32& ref_uiSeed)
{
  ref_uiSeed = 214013L * ref_uiSeed + 2531011L;
  return ((ref_uiSeed >> 16) & 0x7FFFF);
}

plGameObjectHandle plWorld::CreateObject(const plGameObjectDesc& desc, plGameObject*& out_pObject)
{
  CheckForWriteAccess();

  PLASMA_ASSERT_DEV(m_Data.m_Objects.GetCount() < GetMaxNumGameObjects(), "Max number of game objects reached: {}", GetMaxNumGameObjects());

  plGameObject* pParentObject = nullptr;
  plGameObject::TransformationData* pParentData = nullptr;
  plUInt32 uiParentIndex = 0;
  plUInt64 uiHierarchyLevel = 0;
  bool bDynamic = desc.m_bDynamic;

  if (TryGetObject(desc.m_hParent, pParentObject))
  {
    pParentData = pParentObject->m_pTransformationData;
    uiParentIndex = desc.m_hParent.m_InternalId.m_InstanceIndex;
    uiHierarchyLevel = pParentObject->m_uiHierarchyLevel + 1; // if there is a parent hierarchy level is parent level + 1
    PLASMA_ASSERT_DEV(uiHierarchyLevel < GetMaxNumHierarchyLevels(), "Max hierarchy level reached: {}", GetMaxNumHierarchyLevels());
    bDynamic |= pParentObject->IsDynamic();
  }

  // get storage for the transformation data
  plGameObject::TransformationData* pTransformationData = m_Data.CreateTransformationData(bDynamic, static_cast<plUInt32>(uiHierarchyLevel));

  // get storage for the object itself
  plGameObject* pNewObject = m_Data.m_ObjectStorage.Create();

  // insert the new object into the id mapping table
  plGameObjectId newId = m_Data.m_Objects.Insert(pNewObject);
  newId.m_WorldIndex = plGameObjectId::StorageType(m_uiIndex & (PLASMA_MAX_WORLDS - 1));

  // fill out some data
  pNewObject->m_InternalId = newId;
  pNewObject->m_Flags = plObjectFlags::None;
  pNewObject->m_Flags.AddOrRemove(plObjectFlags::Dynamic, bDynamic);
  pNewObject->m_Flags.AddOrRemove(plObjectFlags::ActiveFlag, desc.m_bActiveFlag);
  pNewObject->m_sName = desc.m_sName;
  pNewObject->m_uiParentIndex = uiParentIndex;
  pNewObject->m_Tags = desc.m_Tags;
  pNewObject->m_uiTeamID = desc.m_uiTeamID;

  static_assert((GetMaxNumHierarchyLevels() - 1) <= plMath::MaxValue<plUInt16>());
  pNewObject->m_uiHierarchyLevel = static_cast<plUInt16>(uiHierarchyLevel);

  // fill out the transformation data
  pTransformationData->m_pObject = pNewObject;
  pTransformationData->m_pParentData = pParentData;
  pTransformationData->m_localPosition = plSimdConversion::ToVec3(desc.m_LocalPosition);
  pTransformationData->m_localRotation = plSimdConversion::ToQuat(desc.m_LocalRotation);
  pTransformationData->m_localScaling = plSimdConversion::ToVec4(desc.m_LocalScaling.GetAsVec4(desc.m_LocalUniformScaling));
  pTransformationData->m_globalTransform = plSimdTransform::MakeIdentity();
#if PLASMA_ENABLED(PLASMA_GAMEOBJECT_VELOCITY)
  pTransformationData->m_lastGlobalTransform = plSimdTransform::MakeIdentity();
  pTransformationData->m_uiLastGlobalTransformUpdateCounter = plInvalidIndex;
#endif
  pTransformationData->m_localBounds.SetInvalid();
  pTransformationData->m_localBounds.m_BoxHalfExtents.SetW(plSimdFloat::MakeZero());
  pTransformationData->m_globalBounds = pTransformationData->m_localBounds;
  pTransformationData->m_hSpatialData.Invalidate();
  pTransformationData->m_uiSpatialDataCategoryBitmask = 0;
  pTransformationData->m_uiStableRandomSeed = desc.m_uiStableRandomSeed;

  // if seed is set to 0xFFFFFFFF, use the parent's seed to create a deterministic value for this object
  if (pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF && pTransformationData->m_pParentData != nullptr)
  {
    plUInt32 seed = pTransformationData->m_pParentData->m_uiStableRandomSeed + pTransformationData->m_pParentData->m_pObject->GetChildCount();

    do
    {
      pTransformationData->m_uiStableRandomSeed = NextStableRandomSeed(seed);

    } while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF);
  }

  // if the seed is zero (or there was no parent to derive the seed from), assign a random value
  while (pTransformationData->m_uiStableRandomSeed == 0 || pTransformationData->m_uiStableRandomSeed == 0xFFFFFFFF)
  {
    pTransformationData->m_uiStableRandomSeed = GetRandomNumberGenerator().UInt();
  }

  pTransformationData->UpdateGlobalTransformNonRecursive(0);

  // link the transformation data to the game object
  pNewObject->m_pTransformationData = pTransformationData;

  // fix links
  LinkToParent(pNewObject);

  pNewObject->UpdateActiveState(pParentObject == nullptr ? true : pParentObject->IsActive());

  out_pObject = pNewObject;
  return plGameObjectHandle(newId);
}

void plWorld::DeleteObjectNow(const plGameObjectHandle& hObject0, bool bAlsoDeleteEmptyParents /*= true*/)
{
  CheckForWriteAccess();

  plGameObject* pObject = nullptr;
  if (!m_Data.m_Objects.TryGetValue(hObject0, pObject))
    return;

  plGameObjectHandle hObject = hObject0;

  if (bAlsoDeleteEmptyParents)
  {
    plGameObject* pParent = pObject->GetParent();

    while (pParent)
    {
      if (pParent->GetChildCount() != 1 || pParent->GetComponents().GetCount() != 0)
        break;

      pObject = pParent;

      pParent = pParent->GetParent();
    }

    hObject = pObject->GetHandle();
  }

  // inform external systems that we are about to delete this object
  m_Data.m_ObjectDeletionEvent.Broadcast(pObject);

  // set object to inactive so components and children know that they shouldn't access the object anymore.
  pObject->m_Flags.Remove(plObjectFlags::ActiveFlag | plObjectFlags::ActiveState);

  // delete children
  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    DeleteObjectNow(it->GetHandle(), false);
  }

  // delete attached components
  while (!pObject->m_Components.IsEmpty())
  {
    plComponent* pComponent = pObject->m_Components[0];
    pComponent->GetOwningManager()->DeleteComponent(pComponent->GetHandle());
  }
  PLASMA_ASSERT_DEV(pObject->m_Components.GetCount() == 0, "Components should already be removed");

  // fix parent and siblings
  UnlinkFromParent(pObject);

  // remove from global key tables
  SetObjectGlobalKey(pObject, plHashedString());

  // invalidate (but preserve world index) and remove from id table
  pObject->m_InternalId.Invalidate();
  pObject->m_InternalId.m_WorldIndex = m_uiIndex;

  m_Data.m_DeadObjects.Insert(pObject);
  PLASMA_VERIFY(m_Data.m_Objects.Remove(hObject), "Implementation error.");
}

void plWorld::DeleteObjectDelayed(const plGameObjectHandle& hObject, bool bAlsoDeleteEmptyParents /*= true*/)
{
  plMsgDeleteGameObject msg;
  msg.m_bDeleteEmptyParents = bAlsoDeleteEmptyParents;
  PostMessage(hObject, msg, plTime::MakeZero());
}

plComponentInitBatchHandle plWorld::CreateComponentInitBatch(plStringView sBatchName, bool bMustFinishWithinOneFrame /*= true*/)
{
  auto pInitBatch = PLASMA_NEW(GetAllocator(), plInternal::WorldData::InitBatch, GetAllocator(), sBatchName, bMustFinishWithinOneFrame);
  return plComponentInitBatchHandle(m_Data.m_InitBatches.Insert(pInitBatch));
}

void plWorld::DeleteComponentInitBatch(const plComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  PLASMA_ASSERT_DEV(pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty(), "Init batch has not been completely processed");
  m_Data.m_InitBatches.Remove(hBatch.GetInternalID());
}

void plWorld::BeginAddingComponentsToInitBatch(const plComponentInitBatchHandle& hBatch)
{
  PLASMA_ASSERT_DEV(m_Data.m_pCurrentInitBatch == m_Data.m_pDefaultInitBatch, "Nested init batches are not supported");
  m_Data.m_pCurrentInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()].Borrow();
}

void plWorld::EndAddingComponentsToInitBatch(const plComponentInitBatchHandle& hBatch)
{
  PLASMA_ASSERT_DEV(m_Data.m_InitBatches[hBatch.GetInternalID()] == m_Data.m_pCurrentInitBatch, "Init batch with id {} is currently not active", hBatch.GetInternalID().m_Data);
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

void plWorld::SubmitComponentInitBatch(const plComponentInitBatchHandle& hBatch)
{
  m_Data.m_InitBatches[hBatch.GetInternalID()]->m_bIsReady = true;
  m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch;
}

bool plWorld::IsComponentInitBatchCompleted(const plComponentInitBatchHandle& hBatch, double* pCompletionFactor /*= nullptr*/)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  PLASMA_ASSERT_DEV(pInitBatch->m_bIsReady, "Batch is not submitted yet");

  if (pCompletionFactor != nullptr)
  {
    if (pInitBatch->m_ComponentsToInitialize.IsEmpty())
    {
      double fStartSimCompletion = pInitBatch->m_ComponentsToStartSimulation.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToStartSimulation / pInitBatch->m_ComponentsToStartSimulation.GetCount();
      *pCompletionFactor = fStartSimCompletion * 0.5 + 0.5;
    }
    else
    {
      double fInitCompletion = pInitBatch->m_ComponentsToInitialize.IsEmpty() ? 1.0 : (double)pInitBatch->m_uiNextComponentToInitialize / pInitBatch->m_ComponentsToInitialize.GetCount();
      *pCompletionFactor = fInitCompletion * 0.5;
    }
  }

  return pInitBatch->m_ComponentsToInitialize.IsEmpty() && pInitBatch->m_ComponentsToStartSimulation.IsEmpty();
}

void plWorld::CancelComponentInitBatch(const plComponentInitBatchHandle& hBatch)
{
  auto& pInitBatch = m_Data.m_InitBatches[hBatch.GetInternalID()];
  pInitBatch->m_ComponentsToInitialize.Clear();
  pInitBatch->m_ComponentsToStartSimulation.Clear();
}
void plWorld::PostMessage(const plGameObjectHandle& receiverObject, const plMessage& msg, plObjectMsgQueueType::Enum queueType, plTime delay, bool bRecursive) const
{
  // This method is allowed to be called from multiple threads.

  PLASMA_ASSERT_DEBUG((receiverObject.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in object id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = receiverObject.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = false;
  metaData.m_uiRecursive = bRecursive;

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = plMath::Max(delay, plTime::Milliseconds(1));
  }

  plRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    plMessage* pMsgCopy = pMsgRTTIAllocator->Clone<plMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    plMessage* pMsgCopy = pMsgRTTIAllocator->Clone<plMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void plWorld::PostMessage(const plComponentHandle& hReceiverComponent, const plMessage& msg, plTime delay, plObjectMsgQueueType::Enum queueType) const
{
  // This method is allowed to be called from multiple threads.

  PLASMA_ASSERT_DEBUG((hReceiverComponent.m_InternalId.m_Data >> 62) == 0, "Upper 2 bits in component id must not be set");

  QueuedMsgMetaData metaData;
  metaData.m_uiReceiverObjectOrComponent = hReceiverComponent.m_InternalId.m_Data;
  metaData.m_uiReceiverIsComponent = true;
  metaData.m_uiRecursive = false;

  if (m_Data.m_ProcessingMessageQueue == queueType)
  {
    delay = plMath::Max(delay, plTime::Milliseconds(1));
  }

  plRTTIAllocator* pMsgRTTIAllocator = msg.GetDynamicRTTI()->GetAllocator();
  if (delay.IsPositive())
  {
    plMessage* pMsgCopy = pMsgRTTIAllocator->Clone<plMessage>(&msg, &m_Data.m_Allocator);

    metaData.m_Due = m_Data.m_Clock.GetAccumulatedTime() + delay;
    m_Data.m_TimedMessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
  else
  {
    plMessage* pMsgCopy = pMsgRTTIAllocator->Clone<plMessage>(&msg, m_Data.m_StackAllocator.GetCurrentAllocator());
    m_Data.m_MessageQueues[queueType].Enqueue(pMsgCopy, metaData);
  }
}

void plWorld::FindEventMsgHandlers(const plMessage& msg, plGameObject* pSearchObject, plDynamicArray<plComponent*>& out_components)
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void plWorld::FindEventMsgHandlers(const plMessage& msg, const plGameObject* pSearchObject, plDynamicArray<const plComponent*>& out_components) const
{
  FindEventMsgHandlers(*this, msg, pSearchObject, out_components);
}

void plWorld::Update()
{
  CheckForWriteAccess();

  PLASMA_LOG_BLOCK(m_Data.m_sName.GetData());

  {
    plStringBuilder sStatName;
    sStatName.Format("World Update/{0}/Game Object Count", m_Data.m_sName);

    plStringBuilder sStatValue;
    plStats::SetStat(sStatName, GetObjectCount());
  }

  ++m_Data.m_uiUpdateCounter;

  if (!m_Data.m_bSimulateWorld)
  {
    // only change the pause mode temporarily
    // so that user choices don't get overridden

    const bool bClockPaused = m_Data.m_Clock.GetPaused();
    m_Data.m_Clock.SetPaused(true);
    m_Data.m_Clock.Update();
    m_Data.m_Clock.SetPaused(bClockPaused);
  }
  else
  {
    m_Data.m_Clock.Update();
  }

  if (m_Data.m_pSpatialSystem != nullptr)
  {
    m_Data.m_pSpatialSystem->StartNewFrame();
  }

  // initialize phase
  {
    PLASMA_PROFILE_SCOPE("Initialize Phase");
    ProcessComponentsToInitialize();
    ProcessUpdateFunctionsToRegister();

    ProcessQueuedMessages(plObjectMsgQueueType::AfterInitialized);
  }

  // pre-async phase
  {
    PLASMA_PROFILE_SCOPE("Pre-Async Phase");
    ProcessQueuedMessages(plObjectMsgQueueType::NextFrame);
    UpdateSynchronous(m_Data.m_UpdateFunctions[plComponentManagerBase::UpdateFunctionDesc::Phase::PreAsync]);
  }

  // async phase
  {
    // remove write marker but keep the read marker. Thus no one can mark the world for writing now. Only reading is allowed in async phase.
    m_Data.m_WriteThreadID = (plThreadID)0;

    PLASMA_PROFILE_SCOPE("Async Phase");
    UpdateAsynchronous();

    // restore write marker
    m_Data.m_WriteThreadID = plThreadUtils::GetCurrentThreadID();
  }

  // post-async phase
  {
    PLASMA_PROFILE_SCOPE("Post-Async Phase");
    ProcessQueuedMessages(plObjectMsgQueueType::PostAsync);
    UpdateSynchronous(m_Data.m_UpdateFunctions[plComponentManagerBase::UpdateFunctionDesc::Phase::PostAsync]);
  }

  // delete dead objects and update the object hierarchy
  {
    PLASMA_PROFILE_SCOPE("Delete Dead Objects");
    DeleteDeadObjects();
    DeleteDeadComponents();
  }

  // update transforms
  {
    PLASMA_PROFILE_SCOPE("Update Transforms");
    m_Data.UpdateGlobalTransforms();
  }

  // post-transform phase
  {
    PLASMA_PROFILE_SCOPE("Post-Transform Phase");
    ProcessQueuedMessages(plObjectMsgQueueType::PostTransform);
    UpdateSynchronous(m_Data.m_UpdateFunctions[plComponentManagerBase::UpdateFunctionDesc::Phase::PostTransform]);
  }

  // Process again so new component can receive render messages, otherwise we introduce a frame delay.
  {
    PLASMA_PROFILE_SCOPE("Initialize Phase 2");
    // Only process the default init batch here since it contains the components created at runtime.
    // Also make sure that all initialization is finished after this call by giving it enough time.
    ProcessInitializationBatch(*m_Data.m_pDefaultInitBatch, plTime::Now() + plTime::Hours(10000));

    ProcessQueuedMessages(plObjectMsgQueueType::AfterInitialized);
  }

  // Swap our double buffered stack allocator
  m_Data.m_StackAllocator.Swap();
}

////////////////////////////////////////////////////////////////////////////////////////////////////

plWorldModule* plWorld::GetOrCreateModule(const plRTTI* pRtti)
{
  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = plWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId == 0xFFFF)
  {
    return nullptr;
  }

  m_Data.m_Modules.EnsureCount(uiTypeId + 1);

  plWorldModule* pModule = m_Data.m_Modules[uiTypeId];
  if (pModule == nullptr)
  {
    pModule = plWorldModuleFactory::GetInstance()->CreateWorldModule(uiTypeId, this);
    pModule->Initialize();

    m_Data.m_Modules[uiTypeId] = pModule;
    m_Data.m_ModulesToStartSimulation.PushBack(pModule);
  }

  return pModule;
}

void plWorld::DeleteModule(const plRTTI* pRtti)
{
  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = plWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    if (plWorldModule* pModule = m_Data.m_Modules[uiTypeId])
    {
      m_Data.m_Modules[uiTypeId] = nullptr;

      pModule->Deinitialize();
      DeregisterUpdateFunctions(pModule);
      PLASMA_DELETE(&m_Data.m_Allocator, pModule);
    }
  }
}

plWorldModule* plWorld::GetModule(const plRTTI* pRtti)
{
  CheckForWriteAccess();

  const plWorldModuleTypeId uiTypeId = plWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

const plWorldModule* plWorld::GetModule(const plRTTI* pRtti) const
{
  CheckForReadAccess();

  const plWorldModuleTypeId uiTypeId = plWorldModuleFactory::GetInstance()->GetTypeId(pRtti);
  if (uiTypeId < m_Data.m_Modules.GetCount())
  {
    return m_Data.m_Modules[uiTypeId];
  }

  return nullptr;
}

plGameObject* plWorld::Reflection_TryGetObjectWithGlobalKey(plTempHashedString sGlobalKey)
{
  plGameObject* pObject = nullptr;
  bool res = TryGetObjectWithGlobalKey(sGlobalKey, pObject);
  PLASMA_IGNORE_UNUSED(res);
  return pObject;
}

plClock* plWorld::Reflection_GetClock()
{
  return &m_Data.m_Clock;
}

void plWorld::SetParent(plGameObject* pObject, plGameObject* pNewParent, plGameObject::TransformPreservation preserve)
{
  PLASMA_ASSERT_DEV(pObject != pNewParent, "Object can't be its own parent!");
  PLASMA_ASSERT_DEV(pNewParent == nullptr || pObject->IsDynamic() || pNewParent->IsStatic(), "Can't attach a static object to a dynamic parent!");
  CheckForWriteAccess();

  if (GetObjectUnchecked(pObject->m_uiParentIndex) == pNewParent)
    return;

  UnlinkFromParent(pObject);
  // UnlinkFromParent does not clear these as they are still needed in DeleteObjectNow to allow deletes while iterating.
  pObject->m_uiNextSiblingIndex = 0;
  pObject->m_uiPrevSiblingIndex = 0;
  if (pNewParent != nullptr)
  {
    // Ensure that the parent's global transform is up-to-date otherwise the object's local transform will be wrong afterwards.
    pNewParent->UpdateGlobalTransform();

    pObject->m_uiParentIndex = pNewParent->m_InternalId.m_InstanceIndex;
    LinkToParent(pObject);
  }

  PatchHierarchyData(pObject, preserve);

  // TODO: the functions above send messages such as plMsgChildrenChanged, which will not arrive for inactive components, is that a problem ?
  // 1) if a component was active before and now gets deactivated, it may not care about the message anymore anyway
  // 2) if a component was inactive before, it did not get the message, but upon activation it can update the state for which it needed the message
  // so probably it is fine, only components that were active and stay active need the message, and that will be the case
  pObject->UpdateActiveState(pNewParent == nullptr ? true : pNewParent->IsActive());
}

void plWorld::LinkToParent(plGameObject* pObject)
{
  PLASMA_ASSERT_DEBUG(pObject->m_uiNextSiblingIndex == 0 && pObject->m_uiPrevSiblingIndex == 0, "Object is either still linked to another parent or data was not cleared.");
  if (plGameObject* pParentObject = pObject->GetParent())
  {
    const plUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (pParentObject->m_uiFirstChildIndex != 0)
    {
      pObject->m_uiPrevSiblingIndex = pParentObject->m_uiLastChildIndex;
      GetObjectUnchecked(pParentObject->m_uiLastChildIndex)->m_uiNextSiblingIndex = uiIndex;
    }
    else
    {
      pParentObject->m_uiFirstChildIndex = uiIndex;
    }

    pParentObject->m_uiLastChildIndex = uiIndex;
    pParentObject->m_uiChildCount++;

    pObject->m_pTransformationData->m_pParentData = pParentObject->m_pTransformationData;

    if (pObject->m_Flags.IsSet(plObjectFlags::ParentChangesNotifications))
    {
      plMsgParentChanged msg;
      msg.m_Type = plMsgParentChanged::Type::ParentLinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

    if (pParentObject->m_Flags.IsSet(plObjectFlags::ChildChangesNotifications))
    {
      plMsgChildrenChanged msg;
      msg.m_Type = plMsgChildrenChanged::Type::ChildAdded;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void plWorld::UnlinkFromParent(plGameObject* pObject)
{
  if (plGameObject* pParentObject = pObject->GetParent())
  {
    const plUInt32 uiIndex = pObject->m_InternalId.m_InstanceIndex;

    if (uiIndex == pParentObject->m_uiFirstChildIndex)
      pParentObject->m_uiFirstChildIndex = pObject->m_uiNextSiblingIndex;

    if (uiIndex == pParentObject->m_uiLastChildIndex)
      pParentObject->m_uiLastChildIndex = pObject->m_uiPrevSiblingIndex;

    if (plGameObject* pNextObject = GetObjectUnchecked(pObject->m_uiNextSiblingIndex))
      pNextObject->m_uiPrevSiblingIndex = pObject->m_uiPrevSiblingIndex;

    if (plGameObject* pPrevObject = GetObjectUnchecked(pObject->m_uiPrevSiblingIndex))
      pPrevObject->m_uiNextSiblingIndex = pObject->m_uiNextSiblingIndex;

    pParentObject->m_uiChildCount--;
    pObject->m_uiParentIndex = 0;
    pObject->m_pTransformationData->m_pParentData = nullptr;

    if (pObject->m_Flags.IsSet(plObjectFlags::ParentChangesNotifications))
    {
      plMsgParentChanged msg;
      msg.m_Type = plMsgParentChanged::Type::ParentUnlinked;
      msg.m_hParent = pParentObject->GetHandle();

      pObject->SendMessage(msg);
    }

    // Note that the sibling indices must not be set to 0 here.
    // They are still needed if we currently iterate over child objects.

    if (pParentObject->m_Flags.IsSet(plObjectFlags::ChildChangesNotifications))
    {
      plMsgChildrenChanged msg;
      msg.m_Type = plMsgChildrenChanged::Type::ChildRemoved;
      msg.m_hParent = pParentObject->GetHandle();
      msg.m_hChild = pObject->GetHandle();

      pParentObject->SendNotificationMessage(msg);
    }
  }
}

void plWorld::SetObjectGlobalKey(plGameObject* pObject, const plHashedString& sGlobalKey)
{
  if (m_Data.m_GlobalKeyToIdTable.Contains(sGlobalKey.GetHash()))
  {
    plLog::Error("Can't set global key to '{0}' because an object with this global key already exists. Global keys have to be unique.", sGlobalKey);
    return;
  }

  const plUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  // Remove existing entry first.
  plHashedString* pOldGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pOldGlobalKey))
  {
    if (sGlobalKey == *pOldGlobalKey)
    {
      return;
    }

    PLASMA_VERIFY(m_Data.m_GlobalKeyToIdTable.Remove(pOldGlobalKey->GetHash()), "Implementation error.");
    PLASMA_VERIFY(m_Data.m_IdToGlobalKeyTable.Remove(uiId), "Implementation error.");
  }

  // Insert new one if key is valid.
  if (!sGlobalKey.IsEmpty())
  {
    m_Data.m_GlobalKeyToIdTable.Insert(sGlobalKey.GetHash(), pObject->m_InternalId);
    m_Data.m_IdToGlobalKeyTable.Insert(uiId, sGlobalKey);
  }
}

plStringView plWorld::GetObjectGlobalKey(const plGameObject* pObject) const
{
  const plUInt32 uiId = pObject->m_InternalId.m_InstanceIndex;

  const plHashedString* pGlobalKey;
  if (m_Data.m_IdToGlobalKeyTable.TryGetValue(uiId, pGlobalKey))
  {
    return pGlobalKey->GetView();
  }

  return {};
}

void plWorld::ProcessQueuedMessage(const plInternal::WorldData::MessageQueue::Entry& entry)
{
  if (entry.m_MetaData.m_uiReceiverIsComponent)
  {
    plComponentHandle hComponent(plComponentId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    plComponent* pReceiverComponent = nullptr;
    if (TryGetComponent(hComponent, pReceiverComponent))
    {
      pReceiverComponent->SendMessageInternal(*entry.m_pMessage, true);
    }
    else
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        plLog::Warning("plWorld::ProcessQueuedMessage: Receiver plComponent for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
  else
  {
    plGameObjectHandle hObject(plGameObjectId(entry.m_MetaData.m_uiReceiverObjectOrComponent));

    plGameObject* pReceiverObject = nullptr;
    if (TryGetObject(hObject, pReceiverObject))
    {
      if (entry.m_MetaData.m_uiRecursive)
      {
        pReceiverObject->SendMessageRecursiveInternal(*entry.m_pMessage, true);
      }
      else
      {
        pReceiverObject->SendMessageInternal(*entry.m_pMessage, true);
      }
    }
    else
    {
#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEBUG)
      if (entry.m_pMessage->GetDebugMessageRouting())
      {
        plLog::Warning("plWorld::ProcessQueuedMessage: Receiver plGameObject for message of type '{0}' does not exist anymore.", entry.m_pMessage->GetId());
      }
#endif
    }
  }
}

void plWorld::ProcessQueuedMessages(plObjectMsgQueueType::Enum queueType)
{
  PLASMA_PROFILE_SCOPE("Process Queued Messages");

  struct MessageComparer
  {
    PLASMA_FORCE_INLINE bool Less(const plInternal::WorldData::MessageQueue::Entry& a, const plInternal::WorldData::MessageQueue::Entry& b) const
    {
      if (a.m_MetaData.m_Due != b.m_MetaData.m_Due)
        return a.m_MetaData.m_Due < b.m_MetaData.m_Due;

      const plInt32 iKeyA = a.m_pMessage->GetSortingKey();
      const plInt32 iKeyB = b.m_pMessage->GetSortingKey();
      if (iKeyA != iKeyB)
        return iKeyA < iKeyB;

      if (a.m_pMessage->GetId() != b.m_pMessage->GetId())
        return a.m_pMessage->GetId() < b.m_pMessage->GetId();

      if (a.m_MetaData.m_uiReceiverData != b.m_MetaData.m_uiReceiverData)
        return a.m_MetaData.m_uiReceiverData < b.m_MetaData.m_uiReceiverData;

      if (a.m_uiMessageHash == 0)
      {
        a.m_uiMessageHash = a.m_pMessage->GetHash();
      }

      if (b.m_uiMessageHash == 0)
      {
        b.m_uiMessageHash = b.m_pMessage->GetHash();
      }

      return a.m_uiMessageHash < b.m_uiMessageHash;
    }
  };

  // regular messages
  {
    plInternal::WorldData::MessageQueue& queue = m_Data.m_MessageQueues[queueType];
    queue.Sort(MessageComparer());

    m_Data.m_ProcessingMessageQueue = queueType;
    for (plUInt32 i = 0; i < queue.GetCount(); ++i)
    {
      ProcessQueuedMessage(queue[i]);

      // no need to deallocate these messages, they are allocated through a frame allocator
    }
    m_Data.m_ProcessingMessageQueue = plObjectMsgQueueType::COUNT;

    queue.Clear();
  }

  // timed messages
  {
    plInternal::WorldData::MessageQueue& queue = m_Data.m_TimedMessageQueues[queueType];
    queue.Sort(MessageComparer());

    const plTime now = m_Data.m_Clock.GetAccumulatedTime();

    m_Data.m_ProcessingMessageQueue = queueType;
    while (!queue.IsEmpty())
    {
      auto& entry = queue.Peek();
      if (entry.m_MetaData.m_Due > now)
        break;

      ProcessQueuedMessage(entry);

      PLASMA_DELETE(&m_Data.m_Allocator, entry.m_pMessage);

      queue.Dequeue();
    }
    m_Data.m_ProcessingMessageQueue = plObjectMsgQueueType::COUNT;
  }
}

// static
template <typename World, typename GameObject, typename Component>
void plWorld::FindEventMsgHandlers(World& world, const plMessage& msg, GameObject pSearchObject, plDynamicArray<Component>& out_components)
{
  using EventMessageHandlerComponentType = typename std::conditional<std::is_const<World>::value, const plEventMessageHandlerComponent*, plEventMessageHandlerComponent*>::type;

  out_components.Clear();

  // walk the graph upwards until an object is found with at least one plComponent that handles this type of message
  {
    auto pCurrentObject = pSearchObject;

    while (pCurrentObject != nullptr)
    {
      bool bContinueSearch = true;
      for (auto pComponent : pCurrentObject->GetComponents())
      {
        if constexpr (std::is_const<World>::value == false)
        {
          pComponent->EnsureInitialized();
        }

        if (pComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pComponent);
          bContinueSearch = false;
        }
        else
        {
          if constexpr (std::is_const<World>::value)
          {
            if (pComponent->IsInitialized() == false)
            {
              plLog::Warning("Component of type '{}' was not initialized (yet) and thus might have reported an incorrect result in HandlesMessage(). "
                             "To allow this component to be automatically initialized at this point in time call the non-const variant of SendEventMessage.",
                pComponent->GetDynamicRTTI()->GetTypeName());
            }
          }

          // only continue to search on parent objects if all event handlers on the current object have the "pass through unhandled events" flag set.
          if (auto pEventMessageHandlerComponent = plDynamicCast<EventMessageHandlerComponentType>(pComponent))
          {
            bContinueSearch &= pEventMessageHandlerComponent->GetPassThroughUnhandledEvents();
          }
        }
      }

      if (!bContinueSearch)
      {
        // stop searching as we found at least one plEventMessageHandlerComponent or one doesn't have the "pass through" flag set.
        return;
      }

      pCurrentObject = pCurrentObject->GetParent();
    }
  }

  // if no components have been found, check all event handler components that are registered as 'global event handlers'
  if (out_components.IsEmpty())
  {
    auto globalEventMessageHandler = plEventMessageHandlerComponent::GetAllGlobalEventHandler(&world);
    for (auto hEventMessageHandlerComponent : globalEventMessageHandler)
    {
      EventMessageHandlerComponentType pEventMessageHandlerComponent = nullptr;
      if (world.TryGetComponent(hEventMessageHandlerComponent, pEventMessageHandlerComponent))
      {
        if (pEventMessageHandlerComponent->HandlesMessage(msg))
        {
          out_components.PushBack(pEventMessageHandlerComponent);
        }
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void plWorld::RegisterUpdateFunction(const plComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  PLASMA_ASSERT_DEV(desc.m_Phase == plComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_uiGranularity == 0, "Granularity must be 0 for synchronous update functions");
  PLASMA_ASSERT_DEV(desc.m_Phase != plComponentManagerBase::UpdateFunctionDesc::Phase::Async || desc.m_DependsOn.GetCount() == 0, "Asynchronous update functions must not have dependencies");
  PLASMA_ASSERT_DEV(desc.m_Function.IsComparable(), "Delegates with captures are not allowed as plWorld update functions.");

  m_Data.m_UpdateFunctionsToRegister.PushBack(desc);
}

void plWorld::DeregisterUpdateFunction(const plComponentManagerBase::UpdateFunctionDesc& desc)
{
  CheckForWriteAccess();

  plDynamicArrayBase<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];

  for (plUInt32 i = updateFunctions.GetCount(); i-- > 0;)
  {
    if (updateFunctions[i].m_Function.IsEqualIfComparable(desc.m_Function))
    {
      updateFunctions.RemoveAtAndCopy(i);
    }
  }
}

void plWorld::DeregisterUpdateFunctions(plWorldModule* pModule)
{
  CheckForWriteAccess();

  for (plUInt32 phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync; phase < plWorldModule::UpdateFunctionDesc::Phase::COUNT; ++phase)
  {
    plDynamicArrayBase<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[phase];

    for (plUInt32 i = updateFunctions.GetCount(); i-- > 0;)
    {
      if (updateFunctions[i].m_Function.GetClassInstance() == pModule)
      {
        updateFunctions.RemoveAtAndCopy(i);
      }
    }
  }
}

void plWorld::AddComponentToInitialize(plComponentHandle hComponent)
{
  m_Data.m_pCurrentInitBatch->m_ComponentsToInitialize.PushBack(hComponent);
}

void plWorld::UpdateFromThread()
{
  PLASMA_LOCK(GetWriteMarker());

  Update();
}

void plWorld::UpdateSynchronous(const plArrayPtr<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions)
{
  plWorldModule::UpdateContext context;
  context.m_uiFirstComponentIndex = 0;
  context.m_uiComponentCount = plInvalidIndex;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    {
      PLASMA_PROFILE_SCOPE(updateFunction.m_sFunctionName);
      updateFunction.m_Function(context);
    }
  }
}

void plWorld::UpdateAsynchronous()
{
  plTaskGroupID taskGroupId = plTaskSystem::CreateTaskGroup(plTaskPriority::EarlyThisFrame);

  plDynamicArrayBase<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[plComponentManagerBase::UpdateFunctionDesc::Phase::Async];

  plUInt32 uiCurrentTaskIndex = 0;

  for (auto& updateFunction : updateFunctions)
  {
    if (updateFunction.m_bOnlyUpdateWhenSimulating && !m_Data.m_bSimulateWorld)
      continue;

    plWorldModule* pModule = static_cast<plWorldModule*>(updateFunction.m_Function.GetClassInstance());
    plComponentManagerBase* pManager = plDynamicCast<plComponentManagerBase*>(pModule);

    // a world module can also register functions in the async phase so we want at least one task
    const plUInt32 uiTotalCount = pManager != nullptr ? pManager->GetComponentCount() : 1;
    const plUInt32 uiGranularity = (updateFunction.m_uiGranularity != 0) ? updateFunction.m_uiGranularity : uiTotalCount;

    plUInt32 uiStartIndex = 0;
    while (uiStartIndex < uiTotalCount)
    {
      plSharedPtr<plInternal::WorldData::UpdateTask> pTask;
      if (uiCurrentTaskIndex < m_Data.m_UpdateTasks.GetCount())
      {
        pTask = m_Data.m_UpdateTasks[uiCurrentTaskIndex];
      }
      else
      {
        pTask = PLASMA_NEW(&m_Data.m_Allocator, plInternal::WorldData::UpdateTask);
        m_Data.m_UpdateTasks.PushBack(pTask);
      }

      pTask->ConfigureTask(updateFunction.m_sFunctionName, plTaskNesting::Maybe);
      pTask->m_Function = updateFunction.m_Function;
      pTask->m_uiStartIndex = uiStartIndex;
      pTask->m_uiCount = (uiStartIndex + uiGranularity < uiTotalCount) ? uiGranularity : plInvalidIndex;
      plTaskSystem::AddTaskToGroup(taskGroupId, pTask);

      ++uiCurrentTaskIndex;
      uiStartIndex += uiGranularity;
    }
  }

  plTaskSystem::StartTaskGroup(taskGroupId);
  plTaskSystem::WaitForGroup(taskGroupId);
}

bool plWorld::ProcessInitializationBatch(plInternal::WorldData::InitBatch& batch, plTime endTime)
{
  CheckForWriteAccess();

  // ensure that all components that are created during this batch (e.g. from prefabs)
  // will also get initialized within this batch
  m_Data.m_pCurrentInitBatch = &batch;
  PLASMA_SCOPE_EXIT(m_Data.m_pCurrentInitBatch = m_Data.m_pDefaultInitBatch);

  if (!batch.m_ComponentsToInitialize.IsEmpty())
  {
    plStringBuilder profileScopeName("Init ", batch.m_sName);
    PLASMA_PROFILE_SCOPE(profileScopeName);

    // Reserve for later use
    batch.m_ComponentsToStartSimulation.Reserve(batch.m_ComponentsToInitialize.GetCount());

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToInitialize < batch.m_ComponentsToInitialize.GetCount(); ++batch.m_uiNextComponentToInitialize)
    {
      plComponentHandle hComponent = batch.m_ComponentsToInitialize[batch.m_uiNextComponentToInitialize];

      // if it is in the editor, the component might have been added and already deleted, without ever running the simulation
      plComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      PLASMA_ASSERT_DEBUG(pComponent->GetOwner() != nullptr, "Component must have a valid owner");

      // make sure the object's transform is up to date before the component is initialized.
      pComponent->GetOwner()->UpdateGlobalTransform();

      pComponent->EnsureInitialized();

      if (pComponent->IsActive())
      {
        pComponent->OnActivated();

        batch.m_ComponentsToStartSimulation.PushBack(hComponent);
      }

      // Check if there is still time left to initialize more components
      if (plTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToInitialize;
        return false;
      }
    }

    batch.m_ComponentsToInitialize.Clear();
    batch.m_uiNextComponentToInitialize = 0;
  }

  if (m_Data.m_bSimulateWorld)
  {
    plStringBuilder startSimName("Start Sim ", batch.m_sName);
    PLASMA_PROFILE_SCOPE(startSimName);

    // Can't use foreach here because the array might be resized during iteration.
    for (; batch.m_uiNextComponentToStartSimulation < batch.m_ComponentsToStartSimulation.GetCount(); ++batch.m_uiNextComponentToStartSimulation)
    {
      plComponentHandle hComponent = batch.m_ComponentsToStartSimulation[batch.m_uiNextComponentToStartSimulation];

      // if it is in the editor, the component might have been added and already deleted,  without ever running the simulation
      plComponent* pComponent = nullptr;
      if (!TryGetComponent(hComponent, pComponent))
        continue;

      if (pComponent->IsActiveAndInitialized())
      {
        pComponent->EnsureSimulationStarted();
      }

      // Check if there is still time left to initialize more components
      if (plTime::Now() >= endTime)
      {
        ++batch.m_uiNextComponentToStartSimulation;
        return false;
      }
    }

    batch.m_ComponentsToStartSimulation.Clear();
    batch.m_uiNextComponentToStartSimulation = 0;
  }

  return true;
}

void plWorld::ProcessComponentsToInitialize()
{
  CheckForWriteAccess();

  if (m_Data.m_bSimulateWorld)
  {
    PLASMA_PROFILE_SCOPE("Modules Start Simulation");

    // Can't use foreach here because the array might be resized during iteration.
    for (plUInt32 i = 0; i < m_Data.m_ModulesToStartSimulation.GetCount(); ++i)
    {
      m_Data.m_ModulesToStartSimulation[i]->OnSimulationStarted();
    }

    m_Data.m_ModulesToStartSimulation.Clear();
  }

  PLASMA_PROFILE_SCOPE("Initialize Components");

  plTime endTime = plTime::Now() + m_Data.m_MaxInitializationTimePerFrame;

  // First process all component init batches that have to finish within this frame
  for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
  {
    auto& pInitBatch = it.Value();
    if (pInitBatch->m_bIsReady && pInitBatch->m_bMustFinishWithinOneFrame)
    {
      ProcessInitializationBatch(*pInitBatch, plTime::Now() + plTime::Hours(10000));
    }
  }

  // If there is still time left process other component init batches
  if (plTime::Now() < endTime)
  {
    for (auto it = m_Data.m_InitBatches.GetIterator(); it.IsValid(); ++it)
    {
      auto& pInitBatch = it.Value();
      if (!pInitBatch->m_bIsReady || pInitBatch->m_bMustFinishWithinOneFrame)
        continue;

      if (!ProcessInitializationBatch(*pInitBatch, endTime))
        return;
    }
  }
}

void plWorld::ProcessUpdateFunctionsToRegister()
{
  CheckForWriteAccess();

  if (m_Data.m_UpdateFunctionsToRegister.IsEmpty())
    return;

  PLASMA_PROFILE_SCOPE("Register update functions");

  while (!m_Data.m_UpdateFunctionsToRegister.IsEmpty())
  {
    const plUInt32 uiNumFunctionsToRegister = m_Data.m_UpdateFunctionsToRegister.GetCount();

    for (plUInt32 i = uiNumFunctionsToRegister; i-- > 0;)
    {
      if (RegisterUpdateFunctionInternal(m_Data.m_UpdateFunctionsToRegister[i]).Succeeded())
      {
        m_Data.m_UpdateFunctionsToRegister.RemoveAtAndCopy(i);
      }
    }

    PLASMA_ASSERT_DEV(m_Data.m_UpdateFunctionsToRegister.GetCount() < uiNumFunctionsToRegister, "No functions have been registered because the dependencies could not be found.");
  }
}

plResult plWorld::RegisterUpdateFunctionInternal(const plWorldModule::UpdateFunctionDesc& desc)
{
  plDynamicArrayBase<plInternal::WorldData::RegisteredUpdateFunction>& updateFunctions = m_Data.m_UpdateFunctions[desc.m_Phase.GetValue()];
  plUInt32 uiInsertionIndex = 0;

  for (plUInt32 i = 0; i < desc.m_DependsOn.GetCount(); ++i)
  {
    plUInt32 uiDependencyIndex = plInvalidIndex;

    for (plUInt32 j = 0; j < updateFunctions.GetCount(); ++j)
    {
      if (updateFunctions[j].m_sFunctionName == desc.m_DependsOn[i])
      {
        uiDependencyIndex = j;
        break;
      }
    }

    if (uiDependencyIndex == plInvalidIndex) // dependency not found
    {
      return PLASMA_FAILURE;
    }
    else
    {
      uiInsertionIndex = plMath::Max(uiInsertionIndex, uiDependencyIndex + 1);
    }
  }

  plInternal::WorldData::RegisteredUpdateFunction newFunction;
  newFunction.FillFromDesc(desc);

  while (uiInsertionIndex < updateFunctions.GetCount())
  {
    const auto& existingFunction = updateFunctions[uiInsertionIndex];
    if (newFunction < existingFunction)
    {
      break;
    }

    ++uiInsertionIndex;
  }

  updateFunctions.Insert(newFunction, uiInsertionIndex);

  return PLASMA_SUCCESS;
}

void plWorld::DeleteDeadObjects()
{
  while (!m_Data.m_DeadObjects.IsEmpty())
  {
    plGameObject* pObject = m_Data.m_DeadObjects.GetIterator().Key();

    if (!pObject->m_pTransformationData->m_hSpatialData.IsInvalidated())
    {
      m_Data.m_pSpatialSystem->DeleteSpatialData(pObject->m_pTransformationData->m_hSpatialData);
    }

    m_Data.DeleteTransformationData(pObject->IsDynamic(), pObject->m_uiHierarchyLevel, pObject->m_pTransformationData);

    plGameObject* pMovedObject = nullptr;
    m_Data.m_ObjectStorage.Delete(pObject, pMovedObject);

    if (pObject != pMovedObject)
    {
      // patch the id table: the last element in the storage has been moved to deleted object's location,
      // thus the pointer now points to another object
      plGameObjectId id = pObject->m_InternalId;
      if (id.m_InstanceIndex != plGameObjectId::INVALID_INSTANCE_INDEX)
        m_Data.m_Objects[id] = pObject;

      // The moved object might be deleted as well so we remove it from the dead objects set instead.
      // If that is not the case we remove the original object from the set.
      if (m_Data.m_DeadObjects.Remove(pMovedObject))
      {
        continue;
      }
    }

    m_Data.m_DeadObjects.Remove(pObject);
  }
}

void plWorld::DeleteDeadComponents()
{
  while (!m_Data.m_DeadComponents.IsEmpty())
  {
    plComponent* pComponent = m_Data.m_DeadComponents.GetIterator().Key();

    plComponentManagerBase* pManager = pComponent->GetOwningManager();
    plComponent* pMovedComponent = nullptr;
    pManager->DeleteComponentStorage(pComponent, pMovedComponent);

    // another component has been moved to the deleted component location
    if (pComponent != pMovedComponent)
    {
      pManager->PatchIdTable(pComponent);

      if (plGameObject* pOwner = pComponent->GetOwner())
      {
        pOwner->FixComponentPointer(pMovedComponent, pComponent);
      }

      // The moved component might be deleted as well so we remove it from the dead components set instead.
      // If that is not the case we remove the original component from the set.
      if (m_Data.m_DeadComponents.Remove(pMovedComponent))
      {
        continue;
      }
    }

    m_Data.m_DeadComponents.Remove(pComponent);
  }
}

void plWorld::PatchHierarchyData(plGameObject* pObject, plGameObject::TransformPreservation preserve)
{
  plGameObject* pParent = pObject->GetParent();

  RecreateHierarchyData(pObject, pObject->IsDynamic());

  pObject->m_pTransformationData->m_pParentData = pParent != nullptr ? pParent->m_pTransformationData : nullptr;

  if (preserve == plGameObject::TransformPreservation::PreserveGlobal)
  {
    // SetGlobalTransform will internally trigger bounds update for static objects
    pObject->SetGlobalTransform(pObject->m_pTransformationData->m_globalTransform);
  }
  else
  {
    // Explicitly trigger transform AND bounds update, otherwise bounds would be outdated for static objects
    // Don't call pObject->UpdateGlobalTransformAndBounds() here since that would recursively update the parent global transform which is already up-to-date.
    pObject->m_pTransformationData->UpdateGlobalTransformNonRecursive(GetUpdateCounter());

    pObject->m_pTransformationData->UpdateGlobalBounds(GetSpatialSystem());
  }

  for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
  {
    PatchHierarchyData(it, preserve);
  }
  PLASMA_ASSERT_DEBUG(pObject->m_pTransformationData != pObject->m_pTransformationData->m_pParentData, "Hierarchy corrupted!");
}

void plWorld::RecreateHierarchyData(plGameObject* pObject, bool bWasDynamic)
{
  plGameObject* pParent = pObject->GetParent();

  const plUInt32 uiNewHierarchyLevel = pParent != nullptr ? pParent->m_uiHierarchyLevel + 1 : 0;
  const plUInt32 uiOldHierarchyLevel = pObject->m_uiHierarchyLevel;

  const bool bIsDynamic = pObject->IsDynamic();

  if (uiNewHierarchyLevel != uiOldHierarchyLevel || bIsDynamic != bWasDynamic)
  {
    plGameObject::TransformationData* pOldTransformationData = pObject->m_pTransformationData;

    plGameObject::TransformationData* pNewTransformationData = m_Data.CreateTransformationData(bIsDynamic, uiNewHierarchyLevel);
    plMemoryUtils::Copy(pNewTransformationData, pOldTransformationData, 1);

    pObject->m_uiHierarchyLevel = static_cast<plUInt16>(uiNewHierarchyLevel);
    pObject->m_pTransformationData = pNewTransformationData;

    // fix parent transform data for children as well
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      plGameObject::TransformationData* pTransformData = it->m_pTransformationData;
      pTransformData->m_pParentData = pNewTransformationData;
    }

    m_Data.DeleteTransformationData(bWasDynamic, uiOldHierarchyLevel, pOldTransformationData);
  }
}

void plWorld::SetMaxInitializationTimePerFrame(plTime maxInitTime)
{
  CheckForWriteAccess();

  m_Data.m_MaxInitializationTimePerFrame = maxInitTime;
}

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_World);
