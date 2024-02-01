#include <Core/CorePCH.h>

#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/StringDeduplicationContext.h>

void plWorldWriter::Clear()
{
  m_AllRootObjects.Clear();
  m_AllChildObjects.Clear();
  m_AllComponents.Clear();

  m_pStream = nullptr;
  m_pExclude = nullptr;

  // invalid handles
  {
    m_WrittenGameObjectHandles.Clear();
    m_WrittenGameObjectHandles[plGameObjectHandle()] = 0;
  }
}

void plWorldWriter::WriteWorld(plStreamWriter& inout_stream, plWorld& ref_world, const plTagSet* pExclude)
{
  Clear();

  m_pStream = &inout_stream;
  m_pExclude = pExclude;

  PL_LOCK(ref_world.GetReadMarker());

  ref_world.Traverse(plMakeDelegate(&plWorldWriter::ObjectTraverser, this), plWorld::TraversalMethod::DepthFirst);

  WriteToStream().IgnoreResult();
}

void plWorldWriter::WriteObjects(plStreamWriter& inout_stream, const plDeque<const plGameObject*>& rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const plGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<plGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

void plWorldWriter::WriteObjects(plStreamWriter& inout_stream, plArrayPtr<const plGameObject*> rootObjects)
{
  Clear();

  m_pStream = &inout_stream;

  for (const plGameObject* pObject : rootObjects)
  {
    // traversal function takes a non-const object, but we only read it anyway
    Traverse(const_cast<plGameObject*>(pObject));
  }

  WriteToStream().IgnoreResult();
}

plResult plWorldWriter::WriteToStream()
{
  const plUInt8 uiVersion = 10;
  *m_pStream << uiVersion;

  // version 8: use string dedup instead of handle writer
  plStringDeduplicationWriteContext stringDedupWriteContext(*m_pStream);
  m_pStream = &stringDedupWriteContext.Begin();

  IncludeAllComponentBaseTypes();

  plUInt32 uiNumRootObjects = m_AllRootObjects.GetCount();
  plUInt32 uiNumChildObjects = m_AllChildObjects.GetCount();
  plUInt32 uiNumComponentTypes = m_AllComponents.GetCount();

  *m_pStream << uiNumRootObjects;
  *m_pStream << uiNumChildObjects;
  *m_pStream << uiNumComponentTypes;

  // this is used to sort all component types by name, to make the file serialization deterministic
  plMap<plString, const plRTTI*> sortedTypes;

  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    sortedTypes[it.Key()->GetTypeName()] = it.Key();
  }

  AssignGameObjectIndices();
  AssignComponentHandleIndices(sortedTypes);

  for (const auto* pObject : m_AllRootObjects)
  {
    WriteGameObject(pObject);
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    WriteGameObject(pObject);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentTypeInfo(it.Value());
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentCreationData(m_AllComponents[it.Value()].m_Components);
  }

  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    WriteComponentSerializationData(m_AllComponents[it.Value()].m_Components);
  }

  PL_SUCCEED_OR_RETURN(stringDedupWriteContext.End());
  m_pStream = &stringDedupWriteContext.GetOriginalStream();

  return PL_SUCCESS;
}


void plWorldWriter::AssignGameObjectIndices()
{
  plUInt32 uiGameObjectIndex = 1;
  for (const auto* pObject : m_AllRootObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }

  for (const auto* pObject : m_AllChildObjects)
  {
    m_WrittenGameObjectHandles[pObject->GetHandle()] = uiGameObjectIndex;
    ++uiGameObjectIndex;
  }
}

void plWorldWriter::AssignComponentHandleIndices(const plMap<plString, const plRTTI*>& sortedTypes)
{
  plUInt16 uiTypeIndex = 0;

  PL_ASSERT_DEV(m_AllComponents.GetCount() <= plMath::MaxValue<plUInt16>(), "Too many types for world writer");

  // assign the component handle indices in the order in which the components are written
  for (auto it = sortedTypes.GetIterator(); it.IsValid(); ++it)
  {
    auto& components = m_AllComponents[it.Value()];

    components.m_uiSerializedTypeIndex = uiTypeIndex;
    ++uiTypeIndex;

    plUInt32 uiComponentIndex = 1;
    components.m_HandleToIndex[plComponentHandle()] = 0;

    for (const plComponent* pComp : components.m_Components)
    {
      components.m_HandleToIndex[pComp->GetHandle()] = uiComponentIndex;
      ++uiComponentIndex;
    }
  }
}


void plWorldWriter::IncludeAllComponentBaseTypes()
{
  plDynamicArray<const plRTTI*> allNow;
  allNow.Reserve(m_AllComponents.GetCount());
  for (auto it = m_AllComponents.GetIterator(); it.IsValid(); ++it)
  {
    allNow.PushBack(it.Key());
  }

  for (auto pRtti : allNow)
  {
    IncludeAllComponentBaseTypes(pRtti->GetParentType());
  }
}


void plWorldWriter::IncludeAllComponentBaseTypes(const plRTTI* pRtti)
{
  if (pRtti == nullptr || !pRtti->IsDerivedFrom<plComponent>() || m_AllComponents.Contains(pRtti))
    return;

  // this is actually used to insert the type, but we have no component of this type
  m_AllComponents[pRtti];

  IncludeAllComponentBaseTypes(pRtti->GetParentType());
}


void plWorldWriter::Traverse(plGameObject* pObject)
{
  if (ObjectTraverser(pObject) == plVisitorExecution::Continue)
  {
    for (auto it = pObject->GetChildren(); it.IsValid(); ++it)
    {
      Traverse(&(*it));
    }
  }
}

void plWorldWriter::WriteGameObjectHandle(const plGameObjectHandle& hObject)
{
  auto it = m_WrittenGameObjectHandles.Find(hObject);

  plUInt32 uiIndex = 0;

  PL_ASSERT_DEV(it.IsValid(), "Referenced object does not exist in the scene. This can happen, if it was optimized away, because it had no name, no children and no essential components.");

  if (it.IsValid())
    uiIndex = it.Value();

  *m_pStream << uiIndex;
}

void plWorldWriter::WriteComponentHandle(const plComponentHandle& hComponent)
{
  plUInt16 uiTypeIndex = 0;
  plUInt32 uiIndex = 0;

  plComponent* pComponent = nullptr;
  if (plWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent))
  {
    if (auto* components = m_AllComponents.GetValue(pComponent->GetDynamicRTTI()))
    {
      auto it = components->m_HandleToIndex.Find(hComponent);
      PL_ASSERT_DEBUG(it.IsValid(), "Handle should always be in the written map at this point");

      if (it.IsValid())
      {
        uiTypeIndex = components->m_uiSerializedTypeIndex;
        uiIndex = it.Value();
      }
    }
  }

  *m_pStream << uiTypeIndex;
  *m_pStream << uiIndex;
}

plVisitorExecution::Enum plWorldWriter::ObjectTraverser(plGameObject* pObject)
{
  if (m_pExclude && pObject->GetTags().IsAnySet(*m_pExclude))
    return plVisitorExecution::Skip;
  if (pObject->WasCreatedByPrefab())
    return plVisitorExecution::Skip;

  if (pObject->GetParent())
    m_AllChildObjects.PushBack(pObject);
  else
    m_AllRootObjects.PushBack(pObject);

  auto components = pObject->GetComponents();

  for (const plComponent* pComp : components)
  {
    if (pComp->WasCreatedByPrefab())
      continue;

    m_AllComponents[pComp->GetDynamicRTTI()].m_Components.PushBack(pComp);
  }

  return plVisitorExecution::Continue;
}

void plWorldWriter::WriteGameObject(const plGameObject* pObject)
{
  if (pObject->GetParent())
    WriteGameObjectHandle(pObject->GetParent()->GetHandle());
  else
    WriteGameObjectHandle(plGameObjectHandle());

  plStreamWriter& s = *m_pStream;

  s << pObject->GetName();
  s << pObject->GetGlobalKey();
  s << pObject->GetLocalPosition();
  s << pObject->GetLocalRotation();
  s << pObject->GetLocalScaling();
  s << pObject->GetLocalUniformScaling();
  s << pObject->GetActiveFlag();
  s << pObject->IsDynamic();
  pObject->GetTags().Save(s);
  s << pObject->GetTeamID();
  s << pObject->GetStableRandomSeed();
}

void plWorldWriter::WriteComponentTypeInfo(const plRTTI* pRtti)
{
  plStreamWriter& s = *m_pStream;

  s << pRtti->GetTypeName();
  s << pRtti->GetTypeVersion();
}

void plWorldWriter::WriteComponentCreationData(const plDeque<const plComponent*>& components)
{
  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter memWriter(&storage);

  plStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  {
    plStreamWriter& s = *m_pStream;
    s << components.GetCount();

    plUInt32 uiComponentIndex = 1;
    for (auto pComponent : components)
    {
      WriteGameObjectHandle(pComponent->GetOwner()->GetHandle());
      s << uiComponentIndex;
      ++uiComponentIndex;

      s << pComponent->GetActiveFlag();

      // version 7
      {
        plUInt8 userFlags = 0;
        for (plUInt8 i = 0; i < 8; ++i)
        {
          userFlags |= pComponent->GetUserFlag(i) ? PL_BIT(i) : 0;
        }

        s << userFlags;
      }
    }
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    plStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    PL_ASSERT_ALWAYS(storage.GetStorageSize64() <= plMath::MaxValue<plUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}

void plWorldWriter::WriteComponentSerializationData(const plDeque<const plComponent*>& components)
{
  plDefaultMemoryStreamStorage storage;
  plMemoryStreamWriter memWriter(&storage);

  plStreamWriter* pPrevStream = m_pStream;
  m_pStream = &memWriter;

  // write to memory stream
  for (auto pComp : components)
  {
    pComp->SerializeComponent(*this);
  }

  m_pStream = pPrevStream;

  // write result to actual stream
  {
    plStreamWriter& s = *m_pStream;
    s << storage.GetStorageSize32();

    PL_ASSERT_ALWAYS(storage.GetStorageSize64() <= plMath::MaxValue<plUInt32>(), "Slight file format change and version increase needed to support > 4GB worlds.");

    storage.CopyToStream(s).IgnoreResult();
  }
}


