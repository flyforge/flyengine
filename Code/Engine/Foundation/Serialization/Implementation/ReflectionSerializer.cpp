#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Serialization/BinarySerializer.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

////////////////////////////////////////////////////////////////////////
// plReflectionSerializer public static functions
////////////////////////////////////////////////////////////////////////

void plReflectionSerializer::WriteObjectToDDL(plStreamWriter& inout_stream, const plRTTI* pRtti, const void* pObject, bool bCompactMmode /*= true*/, plOpenDdlWriter::TypeStringMode typeMode /*= plOpenDdlWriter::TypeStringMode::Shortest*/)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;
  plRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(plUuid::CreateUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  plAbstractGraphDdlSerializer::Write(inout_stream, &graph, nullptr, bCompactMmode, typeMode);
}

void plReflectionSerializer::WriteObjectToDDL(plOpenDdlWriter& ref_ddl, const plRTTI* pRtti, const void* pObject, plUuid guid /*= plUuid()*/)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;
  plRttiConverterWriter conv(&graph, &context, false, true);

  if (!guid.IsValid())
  {
    guid = plUuid::CreateUuid();
  }

  context.RegisterObject(guid, pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  plAbstractGraphDdlSerializer::Write(ref_ddl, &graph, nullptr);
}

void plReflectionSerializer::WriteObjectToBinary(plStreamWriter& inout_stream, const plRTTI* pRtti, const void* pObject)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;
  plRttiConverterWriter conv(&graph, &context, false, true);

  context.RegisterObject(plUuid::CreateUuid(), pRtti, const_cast<void*>(pObject));
  conv.AddObjectToGraph(pRtti, const_cast<void*>(pObject), "root");

  plAbstractGraphBinarySerializer::Write(inout_stream, &graph);
}

void* plReflectionSerializer::ReadObjectFromDDL(plStreamReader& inout_stream, const plRTTI*& ref_pRtti)
{
  plOpenDdlReader reader;
  if (reader.ParseDocument(inout_stream, 0, plLog::GetThreadLocalLogSystem()).Failed())
  {
    plLog::Error("Failed to parse DDL graph");
    return nullptr;
  }

  return ReadObjectFromDDL(reader.GetRootElement(), ref_pRtti);
}

void* plReflectionSerializer::ReadObjectFromDDL(const plOpenDdlReaderElement* pRootElement, const plRTTI*& ref_pRtti)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;

  plAbstractGraphDdlSerializer::Read(pRootElement, &graph).IgnoreResult();

  plRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  PLASMA_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = plRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void* plReflectionSerializer::ReadObjectFromBinary(plStreamReader& inout_stream, const plRTTI*& ref_pRtti)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;

  plAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  plRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  PLASMA_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  ref_pRtti = plRTTI::FindTypeByName(pRootNode->GetType());

  void* pTarget = context.CreateObject(pRootNode->GetGuid(), ref_pRtti);

  convRead.ApplyPropertiesToObject(pRootNode, ref_pRtti, pTarget);

  return pTarget;
}

void plReflectionSerializer::ReadObjectPropertiesFromDDL(plStreamReader& inout_stream, const plRTTI& rtti, void* pObject)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;

  plAbstractGraphDdlSerializer::Read(inout_stream, &graph).IgnoreResult();

  plRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  PLASMA_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  if (pRootNode == nullptr)
    return;

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}

void plReflectionSerializer::ReadObjectPropertiesFromBinary(plStreamReader& inout_stream, const plRTTI& rtti, void* pObject)
{
  plAbstractObjectGraph graph;
  plRttiConverterContext context;

  plAbstractGraphBinarySerializer::Read(inout_stream, &graph);

  plRttiConverterReader convRead(&graph, &context);
  auto* pRootNode = graph.GetNodeByName("root");

  PLASMA_ASSERT_DEV(pRootNode != nullptr, "invalid document");

  convRead.ApplyPropertiesToObject(pRootNode, &rtti, pObject);
}


namespace
{
  static void CloneProperty(const void* pObject, void* pClone, const plAbstractProperty* pProp)
  {
    if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
      return;

    const plRTTI* pPropType = pProp->GetSpecificType();

    const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

    plVariant vTemp;
    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const plAbstractMemberProperty*>(pProp);

        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);

          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner) && pRefrencedObject)
          {
            pRefrencedObject = plReflectionSerializer::Clone(pRefrencedObject, pPropType);
            vTemp = plVariant(pRefrencedObject, pPropType);
          }

          plVariant vOldValue = plReflectionUtils::GetMemberPropertyValue(pSpecific, pClone);
          plReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
            plReflectionUtils::DeleteObject(vOldValue.ConvertTo<void*>(), pProp);
        }
        else
        {
          if (bIsValueType || pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
          {
            vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
            plReflectionUtils::SetMemberPropertyValue(pSpecific, pClone, vTemp);
          }
          else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
          {
            void* pSubObject = pSpecific->GetPropertyPointer(pObject);
            // Do we have direct access to the property?
            if (pSubObject != nullptr)
            {
              void* pSubClone = pSpecific->GetPropertyPointer(pClone);
              plReflectionSerializer::Clone(pSubObject, pSubClone, pPropType);
            }
            // If the property is behind an accessor, we need to retrieve it first.
            else if (pPropType->GetAllocator()->CanAllocate())
            {
              pSubObject = pPropType->GetAllocator()->Allocate<void>();
              pSpecific->GetValuePtr(pObject, pSubObject);
              pSpecific->SetValuePtr(pClone, pSubObject);
              pPropType->GetAllocator()->Deallocate(pSubObject);
            }
          }
        }
      }
      break;
      case plPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const plAbstractArrayProperty*>(pProp);
        // Delete old values
        if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
        {
          const plInt32 iCloneCount = (plInt32)pSpecific->GetCount(pClone);
          for (plInt32 i = iCloneCount - 1; i >= 0; --i)
          {
            void* pOldSubClone = nullptr;
            pSpecific->GetValue(pClone, i, &pOldSubClone);
            pSpecific->Remove(pClone, i);
            if (pOldSubClone)
              plReflectionUtils::DeleteObject(pOldSubClone, pProp);
          }
        }

        const plUInt32 uiCount = pSpecific->GetCount(pObject);
        pSpecific->SetCount(pClone, uiCount);
        if (pSpecific->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          for (plUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            void* pRefrencedObject = vTemp.ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = plReflectionSerializer::Clone(pRefrencedObject, pPropType);
              vTemp = plVariant(pRefrencedObject, pPropType);
            }
            plReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
          }
        }
        else
        {
          if (bIsValueType)
          {
            for (plUInt32 i = 0; i < uiCount; ++i)
            {
              vTemp = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
              plReflectionUtils::SetArrayPropertyValue(pSpecific, pClone, i, vTemp);
            }
          }
          else if (pProp->GetFlags().IsSet(plPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
          {
            void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

            for (plUInt32 i = 0; i < uiCount; ++i)
            {
              pSpecific->GetValue(pObject, i, pSubObject);
              pSpecific->SetValue(pClone, i, pSubObject);
            }

            pPropType->GetAllocator()->Deallocate(pSubObject);
          }
        }
      }
      break;
      case plPropertyCategory::Set:
      {
        auto pSpecific = static_cast<const plAbstractSetProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
        {
          plHybridArray<plVariant, 16> keys;
          pSpecific->GetValues(pClone, keys);
          pSpecific->Clear(pClone);
          for (plVariant& value : keys)
          {
            void* pOldClone = value.ConvertTo<void*>();
            if (pOldClone)
              plReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        plHybridArray<plVariant, 16> values;
        pSpecific->GetValues(pObject, values);


        if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          for (plUInt32 i = 0; i < values.GetCount(); ++i)
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner) && pRefrencedObject)
            {
              pRefrencedObject = plReflectionSerializer::Clone(pRefrencedObject, pPropType);
            }
            vTemp = plVariant(pRefrencedObject, pPropType);
            plReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, vTemp);
          }
        }
        else if (bIsValueType)
        {
          for (plUInt32 i = 0; i < values.GetCount(); ++i)
          {
            plReflectionUtils::InsertSetPropertyValue(pSpecific, pClone, values[i]);
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const plAbstractMapProperty*>(pProp);

        // Delete old values
        if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
        {
          plHybridArray<plString, 16> keys;
          pSpecific->GetKeys(pClone, keys);
          for (const plString& sKey : keys)
          {
            plVariant value = plReflectionUtils::GetMapPropertyValue(pSpecific, pClone, sKey);
            void* pOldClone = value.ConvertTo<void*>();
            pSpecific->Remove(pClone, sKey);
            if (pOldClone)
              plReflectionUtils::DeleteObject(pOldClone, pProp);
          }
        }
        pSpecific->Clear(pClone);

        plHybridArray<plString, 16> keys;
        pSpecific->GetKeys(pObject, keys);

        for (plUInt32 i = 0; i < keys.GetCount(); ++i)
        {
          if (bIsValueType ||
              (pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
          {
            plVariant value = plReflectionUtils::GetMapPropertyValue(pSpecific, pObject, keys[i]);
            plReflectionUtils::SetMapPropertyValue(pSpecific, pClone, keys[i], value);
          }
          else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
          {
            if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
            {
              void* pValue = nullptr;
              pSpecific->GetValue(pObject, keys[i], &pValue);
              pValue = plReflectionSerializer::Clone(pValue, pPropType);
              pSpecific->Insert(pClone, keys[i], &pValue);
            }
            else
            {
              if (pPropType->GetAllocator()->CanAllocate())
              {
                void* pValue = pPropType->GetAllocator()->Allocate<void>();
                PLASMA_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(pValue););
                PLASMA_VERIFY(pSpecific->GetValue(pObject, keys[i], pValue), "Previously retrieved key does not exist.");
                pSpecific->Insert(pClone, keys[i], pValue);
              }
              else
              {
                plLog::Error("The property '{0}' can not be cloned as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
              }
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }

  static void CloneProperties(const void* pObject, void* pClone, const plRTTI* pType)
  {
    if (pType->GetParentType())
      CloneProperties(pObject, pClone, pType->GetParentType());

    for (auto* pProp : pType->GetProperties())
    {
      CloneProperty(pObject, pClone, pProp);
    }
  }
} // namespace

void* plReflectionSerializer::Clone(const void* pObject, const plRTTI* pType)
{
  if (!pObject)
    return nullptr;

  PLASMA_ASSERT_DEV(pType != nullptr, "invalid type.");
  if (pType->IsDerivedFrom<plReflectedClass>())
  {
    const plReflectedClass* pRefObject = static_cast<const plReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  PLASMA_ASSERT_DEV(pType->GetAllocator()->CanAllocate(), "The type '{0}' can't be cloned!", pType->GetTypeName());
  void* pClone = pType->GetAllocator()->Allocate<void>();
  CloneProperties(pObject, pClone, pType);
  return pClone;
}


void plReflectionSerializer::Clone(const void* pObject, void* pClone, const plRTTI* pType)
{
  PLASMA_ASSERT_DEV(pObject && pClone && pType, "invalid type.");
  if (pType->IsDerivedFrom<plReflectedClass>())
  {
    const plReflectedClass* pRefObject = static_cast<const plReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
    PLASMA_ASSERT_DEV(pType == static_cast<plReflectedClass*>(pClone)->GetDynamicRTTI(), "Object '{0}' and clone '{1}' have mismatching types!", pType->GetTypeName(), static_cast<plReflectedClass*>(pClone)->GetDynamicRTTI()->GetTypeName());
  }

  CloneProperties(pObject, pClone, pType);
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_Serialization_Implementation_ReflectionSerializer);
