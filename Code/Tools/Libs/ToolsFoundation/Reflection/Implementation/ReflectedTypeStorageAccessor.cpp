#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Types/Status.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Reflection/ReflectedTypeStorageAccessor.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>


////////////////////////////////////////////////////////////////////////
// plReflectedTypeStorageAccessor public functions
////////////////////////////////////////////////////////////////////////

plReflectedTypeStorageAccessor::plReflectedTypeStorageAccessor(const plRTTI* pRtti, plDocumentObject* pOwner)
  : plIReflectedTypeAccessor(pRtti, pOwner)
{
  const plRTTI* pType = pRtti;
  PLASMA_ASSERT_DEV(pType != nullptr, "Trying to construct an plReflectedTypeStorageAccessor for an invalid type!");
  m_pMapping = plReflectedTypeStorageManager::AddStorageAccessor(this);
  PLASMA_ASSERT_DEV(m_pMapping != nullptr, "The type for this plReflectedTypeStorageAccessor is unknown to the plReflectedTypeStorageManager!");

  auto& indexTable = m_pMapping->m_PathToStorageInfoTable;
  const plUInt32 uiProperties = indexTable.GetCount();
  // To prevent re-allocs due to new properties being added we reserve 20% more space.
  m_Data.Reserve(uiProperties + uiProperties / 20);
  m_Data.SetCount(uiProperties);

  // Fill data storage with default values for the given types.
  for (auto it = indexTable.GetIterator(); it.IsValid(); ++it)
  {
    const auto storageInfo = it.Value();
    m_Data[storageInfo.m_uiIndex] = storageInfo.m_DefaultValue;
  }
}

plReflectedTypeStorageAccessor::~plReflectedTypeStorageAccessor()
{
  plReflectedTypeStorageManager::RemoveStorageAccessor(this);
}

const plVariant plReflectedTypeStorageAccessor::GetValue(const char* szProperty, plVariant index, plStatus* pRes) const
{
  const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
  if (pProp == nullptr)
  {
    if (pRes)
      *pRes = plStatus(plFmt("Property '{0}' not found in type '{1}'", szProperty, GetType()->GetTypeName()));
    return plVariant();
  }

  if (pRes)
    *pRes = plStatus(PLASMA_SUCCESS);
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
        return m_Data[storageInfo->m_uiIndex];
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        if (!index.IsValid())
        {
          return m_Data[storageInfo->m_uiIndex];
        }

        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        if (index.CanConvertTo<plUInt32>())
        {
          plUInt32 uiIndex = index.ConvertTo<plUInt32>();
          if (uiIndex < values.GetCount())
          {
            return values[uiIndex];
          }
        }
        if (pRes)
          *pRes = plStatus(plFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, szProperty));
      }
      break;
      case plPropertyCategory::Map:
      {
        if (!index.IsValid())
        {
          return m_Data[storageInfo->m_uiIndex];
        }

        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        if (index.IsA<plString>())
        {
          const plString& sIndex = index.Get<plString>();
          if (const plVariant* pValue = values.GetValue(sIndex))
          {
            return *pValue;
          }
        }
        if (pRes)
          *pRes = plStatus(plFmt("Index '{0}' for property '{1}' is invalid or out of bounds.", index, szProperty));
      }
      break;
      default:
        break;
    }
  }
  return plVariant();
}

bool plReflectedTypeStorageAccessor::SetValue(const char* szProperty, const plVariant& value, plVariant index)
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;
    PLASMA_ASSERT_DEV(pProp->GetSpecificType() == plGetStaticRTTI<plVariant>() || value.IsValid(), "");

    if (storageInfo->m_Type == plVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = plReflectionUtils::IsValueType(pProp);
    const plVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(plPropertyFlags::Class) && !isValueType) ? plVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
      {
        if (value.IsA<plString>() && pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags))
        {
          plInt64 iValue;
          plReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<plString>(), iValue);
          m_Data[storageInfo->m_uiIndex] = plVariant(iValue).ConvertTo(storageInfo->m_Type);
          return true;
        }
        else if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
        {
          m_Data[storageInfo->m_uiIndex] = value;
          return true;
        }
        else if (value.CanConvertTo(storageInfo->m_Type))
        {
          // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
          // that may have a different type now as someone reloaded the type information and replaced a type.
          m_Data[storageInfo->m_uiIndex] = value.ConvertTo(storageInfo->m_Type);
          return true;
        }
      }
      break;
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        if (index.CanConvertTo<plUInt32>())
        {
          plUInt32 uiIndex = index.ConvertTo<plUInt32>();
          if (uiIndex < values.GetCount())
          {
            plVariantArray changedValues = values;
            if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
            {
              changedValues[uiIndex] = value;
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else
            {
              if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
              {
                m_Data[storageInfo->m_uiIndex] = value;
                return true;
              }
              else if (value.CanConvertTo(SpecVarType))
              {
                // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
                // that may have a different type now as someone reloaded the type information and replaced a type.
                changedValues[uiIndex] = value.ConvertTo(SpecVarType);
                m_Data[storageInfo->m_uiIndex] = changedValues;
                return true;
              }
            }
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        if (index.IsA<plString>() && values.Contains(index.Get<plString>()))
        {
          const plString& sIndex = index.Get<plString>();
          plVariantDictionary changedValues = values;
          if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
          {
            changedValues[sIndex] = value;
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else
          {
            if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues[sIndex] = value.ConvertTo(SpecVarType);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

plInt32 plReflectedTypeStorageAccessor::GetCount(const char* szProperty) const
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return -1;

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return -1;

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        return values.GetCount();
      }
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        return values.GetCount();
      }
      default:
        break;
    }
  }
  return -1;
}

bool plReflectedTypeStorageAccessor::GetKeys(const char* szProperty, plDynamicArray<plVariant>& out_keys) const
{
  out_keys.Clear();

  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return false;

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        out_keys.Reserve(values.GetCount());
        for (plUInt32 i = 0; i < values.GetCount(); ++i)
        {
          out_keys.PushBack(i);
        }
        return true;
      }
      break;
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        out_keys.Reserve(values.GetCount());
        for (auto it = values.GetIterator(); it.IsValid(); ++it)
        {
          out_keys.PushBack(plVariant(it.Key()));
        }
        return true;
      }
      break;

      default:
        break;
    }
  }
  return false;
}
bool plReflectedTypeStorageAccessor::InsertValue(const char* szProperty, plVariant index, const plVariant& value)
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return false;

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    if (storageInfo->m_Type == plVariantType::TypedObject && storageInfo->m_DefaultValue.GetReflectedType() != value.GetReflectedType())
    {
      // Typed objects must match exactly.
      return false;
    }

    const bool isValueType = plReflectionUtils::IsValueType(pProp);
    const plVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(plPropertyFlags::Class) && !isValueType) ? plVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        if (index.CanConvertTo<plUInt32>())
        {
          plUInt32 uiIndex = index.ConvertTo<plUInt32>();
          if (uiIndex <= values.GetCount())
          {
            plVariantArray changedValues = values;
            if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
            {
              changedValues.Insert(value, uiIndex);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
            else if (value.CanConvertTo(SpecVarType))
            {
              // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
              // that may have a different type now as someone reloaded the type information and replaced a type.
              changedValues.Insert(value.ConvertTo(SpecVarType), uiIndex);
              m_Data[storageInfo->m_uiIndex] = changedValues;
              return true;
            }
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        if (index.IsA<plString>() && !values.Contains(index.Get<plString>()))
        {
          const plString& sIndex = index.Get<plString>();
          plVariantDictionary changedValues = values;
          if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
          {
            changedValues.Insert(sIndex, value);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
          else if (value.CanConvertTo(SpecVarType))
          {
            // We are lenient here regarding the type, as we may have stored values in the undo-redo stack
            // that may have a different type now as someone reloaded the type information and replaced a type.
            changedValues.Insert(sIndex, value.ConvertTo(SpecVarType));
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool plReflectedTypeStorageAccessor::RemoveValue(const char* szProperty, plVariant index)
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return false;

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        if (index.CanConvertTo<plUInt32>())
        {
          plUInt32 uiIndex = index.ConvertTo<plUInt32>();
          if (uiIndex < values.GetCount())
          {
            plVariantArray changedValues = values;
            changedValues.RemoveAtAndCopy(uiIndex);
            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        if (index.IsA<plString>() && values.Contains(index.Get<plString>()))
        {
          const plString& sIndex = index.Get<plString>();
          plVariantDictionary changedValues = values;
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
      }
      break;
      default:
        break;
    }
  }
  return false;
}

bool plReflectedTypeStorageAccessor::MoveValue(const char* szProperty, plVariant oldIndex, plVariant newIndex)
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return false;

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return false;

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
        if (oldIndex.CanConvertTo<plUInt32>() && newIndex.CanConvertTo<plUInt32>())
        {
          plUInt32 uiOldIndex = oldIndex.ConvertTo<plUInt32>();
          plUInt32 uiNewIndex = newIndex.ConvertTo<plUInt32>();
          if (uiOldIndex < values.GetCount() && uiNewIndex <= values.GetCount())
          {
            plVariantArray changedValues = values;
            plVariant value = changedValues[uiOldIndex];
            changedValues.RemoveAtAndCopy(uiOldIndex);
            if (uiNewIndex > uiOldIndex)
            {
              uiNewIndex -= 1;
            }
            changedValues.Insert(value, uiNewIndex);

            m_Data[storageInfo->m_uiIndex] = changedValues;
            return true;
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
        if (oldIndex.IsA<plString>() && values.Contains(oldIndex.Get<plString>()) && newIndex.IsA<plString>())
        {
          const plString& sIndex = oldIndex.Get<plString>();
          plVariantDictionary changedValues = values;
          changedValues.Insert(newIndex.Get<plString>(), changedValues[sIndex]);
          changedValues.Remove(sIndex);
          m_Data[storageInfo->m_uiIndex] = changedValues;
          return true;
        }
      }
      default:
        break;
    }
  }
  return false;
}

plVariant plReflectedTypeStorageAccessor::GetPropertyChildIndex(const char* szProperty, const plVariant& value) const
{
  const plReflectedTypeStorageManager::ReflectedTypeStorageMapping::StorageInfo* storageInfo = nullptr;
  if (m_pMapping->m_PathToStorageInfoTable.TryGetValue(szProperty, storageInfo))
  {
    if (storageInfo->m_Type == plVariant::Type::Invalid)
      return plVariant();

    const plAbstractProperty* pProp = GetType()->FindPropertyByName(szProperty);
    if (pProp == nullptr)
      return plVariant();

    const bool isValueType = plReflectionUtils::IsValueType(pProp);
    const plVariantType::Enum SpecVarType = pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || (pProp->GetFlags().IsSet(plPropertyFlags::Class) && !isValueType) ? plVariantType::Uuid : pProp->GetSpecificType()->GetVariantType();

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const plVariantArray& values = m_Data[storageInfo->m_uiIndex].Get<plVariantArray>();
          for (plUInt32 i = 0; i < values.GetCount(); i++)
          {
            if (values[i] == value)
              return plVariant((plUInt32)i);
          }
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        if (value.CanConvertTo(SpecVarType))
        {
          const plVariantDictionary& values = m_Data[storageInfo->m_uiIndex].Get<plVariantDictionary>();
          for (auto it = values.GetIterator(); it.IsValid(); ++it)
          {
            if (it.Value() == value)
              return plVariant(it.Key());
          }
        }
      }
      break;
      default:
        break;
    }
  }
  return plVariant();
}
