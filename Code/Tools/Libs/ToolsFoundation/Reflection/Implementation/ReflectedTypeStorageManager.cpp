#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageManager.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>

plMap<const plRTTI*, plReflectedTypeStorageManager::ReflectedTypeStorageMapping*> plReflectedTypeStorageManager::s_ReflectedTypeToStorageMapping;

// clang-format off
// 
PL_BEGIN_SUBSYSTEM_DECLARATION(ToolsFoundation, ReflectedTypeStorageManager)

  BEGIN_SUBSYSTEM_DEPENDENCIES
  "Foundation",
  "ReflectedTypeManager"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORESYSTEMS_STARTUP
  {
    plReflectedTypeStorageManager::Startup();
  }

  ON_CORESYSTEMS_SHUTDOWN
  {
    plReflectedTypeStorageManager::Shutdown();
  }

PL_END_SUBSYSTEM_DECLARATION;
// clang-format on

////////////////////////////////////////////////////////////////////////
// plReflectedTypeStorageManager::ReflectedTypeStorageMapping public functions
////////////////////////////////////////////////////////////////////////

void plReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddProperties(const plRTTI* pType)
{
  // Mark all properties as invalid. Thus, when a property is dropped we know it is no longer valid.
  // All others will be set to their old or new value by the AddPropertiesRecursive function.
  for (auto it = m_PathToStorageInfoTable.GetIterator(); it.IsValid(); ++it)
  {
    it.Value().m_Type = plVariant::Type::Invalid;
  }

  plSet<const plDocumentObject*> requiresPatchingEmbeddedClass;
  AddPropertiesRecursive(pType, requiresPatchingEmbeddedClass);

  for (const plDocumentObject* pObject : requiresPatchingEmbeddedClass)
  {
    pObject->GetDocumentObjectManager()->PatchEmbeddedClassObjects(pObject);
  }
}

void plReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertiesRecursive(
  const plRTTI* pType, plSet<const plDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  // Parse parent class
  const plRTTI* pParent = pType->GetParentType();
  if (pParent != nullptr)
    AddPropertiesRecursive(pParent, ref_requiresPatchingEmbeddedClass);

  // Parse properties
  const plUInt32 uiPropertyCount = pType->GetProperties().GetCount();
  for (plUInt32 i = 0; i < uiPropertyCount; ++i)
  {
    const plAbstractProperty* pProperty = pType->GetProperties()[i];

    plString path = pProperty->GetPropertyName();

    StorageInfo* storageInfo = nullptr;
    if (m_PathToStorageInfoTable.TryGetValue(path, storageInfo))
    {
      // Value already present, update type and instances
      storageInfo->m_Type = GetStorageType(pProperty);
      storageInfo->m_DefaultValue = plToolsReflectionUtils::GetStorageDefault(pProperty);
      UpdateInstances(storageInfo->m_uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
    else
    {
      const plUInt16 uiIndex = (plUInt16)m_PathToStorageInfoTable.GetCount();

      // Add value, new entries are appended
      m_PathToStorageInfoTable.Insert(path, StorageInfo(uiIndex, GetStorageType(pProperty), plToolsReflectionUtils::GetStorageDefault(pProperty)));
      AddPropertyToInstances(uiIndex, pProperty, ref_requiresPatchingEmbeddedClass);
    }
  }
}

void plReflectedTypeStorageManager::ReflectedTypeStorageMapping::UpdateInstances(
  plUInt32 uiIndex, const plAbstractProperty* pProperty, plSet<const plDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    plDynamicArray<plVariant>& data = it.Key()->m_Data;
    PL_ASSERT_DEV(uiIndex < data.GetCount(), "plReflectedTypeStorageAccessor found with fewer properties that is should have!");
    plVariant& value = data[uiIndex];

    const auto SpecVarType = GetStorageType(pProperty);

    switch (pProperty->GetCategory())
    {
      case plPropertyCategory::Member:
      {
        if (pProperty->GetFlags().IsSet(plPropertyFlags::Class) && !pProperty->GetFlags().IsSet(plPropertyFlags::Pointer))
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            if (!value.Get<plUuid>().IsValid())
            {
              ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
            }
          }
          else
          {
            value = plToolsReflectionUtils::GetStorageDefault(pProperty);
            ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
          }
          continue;
        }
        else
        {
          // Did the type change from what it was previously?
          if (value.GetType() == SpecVarType)
          {
            // The types are equal so nothing needs to be done. The current value will stay valid.
            // This should be the most common case.
            continue;
          }
          else
          {
            // The type is new or has changed but we have a valid value stored. Assume that the type of a property was changed
            // and try to convert the value.
            if (value.CanConvertTo(SpecVarType))
            {
              value = value.ConvertTo(SpecVarType);
            }
            else
            {
              value = plToolsReflectionUtils::GetStorageDefault(pProperty);
            }
            continue;
          }
        }
      }
      break;
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        if (value.GetType() != plVariantType::VariantArray)
        {
          value = plVariantArray();
          continue;
        }
        plVariantArray values = value.Get<plVariantArray>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for plPropertyCategory::Member, but for each element instead.
        for (plUInt32 i = 0; i < values.GetCount(); i++)
        {
          plVariant& var = values[i];
          if (var.GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            plResult res(PL_FAILURE);
            var = var.ConvertTo(SpecVarType, &res);
            if (res == PL_FAILURE)
            {
              var = plReflectionUtils::GetDefaultValue(pProperty, i);
            }
          }
        }
        value = values;
      }
      break;
      case plPropertyCategory::Map:
      {
        if (value.GetType() != plVariantType::VariantDictionary)
        {
          value = plVariantDictionary();
          continue;
        }
        plVariantDictionary values = value.Get<plVariantDictionary>();
        if (values.IsEmpty())
          continue;

        // Same conversion logic as for plPropertyCategory::Member, but for each element instead.
        for (auto it2 = values.GetIterator(); it2.IsValid(); ++it2)
        {
          if (it2.Value().GetType() == SpecVarType)
          {
            continue;
          }
          else
          {
            plResult res(PL_FAILURE);
            it2.Value() = it2.Value().ConvertTo(SpecVarType, &res);
            if (res == PL_FAILURE)
            {
              it2.Value() = plReflectionUtils::GetDefaultValue(pProperty, it2.Key());
            }
          }
        }
        value = values;
      }
      break;
      default:
        break;
    }
  }
}

void plReflectedTypeStorageManager::ReflectedTypeStorageMapping::AddPropertyToInstances(
  plUInt32 uiIndex, const plAbstractProperty* pProperty, plSet<const plDocumentObject*>& ref_requiresPatchingEmbeddedClass)
{
  if (pProperty->GetCategory() != plPropertyCategory::Member)
    return;

  for (auto it = m_Instances.GetIterator(); it.IsValid(); ++it)
  {
    plDynamicArray<plVariant>& data = it.Key()->m_Data;
    PL_ASSERT_DEV(data.GetCount() == uiIndex, "plReflectedTypeStorageAccessor found with a property count that does not match its storage mapping!");
    data.PushBack(plToolsReflectionUtils::GetStorageDefault(pProperty));
    if (pProperty->GetFlags().IsSet(plPropertyFlags::Class) && !pProperty->GetFlags().IsSet(plPropertyFlags::Pointer))
    {
      ref_requiresPatchingEmbeddedClass.Insert(it.Key()->GetOwner());
    }
  }
}


plVariantType::Enum plReflectedTypeStorageManager::ReflectedTypeStorageMapping::GetStorageType(const plAbstractProperty* pProperty)
{
  plVariantType::Enum type = plVariantType::Uuid;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      if (bIsValueType)
        type = pProperty->GetSpecificType()->GetVariantType();
      else if (pProperty->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        type = plVariantType::Int64;
    }
    break;
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      type = plVariantType::VariantArray;
    }
    break;
    case plPropertyCategory::Map:
    {
      type = plVariantType::VariantDictionary;
    }
    break;
    default:
      break;
  }

  return type;
}

////////////////////////////////////////////////////////////////////////
// plReflectedTypeStorageManager private functions
////////////////////////////////////////////////////////////////////////

void plReflectedTypeStorageManager::Startup()
{
  plPhantomRttiManager::s_Events.AddEventHandler(TypeEventHandler);
}

void plReflectedTypeStorageManager::Shutdown()
{
  plPhantomRttiManager::s_Events.RemoveEventHandler(TypeEventHandler);

  for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
  {
    ReflectedTypeStorageMapping* pMapping = it.Value();

    for (auto inst : pMapping->m_Instances)
    {
      plLog::Error("Type '{0}' survived shutdown!", inst->GetType()->GetTypeName());
    }

    PL_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
    PL_DEFAULT_DELETE(pMapping);
  }
  s_ReflectedTypeToStorageMapping.Clear();
}

const plReflectedTypeStorageManager::ReflectedTypeStorageMapping* plReflectedTypeStorageManager::AddStorageAccessor(
  plReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Insert(pInstance);
  return pMapping;
}

void plReflectedTypeStorageManager::RemoveStorageAccessor(plReflectedTypeStorageAccessor* pInstance)
{
  ReflectedTypeStorageMapping* pMapping = GetTypeStorageMapping(pInstance->GetType());
  pMapping->m_Instances.Remove(pInstance);
}

plReflectedTypeStorageManager::ReflectedTypeStorageMapping* plReflectedTypeStorageManager::GetTypeStorageMapping(const plRTTI* pType)
{
  PL_ASSERT_DEV(pType != nullptr, "Nullptr is not a valid type!");
  auto it = s_ReflectedTypeToStorageMapping.Find(pType);
  if (it.IsValid())
    return it.Value();

  ReflectedTypeStorageMapping* pMapping = PL_DEFAULT_NEW(ReflectedTypeStorageMapping);
  pMapping->AddProperties(pType);
  s_ReflectedTypeToStorageMapping[pType] = pMapping;
  return pMapping;
}

void plReflectedTypeStorageManager::TypeEventHandler(const plPhantomRttiManagerEvent& e)
{
  switch (e.m_Type)
  {
    case plPhantomRttiManagerEvent::Type::TypeAdded:
    {
      const plRTTI* pType = e.m_pChangedType;
      PL_ASSERT_DEV(pType != nullptr, "A type was added but it has an invalid handle!");

      PL_ASSERT_DEV(!s_ReflectedTypeToStorageMapping.Find(e.m_pChangedType).IsValid(), "The type '{0}' was added twice!", pType->GetTypeName());
      GetTypeStorageMapping(e.m_pChangedType);
    }
    break;
    case plPhantomRttiManagerEvent::Type::TypeChanged:
    {
      const plRTTI* pNewType = e.m_pChangedType;
      PL_ASSERT_DEV(pNewType != nullptr, "A type was updated but its handle is invalid!");

      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      PL_ASSERT_DEV(pMapping != nullptr, "A type was updated but no mapping exists for it!");

      if (pNewType->GetParentType() != nullptr && pNewType->GetParentType()->GetTypeName() == "plEnumBase")
      {
        // PL_ASSERT_DEV(false, "Updating enums not implemented yet!");
        break;
      }
      else if (pNewType->GetParentType() != nullptr && pNewType->GetParentType()->GetTypeName() == "plBitflagsBase")
      {
        PL_ASSERT_DEV(false, "Updating bitflags not implemented yet!");
      }

      pMapping->AddProperties(pNewType);

      plSet<plRTTI*> dependencies;
      // Update all types that either derive from the changed type or have the type as a member.
      for (auto it = s_ReflectedTypeToStorageMapping.GetIterator(); it.IsValid(); ++it)
      {
        if (it.Key() == e.m_pChangedType)
          continue;

        const plRTTI* pType = it.Key();
        if (pType->IsDerivedFrom(e.m_pChangedType))
        {
          it.Value()->AddProperties(pType);
        }
      }
    }
    break;
    case plPhantomRttiManagerEvent::Type::TypeRemoved:
    {
      ReflectedTypeStorageMapping* pMapping = s_ReflectedTypeToStorageMapping[e.m_pChangedType];
      PL_ASSERT_DEV(pMapping != nullptr, "A type was removed but no mapping ever exited for it!");
      PL_ASSERT_DEV(pMapping->m_Instances.IsEmpty(), "A type was removed which still has instances using the type!");
      s_ReflectedTypeToStorageMapping.Remove(e.m_pChangedType);
      PL_DEFAULT_DELETE(pMapping);
    }
    break;
  }
}
