#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

void plRttiConverterContext::Clear()
{
  m_GuidToObject.Clear();
  m_ObjectToGuid.Clear();
  m_QueuedObjects.Clear();
}

void plRttiConverterContext::OnUnknownTypeError(plStringView sTypeName)
{
  plLog::Error("RTTI type '{0}' is unknown, CreateObjectFromNode failed.", sTypeName);
}

plUuid plRttiConverterContext::GenerateObjectGuid(const plUuid& parentGuid, const plAbstractProperty* pProp, plVariant index, void* pObject) const
{
  plUuid guid = parentGuid;
  guid.HashCombine(plUuid::StableUuidForString(pProp->GetPropertyName()));
  if (index.IsA<plString>())
  {
    guid.HashCombine(plUuid::StableUuidForString(index.Get<plString>()));
  }
  else if (index.CanConvertTo<plUInt32>())
  {
    guid.HashCombine(plUuid::StableUuidForInt(index.ConvertTo<plUInt32>()));
  }
  else if (index.IsValid())
  {
    PLASMA_REPORT_FAILURE("Index type must be plUInt32 or plString.");
  }
  // plLog::Warning("{0},{1},{2} -> {3}", parentGuid, pProp->GetPropertyName(), index, guid);
  return guid;
}

plInternal::NewInstance<void> plRttiConverterContext::CreateObject(const plUuid& guid, const plRTTI* pRtti)
{
  PLASMA_ASSERT_DEBUG(pRtti != nullptr, "Cannot create object, RTTI type is unknown");
  if (!pRtti->GetAllocator() || !pRtti->GetAllocator()->CanAllocate())
    return nullptr;

  auto pObj = pRtti->GetAllocator()->Allocate<void>();
  RegisterObject(guid, pRtti, pObj);
  return pObj;
}

void plRttiConverterContext::DeleteObject(const plUuid& guid)
{
  auto object = GetObjectByGUID(guid);
  if (object.m_pObject)
  {
    object.m_pType->GetAllocator()->Deallocate(object.m_pObject);
  }
  UnregisterObject(guid);
}

void plRttiConverterContext::RegisterObject(const plUuid& guid, const plRTTI* pRtti, void* pObject)
{
  PLASMA_ASSERT_DEV(pObject != nullptr, "cannot register null object!");
  plRttiConverterObject& co = m_GuidToObject[guid];

  if (pRtti->IsDerivedFrom<plReflectedClass>())
  {
    pRtti = static_cast<plReflectedClass*>(pObject)->GetDynamicRTTI();
  }

  // TODO: Actually remove child owner ptr from register when deleting an object
  // PLASMA_ASSERT_DEV(co.m_pObject == nullptr || (co.m_pObject == pObject && co.m_pType == pRtti), "Registered same guid twice with different
  // values");

  co.m_pObject = pObject;
  co.m_pType = pRtti;

  m_ObjectToGuid[pObject] = guid;
}

void plRttiConverterContext::UnregisterObject(const plUuid& guid)
{
  plRttiConverterObject* pObj;
  if (m_GuidToObject.TryGetValue(guid, pObj))
  {
    m_GuidToObject.Remove(guid);
    m_ObjectToGuid.Remove(pObj->m_pObject);
  }
}

plRttiConverterObject plRttiConverterContext::GetObjectByGUID(const plUuid& guid) const
{
  plRttiConverterObject object;
  m_GuidToObject.TryGetValue(guid, object);
  return object;
}

plUuid plRttiConverterContext::GetObjectGUID(const plRTTI* pRtti, const void* pObject) const
{
  plUuid guid;

  if (pObject != nullptr)
    m_ObjectToGuid.TryGetValue(pObject, guid);

  return guid;
}

const plRTTI* plRttiConverterContext::FindTypeByName(plStringView sName) const
{
  return plRTTI::FindTypeByName(sName);
}

plUuid plRttiConverterContext::EnqueObject(const plUuid& guid, const plRTTI* pRtti, void* pObject)
{
  PLASMA_ASSERT_DEBUG(guid.IsValid(), "For stable serialization, guid must be well defined");
  plUuid res = guid;

  if (pObject != nullptr)
  {
    // In the rare case that this succeeds we already encountered the object with a different guid before.
    // This can happen if two pointer owner point to the same object.
    if (!m_ObjectToGuid.TryGetValue(pObject, res))
    {
      RegisterObject(guid, pRtti, pObject);
    }

    m_QueuedObjects.Insert(res);
  }
  else
  {
    // Replace nullptr with invalid uuid.
    res = plUuid();
  }
  return res;
}

plRttiConverterObject plRttiConverterContext::DequeueObject()
{
  if (!m_QueuedObjects.IsEmpty())
  {
    auto it = m_QueuedObjects.GetIterator();
    auto object = GetObjectByGUID(it.Key());
    PLASMA_ASSERT_DEV(object.m_pObject != nullptr, "Enqueued object was never registered!");

    m_QueuedObjects.Remove(it);

    return object;
  }

  return plRttiConverterObject();
}


plRttiConverterWriter::plRttiConverterWriter(plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext, bool bSerializeReadOnly, bool bSerializeOwnerPtrs)
{
  m_pGraph = pGraph;
  m_pContext = pContext;

  m_Filter = [bSerializeReadOnly, bSerializeOwnerPtrs](const void* pObject, const plAbstractProperty* pProp) {
    if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly) && !bSerializeReadOnly)
      return false;

    if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner) && !bSerializeOwnerPtrs)
      return false;

    return true;
  };
}

plRttiConverterWriter::plRttiConverterWriter(plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext, FilterFunction filter)
  : m_pContext(pContext)
  , m_pGraph(pGraph)
  , m_Filter(filter)
{
  PLASMA_ASSERT_DEBUG(filter.IsValid(), "Either filter function must be valid or a different ctor must be chosen.");
}

plAbstractObjectNode* plRttiConverterWriter::AddObjectToGraph(const plRTTI* pRtti, const void* pObject, const char* szNodeName)
{
  const plUuid guid = m_pContext->GetObjectGUID(pRtti, pObject);
  PLASMA_ASSERT_DEV(guid.IsValid(), "The object was not registered. Call plRttiConverterContext::RegisterObject before adding.");
  plAbstractObjectNode* pNode = AddSubObjectToGraph(pRtti, pObject, guid, szNodeName);

  plRttiConverterObject obj = m_pContext->DequeueObject();
  while (obj.m_pObject != nullptr)
  {
    const plUuid objectGuid = m_pContext->GetObjectGUID(obj.m_pType, obj.m_pObject);
    AddSubObjectToGraph(obj.m_pType, obj.m_pObject, objectGuid, nullptr);

    obj = m_pContext->DequeueObject();
  }

  return pNode;
}

plAbstractObjectNode* plRttiConverterWriter::AddSubObjectToGraph(const plRTTI* pRtti, const void* pObject, const plUuid& guid, const char* szNodeName)
{
  plAbstractObjectNode* pNode = m_pGraph->AddNode(guid, pRtti->GetTypeName(), pRtti->GetTypeVersion(), szNodeName);
  AddProperties(pNode, pRtti, pObject);
  return pNode;
}

void plRttiConverterWriter::AddProperty(plAbstractObjectNode* pNode, const plAbstractProperty* pProp, const void* pObject)
{
  if (!m_Filter(pObject, pProp))
    return;

  plVariant vTemp;
  plStringBuilder sTemp;
  const plRTTI* pPropType = pProp->GetSpecificType();
  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      const plAbstractMemberProperty* pSpecific = static_cast<const plAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();

        plUuid guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, plVariant(), pRefrencedObject);
        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
        {
          guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
        else
        {
          guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);
          pNode->AddProperty(pProp->GetPropertyName(), guid);
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        {
          vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          plReflectionUtils::EnumerationToString(pPropType, vTemp.Get<plInt64>(), sTemp);

          pNode->AddProperty(pProp->GetPropertyName(), sTemp.GetData());
        }
        else if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject));
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class) && pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);


          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            const plUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, plVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();

            pSpecific->GetValuePtr(pObject, pSubObject);
            const plUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, plVariant(), pSubObject);
            pNode->AddProperty(pProp->GetPropertyName(), SubObjectGuid);

            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
    }
    break;
    case plPropertyCategory::Array:
    {
      const plAbstractArrayProperty* pSpecific = static_cast<const plAbstractArrayProperty*>(pProp);
      plUInt32 uiCount = pSpecific->GetCount(pObject);
      plVariantArray values;
      values.SetCount(uiCount);

      if (pSpecific->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();

          plUuid guid;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          values[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), values);
      }
      else
      {
        if (bIsValueType)
        {
          for (plUInt32 i = 0; i < uiCount; ++i)
          {
            values[i] = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
        }
        else if (pSpecific->GetFlags().IsSet(plPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          for (plUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            const plUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);

            values[i] = SubObjectGuid;
          }
          pNode->AddProperty(pProp->GetPropertyName(), values);
          pPropType->GetAllocator()->Deallocate(pSubObject);
        }
      }
    }
    break;
    case plPropertyCategory::Set:
    {
      const plAbstractSetProperty* pSpecific = static_cast<const plAbstractSetProperty*>(pProp);

      plHybridArray<plVariant, 16> values;
      pSpecific->GetValues(pObject, values);

      plVariantArray ValuesCopied(values);

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < values.GetCount(); ++i)
        {
          void* pRefrencedObject = values[i].ConvertTo<void*>();

          plUuid guid;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, i, pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied[i] = guid;
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case plPropertyCategory::Map:
    {
      const plAbstractMapProperty* pSpecific = static_cast<const plAbstractMapProperty*>(pProp);

      plHybridArray<plString, 16> keys;
      pSpecific->GetKeys(pObject, keys);

      plVariantDictionary ValuesCopied;
      ValuesCopied.Reserve(keys.GetCount());

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          plVariant value = plReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
          void* pRefrencedObject = value.ConvertTo<void*>();

          plUuid guid;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            guid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, plVariant(keys[i]), pRefrencedObject);
            guid = m_pContext->EnqueObject(guid, pPropType, pRefrencedObject);
          }
          else
            guid = m_pContext->GetObjectGUID(pPropType, pRefrencedObject);

          ValuesCopied.Insert(keys[i], guid);
        }

        pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
      }
      else
      {
        if (bIsValueType)
        {
          for (plUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            plVariant value = plReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            ValuesCopied.Insert(keys[i], value);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
        {
          for (plUInt32 i = 0; i < keys.GetCount(); ++i)
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
            PLASMA_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pSubObject););
            PLASMA_VERIFY(pSpecific->GetValue(pObject, keys[i], pSubObject), "Key should be valid.");

            const plUuid SubObjectGuid = m_pContext->GenerateObjectGuid(pNode->GetGuid(), pProp, plVariant(keys[i]), pSubObject);
            AddSubObjectToGraph(pPropType, pSubObject, SubObjectGuid, nullptr);
            ValuesCopied.Insert(keys[i], SubObjectGuid);
          }
          pNode->AddProperty(pProp->GetPropertyName(), ValuesCopied);
        }
      }
    }
    break;
    case plPropertyCategory::Constant:
      // Nothing to do here.
      break;
    default:
      break;
  }
}

void plRttiConverterWriter::AddProperties(plAbstractObjectNode* pNode, const plRTTI* pRtti, const void* pObject)
{
  if (pRtti->GetParentType())
    AddProperties(pNode, pRtti->GetParentType(), pObject);

  for (const auto* pProp : pRtti->GetProperties())
  {
    AddProperty(pNode, pProp, pObject);
  }
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterWriter);
