#pragma once

template <typename KEY, typename VALUE>
plObjectMetaData<KEY, VALUE>::plObjectMetaData()
{
  m_DefaultValue = VALUE();

  auto pStorage = PLASMA_DEFAULT_NEW(Storage);
  pStorage->m_AcessingKey = KEY();
  pStorage->m_AccessMode = Storage::AccessMode::Nothing;
  SwapStorage(pStorage);
}

template <typename KEY, typename VALUE>
const VALUE* plObjectMetaData<KEY, VALUE>::BeginReadMetaData(const KEY objectKey) const
{
  m_pMetaStorage->m_Mutex.Lock();
  PLASMA_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");
  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Read;
  m_pMetaStorage->m_AcessingKey = objectKey;

  const VALUE* pRes = nullptr;
  if (m_pMetaStorage->m_MetaData.TryGetValue(objectKey, pRes)) // TryGetValue is not const correct with the second parameter
    return pRes;

  return &m_DefaultValue;
}

template <typename KEY, typename VALUE>
void plObjectMetaData<KEY, VALUE>::ClearMetaData(const KEY objectKey)
{
  PLASMA_LOCK(m_pMetaStorage->m_Mutex);
  PLASMA_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");

  if (HasMetaData(objectKey))
  {
    m_pMetaStorage->m_MetaData.Remove(objectKey);

    EventData e;
    e.m_ObjectKey = objectKey;
    e.m_pValue = &m_DefaultValue;

    m_pMetaStorage->m_DataModifiedEvent.Broadcast(e);
  }
}

template <typename KEY, typename VALUE>
bool plObjectMetaData<KEY, VALUE>::HasMetaData(const KEY objectKey) const
{
  PLASMA_LOCK(m_pMetaStorage->m_Mutex);
  const VALUE* pValue = nullptr;
  return m_pMetaStorage->m_MetaData.TryGetValue(objectKey, pValue);
}

template <typename KEY, typename VALUE>
VALUE* plObjectMetaData<KEY, VALUE>::BeginModifyMetaData(const KEY objectKey)
{
  m_pMetaStorage->m_Mutex.Lock();
  PLASMA_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Nothing, "Already accessing some data");
  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Write;
  m_pMetaStorage->m_AcessingKey = objectKey;

  return &m_pMetaStorage->m_MetaData[objectKey];
}

template <typename KEY, typename VALUE>
void plObjectMetaData<KEY, VALUE>::EndReadMetaData() const
{
  PLASMA_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Read, "Not accessing data at the moment");

  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Nothing;
  m_pMetaStorage->m_Mutex.Unlock();
}


template <typename KEY, typename VALUE>
void plObjectMetaData<KEY, VALUE>::EndModifyMetaData(plUInt32 uiModifiedFlags /*= 0xFFFFFFFF*/)
{
  PLASMA_ASSERT_DEV(m_pMetaStorage->m_AccessMode == Storage::AccessMode::Write, "Not accessing data at the moment");
  m_pMetaStorage->m_AccessMode = Storage::AccessMode::Nothing;

  if (uiModifiedFlags != 0)
  {
    EventData e;
    e.m_ObjectKey = m_pMetaStorage->m_AcessingKey;
    e.m_pValue = &m_pMetaStorage->m_MetaData[m_pMetaStorage->m_AcessingKey];
    e.m_uiModifiedFlags = uiModifiedFlags;

    m_pMetaStorage->m_DataModifiedEvent.Broadcast(e);
  }

  m_pMetaStorage->m_Mutex.Unlock();
}


template <typename KEY, typename VALUE>
void plObjectMetaData<KEY, VALUE>::AttachMetaDataToAbstractGraph(plAbstractObjectGraph& inout_graph) const
{
  auto& AllNodes = inout_graph.GetAllNodes();

  PLASMA_LOCK(m_pMetaStorage->m_Mutex);

  plHashTable<const char*, plVariant> DefaultValues;

  // store the default values in an easily accessible hash map, to be able to compare against them
  {
    DefaultValues.Reserve(m_DefaultValue.GetDynamicRTTI()->GetProperties().GetCount());

    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != plPropertyCategory::Member)
        continue;

      DefaultValues[pProp->GetPropertyName()] =
        plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), &m_DefaultValue);
    }
  }

  // now serialize all properties that differ from the default value
  {
    plVariant value;

    for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
    {
      auto* pNode = it.Value();
      const plUuid& guid = pNode->GetGuid();

      const VALUE* pMeta = nullptr;
      if (!m_pMetaStorage->m_MetaData.TryGetValue(guid, pMeta)) // TryGetValue is not const correct with the second parameter
        continue;                               // it is the default object, so all values are default -> skip

      for (const auto& pProp : pMeta->GetDynamicRTTI()->GetProperties())
      {
        if (pProp->GetCategory() != plPropertyCategory::Member)
          continue;

        value = plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pMeta);

        if (value.IsValid() && DefaultValues[pProp->GetPropertyName()] != value)
        {
          pNode->AddProperty(pProp->GetPropertyName(), value);
        }
      }
    }
  }
}


template <typename KEY, typename VALUE>
void plObjectMetaData<KEY, VALUE>::RestoreMetaDataFromAbstractGraph(const plAbstractObjectGraph& graph)
{
  PLASMA_LOCK(m_pMetaStorage->m_Mutex);

  plHybridArray<plString, 16> PropertyNames;

  // find all properties (names) that we want to read
  {
    for (const auto& pProp : m_DefaultValue.GetDynamicRTTI()->GetProperties())
    {
      if (pProp->GetCategory() != plPropertyCategory::Member)
        continue;

      PropertyNames.PushBack(pProp->GetPropertyName());
    }
  }

  auto& AllNodes = graph.GetAllNodes();

  for (auto it = AllNodes.GetIterator(); it.IsValid(); ++it)
  {
    auto* pNode = it.Value();
    const plUuid& guid = pNode->GetGuid();

    for (const auto& name : PropertyNames)
    {
      if (const auto* pProp = pNode->FindProperty(name))
      {
        VALUE* pValue = &m_pMetaStorage->m_MetaData[guid];

        plReflectionUtils::SetMemberPropertyValue(
          static_cast<const plAbstractMemberProperty*>(pValue->GetDynamicRTTI()->FindPropertyByName(name)), pValue, pProp->m_Value);
      }
    }
  }
}

template <typename KEY, typename VALUE>
plSharedPtr<typename plObjectMetaData<KEY, VALUE>::Storage> plObjectMetaData<KEY, VALUE>::SwapStorage(plSharedPtr<typename plObjectMetaData<KEY, VALUE>::Storage> pNewStorage)
{
  PLASMA_ASSERT_ALWAYS(pNewStorage != nullptr, "Need a valid history storage object");

  auto retVal = m_pMetaStorage;

  m_EventsUnsubscriber.Unsubscribe();

  m_pMetaStorage = pNewStorage;

  m_pMetaStorage->m_DataModifiedEvent.AddEventHandler([this](const EventData& e) { m_DataModifiedEvent.Broadcast(e); }, m_EventsUnsubscriber);

  return retVal;
}
