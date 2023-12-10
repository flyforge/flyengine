#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>

plRttiConverterReader::plRttiConverterReader(const plAbstractObjectGraph* pGraph, plRttiConverterContext* pContext)
{
  m_pGraph = pGraph;
  m_pContext = pContext;
}

plInternal::NewInstance<void> plRttiConverterReader::CreateObjectFromNode(const plAbstractObjectNode* pNode)
{
  const plRTTI* pRtti = m_pContext->FindTypeByName(pNode->GetType());
  if (pRtti == nullptr)
  {
    m_pContext->OnUnknownTypeError(pNode->GetType());
    return nullptr;
  }

  auto pObject = m_pContext->CreateObject(pNode->GetGuid(), pRtti);
  if (pObject)
  {
    ApplyPropertiesToObject(pNode, pRtti, pObject);
  }

  CallOnObjectCreated(pNode, pRtti, pObject);
  return pObject;
}

void plRttiConverterReader::ApplyPropertiesToObject(const plAbstractObjectNode* pNode, const plRTTI* pRtti, void* pObject)
{
  PLASMA_ASSERT_DEBUG(pNode != nullptr, "Invalid node");

  if (pRtti->GetParentType() != nullptr)
    ApplyPropertiesToObject(pNode, pRtti->GetParentType(), pObject);

  for (auto* prop : pRtti->GetProperties())
  {
    auto* pOtherProp = pNode->FindProperty(prop->GetPropertyName());
    if (pOtherProp == nullptr)
      continue;

    ApplyProperty(pObject, prop, pOtherProp);
  }
}

void plRttiConverterReader::ApplyProperty(void* pObject, const plAbstractProperty* pProp, const plAbstractObjectNode::Property* pSource)
{
  const plRTTI* pPropType = pProp->GetSpecificType();

  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const plAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        if (!pSource->m_Value.IsA<plUuid>())
          return;

        plUuid guid = pSource->m_Value.Get<plUuid>();
        void* pRefrencedObject = nullptr;

        if (guid.IsValid())
        {
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              // plLog::Error("Failed to set property '{0}', type could not be created!", pProp->GetPropertyName());
              return;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
        }

        void* pOldObject = nullptr;
        pSpecific->GetValuePtr(pObject, &pOldObject);
        pSpecific->SetValuePtr(pObject, &pRefrencedObject);
        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          plReflectionUtils::DeleteObject(pOldObject, pProp);
      }
      else
      {
        if (bIsValueType || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        {
          plReflectionUtils::SetMemberPropertyValue(pSpecific, pObject, pSource->m_Value);
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
        {
          if (!pSource->m_Value.IsA<plUuid>())
            return;

          void* pDirectPtr = pSpecific->GetPropertyPointer(pObject);
          bool bDelete = false;
          const plUuid sourceGuid = pSource->m_Value.Get<plUuid>();

          if (pDirectPtr == nullptr)
          {
            bDelete = true;
            pDirectPtr = m_pContext->CreateObject(sourceGuid, pPropType);
          }

          auto* pNode = m_pGraph->GetNode(sourceGuid);
          PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");

          ApplyPropertiesToObject(pNode, pPropType, pDirectPtr);

          if (bDelete)
          {
            pSpecific->SetValuePtr(pObject, pDirectPtr);
            m_pContext->DeleteObject(sourceGuid);
          }
        }
      }
    }
    break;
    case plPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const plAbstractArrayProperty*>(pProp);
      if (!pSource->m_Value.IsA<plVariantArray>())
        return;
      const plVariantArray& array = pSource->m_Value.Get<plVariantArray>();
      // Delete old values
      if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        const plInt32 uiOldCount = (plInt32)pSpecific->GetCount(pObject);
        for (plInt32 i = uiOldCount - 1; i >= 0; --i)
        {
          void* pOldObject = nullptr;
          pSpecific->GetValue(pObject, i, &pOldObject);
          pSpecific->Remove(pObject, i);
          if (pOldObject)
            plReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->SetCount(pObject, array.GetCount());
      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<plUuid>())
            continue;
          plUuid guid = array[i].Get<plUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                plLog::Error("Failed to set array property '{0}' element, type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->SetValue(pObject, i, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (plUInt32 i = 0; i < array.GetCount(); ++i)
          {
            plReflectionUtils::SetArrayPropertyValue(pSpecific, pObject, i, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(plPropertyFlags::Class))
        {
          const plUuid temp = plUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (plUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<plUuid>())
              continue;

            const plUuid sourceGuid = array[i].Get<plUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->SetValue(pObject, i, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case plPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const plAbstractSetProperty*>(pProp);
      if (!pSource->m_Value.IsA<plVariantArray>())
        return;

      const plVariantArray& array = pSource->m_Value.Get<plVariantArray>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        plHybridArray<plVariant, 16> keys;
        pSpecific->GetValues(pObject, keys);
        pSpecific->Clear(pObject);
        for (plVariant& value : keys)
        {
          void* pOldObject = value.ConvertTo<void*>();
          if (pOldObject)
            plReflectionUtils::DeleteObject(pOldObject, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < array.GetCount(); ++i)
        {
          if (!array[i].IsA<plUuid>())
            continue;

          plUuid guid = array[i].Get<plUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            auto* pNode = m_pGraph->GetNode(guid);
            PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");
            pRefrencedObject = CreateObjectFromNode(pNode);
            if (pRefrencedObject == nullptr)
            {
              plLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
              continue;
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (plUInt32 i = 0; i < array.GetCount(); ++i)
          {
            plReflectionUtils::InsertSetPropertyValue(pSpecific, pObject, array[i]);
          }
        }
        else if (pProp->GetFlags().IsAnySet(plPropertyFlags::Class))
        {
          const plUuid temp = plUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (plUInt32 i = 0; i < array.GetCount(); ++i)
          {
            if (!array[i].IsA<plUuid>())
              continue;

            const plUuid sourceGuid = array[i].Get<plUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;
    case plPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const plAbstractMapProperty*>(pProp);
      if (!pSource->m_Value.IsA<plVariantDictionary>())
        return;

      const plVariantDictionary& dict = pSource->m_Value.Get<plVariantDictionary>();

      // Delete old values
      if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        plHybridArray<plString, 16> keys;
        pSpecific->GetKeys(pObject, keys);
        for (const plString& sKey : keys)
        {
          plVariant value = plReflectionUtils::GetMapPropertyValue(pSpecific, pObject, sKey);
          void* pOldClone = value.ConvertTo<void*>();
          pSpecific->Remove(pObject, sKey);
          if (pOldClone)
            plReflectionUtils::DeleteObject(pOldClone, pProp);
        }
      }

      pSpecific->Clear(pObject);

      if (pProp->GetFlags().IsAnySet(plPropertyFlags::Pointer))
      {
        for (auto it = dict.GetIterator(); it.IsValid(); ++it)
        {
          if (!it.Value().IsA<plUuid>())
            continue;

          plUuid guid = it.Value().Get<plUuid>();
          void* pRefrencedObject = nullptr;
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            if (guid.IsValid())
            {
              auto* pNode = m_pGraph->GetNode(guid);
              PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");
              pRefrencedObject = CreateObjectFromNode(pNode);
              if (pRefrencedObject == nullptr)
              {
                plLog::Error("Failed to insert set element into property '{0}', type could not be created!", pProp->GetPropertyName());
                continue;
              }
            }
          }
          else
          {
            pRefrencedObject = m_pContext->GetObjectByGUID(guid).m_pObject;
          }
          pSpecific->Insert(pObject, it.Key(), &pRefrencedObject);
        }
      }
      else
      {
        if (bIsValueType)
        {
          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            plReflectionUtils::SetMapPropertyValue(pSpecific, pObject, it.Key(), it.Value());
          }
        }
        else if (pProp->GetFlags().IsAnySet(plPropertyFlags::Class))
        {
          const plUuid temp = plUuid::MakeUuid();

          void* pValuePtr = m_pContext->CreateObject(temp, pPropType);

          for (auto it = dict.GetIterator(); it.IsValid(); ++it)
          {
            if (!it.Value().IsA<plUuid>())
              continue;

            const plUuid sourceGuid = it.Value().Get<plUuid>();
            auto* pNode = m_pGraph->GetNode(sourceGuid);
            PLASMA_ASSERT_DEV(pNode != nullptr, "node must exist");

            ApplyPropertiesToObject(pNode, pPropType, pValuePtr);
            pSpecific->Insert(pObject, it.Key(), pValuePtr);
          }

          m_pContext->DeleteObject(temp);
        }
      }
    }
    break;

    default:
      PLASMA_ASSERT_NOT_IMPLEMENTED;
      break;
  }
}

void plRttiConverterReader::CallOnObjectCreated(const plAbstractObjectNode* pNode, const plRTTI* pRtti, void* pObject)
{
  auto functions = pRtti->GetFunctions();
  for (auto pFunc : functions)
  {
    // TODO: Make this compare faster
    if (plStringUtils::IsEqual(pFunc->GetPropertyName(), "OnObjectCreated"))
    {
      plHybridArray<plVariant, 1> params;
      params.PushBack(plVariant(pNode));
      plVariant ret;
      pFunc->Execute(pObject, params, ret);
    }
  }
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_RttiConverterReader);
