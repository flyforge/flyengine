#include <VisualScriptPlugin/VisualScriptPluginPCH.h>

#include <Core/Scripting/ScriptCoroutine.h>
#include <VisualScriptPlugin/Runtime/VisualScriptData.h>

namespace
{
  static const char* s_DataOffsetSourceNames[] = {
    "Local",
    "Instance",
    "Constant",
  };
  static_assert(PL_ARRAY_SIZE(s_DataOffsetSourceNames) == (size_t)plVisualScriptDataDescription::DataOffset::Source::Count);
} // namespace

// Check that DataOffset fits in one uint32 and also check that we have enough bits for dataType and source.
static_assert(sizeof(plVisualScriptDataDescription::DataOffset) == sizeof(plUInt32));
static_assert(plVisualScriptDataType::Count <= PL_BIT(plVisualScriptDataDescription::DataOffset::TYPE_BITS));
static_assert(plVisualScriptDataDescription::DataOffset::Source::Count <= PL_BIT(plVisualScriptDataDescription::DataOffset::SOURCE_BITS));

// static
const char* plVisualScriptDataDescription::DataOffset::Source::GetName(Enum source)
{
  PL_ASSERT_DEBUG(source >= 0 && source < PL_ARRAY_SIZE(s_DataOffsetSourceNames), "Out of bounds access");
  return s_DataOffsetSourceNames[source];
}

//////////////////////////////////////////////////////////////////////////

static const plTypeVersion s_uiVisualScriptDataDescriptionVersion = 1;

plResult plVisualScriptDataDescription::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiVisualScriptDataDescriptionVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream << typeInfo.m_uiStartOffset;
    inout_stream << typeInfo.m_uiCount;
  }

  inout_stream << m_uiStorageSizeNeeded;

  return PL_SUCCESS;
}

plResult plVisualScriptDataDescription::Deserialize(plStreamReader& inout_stream)
{
  plTypeVersion uiVersion = inout_stream.ReadVersion(s_uiVisualScriptDataDescriptionVersion);
  PL_IGNORE_UNUSED(uiVersion);

  for (auto& typeInfo : m_PerTypeInfo)
  {
    inout_stream >> typeInfo.m_uiStartOffset;
    inout_stream >> typeInfo.m_uiCount;
  }

  inout_stream >> m_uiStorageSizeNeeded;

  return PL_SUCCESS;
}

void plVisualScriptDataDescription::Clear()
{
  plMemoryUtils::ZeroFillArray(m_PerTypeInfo);
  m_uiStorageSizeNeeded = 0;
}

void plVisualScriptDataDescription::CalculatePerTypeStartOffsets()
{
  plUInt32 uiOffset = 0;
  for (plUInt32 i = 0; i < PL_ARRAY_SIZE(m_PerTypeInfo); ++i)
  {
    auto dataType = static_cast<plVisualScriptDataType::Enum>(i);
    auto& typeInfo = m_PerTypeInfo[i];

    if (typeInfo.m_uiCount > 0)
    {
      uiOffset = plMemoryUtils::AlignSize(uiOffset, plVisualScriptDataType::GetStorageAlignment(dataType));
      typeInfo.m_uiStartOffset = uiOffset;

      uiOffset += plVisualScriptDataType::GetStorageSize(dataType) * typeInfo.m_uiCount;
    }
  }

  m_uiStorageSizeNeeded = uiOffset;
}

//////////////////////////////////////////////////////////////////////////

plVisualScriptDataStorage::plVisualScriptDataStorage(const plSharedPtr<const plVisualScriptDataDescription>& pDesc)
  : m_pDesc(pDesc)
{
}

plVisualScriptDataStorage::~plVisualScriptDataStorage()
{
  DeallocateStorage();
}

void plVisualScriptDataStorage::AllocateStorage()
{
  m_Storage.SetCountUninitialized(m_pDesc->m_uiStorageSizeNeeded);
  m_Storage.ZeroFill();

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (plUInt32 scriptDataType = 0; scriptDataType < plVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == plVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<plString*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    if (scriptDataType == plVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<plHashedString*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Construct<SkipTrivialTypes>(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<plVariant*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Construct<SkipTrivialTypes>(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<plVariantArray*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Construct<SkipTrivialTypes>(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<plVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Construct<SkipTrivialTypes>(pVariantMaps, typeInfo.m_uiCount);
    }
  }
}

void plVisualScriptDataStorage::DeallocateStorage()
{
  if (IsAllocated() == false)
    return;

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (plUInt32 scriptDataType = 0; scriptDataType < plVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == plVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<plString*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<plHashedString*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Destruct(pStrings, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<plVariant*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Destruct(pVariants, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<plVariantArray*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Destruct(pVariantArrays, typeInfo.m_uiCount);
    }
    else if (scriptDataType == plVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<plVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      plMemoryUtils::Destruct(pVariantMaps, typeInfo.m_uiCount);
    }
  }

  m_Storage.Clear();
}

plResult plVisualScriptDataStorage::Serialize(plStreamWriter& inout_stream) const
{
  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (plUInt32 scriptDataType = 0; scriptDataType < plVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == plVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<const plString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<const plHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream << *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<const plVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream << *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<const plVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        PL_SUCCEED_OR_RETURN(inout_stream.WriteArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<const plVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        PL_SUCCEED_OR_RETURN(inout_stream.WriteHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const plUInt32 uiBytesToWrite = typeInfo.m_uiCount * plVisualScriptDataType::GetStorageSize(static_cast<plVisualScriptDataType::Enum>(scriptDataType));
      PL_SUCCEED_OR_RETURN(inout_stream.WriteBytes(pData + typeInfo.m_uiStartOffset, uiBytesToWrite));
    }
  }

  return PL_SUCCESS;
}

plResult plVisualScriptDataStorage::Deserialize(plStreamReader& inout_stream)
{
  if (IsAllocated() == false)
  {
    AllocateStorage();
  }

  auto pData = m_Storage.GetByteBlobPtr().GetPtr();

  for (plUInt32 scriptDataType = 0; scriptDataType < plVisualScriptDataType::Count; ++scriptDataType)
  {
    const auto& typeInfo = m_pDesc->m_PerTypeInfo[scriptDataType];
    if (typeInfo.m_uiCount == 0)
      continue;

    if (scriptDataType == plVisualScriptDataType::String)
    {
      auto pStrings = reinterpret_cast<plString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::HashedString)
    {
      auto pStrings = reinterpret_cast<plHashedString*>(pData + typeInfo.m_uiStartOffset);
      auto pStringsEnd = pStrings + typeInfo.m_uiCount;
      while (pStrings < pStringsEnd)
      {
        inout_stream >> *pStrings;
        ++pStrings;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Variant)
    {
      auto pVariants = reinterpret_cast<plVariant*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantsEnd = pVariants + typeInfo.m_uiCount;
      while (pVariants < pVariantsEnd)
      {
        inout_stream >> *pVariants;
        ++pVariants;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Array)
    {
      auto pVariantArrays = reinterpret_cast<plVariantArray*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantArraysEnd = pVariantArrays + typeInfo.m_uiCount;
      while (pVariantArrays < pVariantArraysEnd)
      {
        PL_SUCCEED_OR_RETURN(inout_stream.ReadArray(*pVariantArrays));
        ++pVariantArrays;
      }
    }
    else if (scriptDataType == plVisualScriptDataType::Map)
    {
      auto pVariantMaps = reinterpret_cast<plVariantDictionary*>(pData + typeInfo.m_uiStartOffset);
      auto pVariantMapsEnd = pVariantMaps + typeInfo.m_uiCount;
      while (pVariantMaps < pVariantMapsEnd)
      {
        PL_SUCCEED_OR_RETURN(inout_stream.ReadHashTable(*pVariantMaps));
        ++pVariantMaps;
      }
    }
    else
    {
      const plUInt32 uiBytesToRead = typeInfo.m_uiCount * plVisualScriptDataType::GetStorageSize(static_cast<plVisualScriptDataType::Enum>(scriptDataType));
      inout_stream.ReadBytes(pData + typeInfo.m_uiStartOffset, uiBytesToRead);
    }
  }

  return PL_SUCCESS;
}

plTypedPointer plVisualScriptDataStorage::GetPointerData(DataOffset dataOffset, plUInt32 uiExecutionCounter) const
{
  m_pDesc->CheckOffset(dataOffset, nullptr);
  auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

  if (dataOffset.m_uiType == plVisualScriptDataType::GameObject)
  {
    auto& gameObjectHandle = *reinterpret_cast<const plVisualScriptGameObjectHandle*>(pData);
    return plTypedPointer(gameObjectHandle.GetPtr(uiExecutionCounter), plGetStaticRTTI<plGameObject>());
  }
  else if (dataOffset.m_uiType == plVisualScriptDataType::Component)
  {
    auto& componentHandle = *reinterpret_cast<const plVisualScriptComponentHandle*>(pData);
    plComponent* pComponent = componentHandle.GetPtr(uiExecutionCounter);
    return plTypedPointer(pComponent, pComponent != nullptr ? pComponent->GetDynamicRTTI() : nullptr);
  }
  else if (dataOffset.m_uiType == plVisualScriptDataType::TypedPointer)
  {
    return *reinterpret_cast<const plTypedPointer*>(pData);
  }

  plTypedPointer t;
  t.m_pObject = const_cast<plUInt8*>(pData);
  t.m_pType = plVisualScriptDataType::GetRtti(static_cast<plVisualScriptDataType::Enum>(dataOffset.m_uiType));
  return t;
}

plVariant plVisualScriptDataStorage::GetDataAsVariant(DataOffset dataOffset, const plRTTI* pExpectedType, plUInt32 uiExecutionCounter) const
{
  auto scriptDataType = dataOffset.GetType();

#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  // pExpectedType == nullptr means that the caller expects an plVariant so we decide solely based on the scriptDataType.
  // We set the pExpectedType to the equivalent of the scriptDataType here so we don't need to check for pExpectedType == nullptr in all the asserts below.
  if (pExpectedType == nullptr || pExpectedType == plGetStaticRTTI<plVariant>())
  {
    pExpectedType = plVisualScriptDataType::GetRtti(scriptDataType);
  }
#endif

  switch (scriptDataType)
  {
    case plVisualScriptDataType::Invalid:
      return plVariant();

    case plVisualScriptDataType::Bool:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<bool>(), "");
      return GetData<bool>(dataOffset);

    case plVisualScriptDataType::Byte:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plUInt8>(), "");
      return GetData<plUInt8>(dataOffset);

    case plVisualScriptDataType::Int:
      if (pExpectedType == plGetStaticRTTI<plInt16>())
      {
        return static_cast<plInt16>(GetData<plInt32>(dataOffset));
      }
      else if (pExpectedType == plGetStaticRTTI<plUInt16>())
      {
        return static_cast<plUInt16>(GetData<plInt32>(dataOffset));
      }
      else if (pExpectedType == plGetStaticRTTI<plInt32>())
      {
        return GetData<plInt32>(dataOffset);
      }
      else
      {
        return static_cast<plUInt32>(GetData<plInt32>(dataOffset));
      }
      PL_ASSERT_NOT_IMPLEMENTED;

    case plVisualScriptDataType::Int64:
      PL_ASSERT_DEBUG(pExpectedType->GetTypeFlags().IsSet(plTypeFlags::IsEnum) || pExpectedType == plGetStaticRTTI<plInt64>(), "");
      return GetData<plInt64>(dataOffset);

    case plVisualScriptDataType::Float:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<float>(), "");
      return GetData<float>(dataOffset);

    case plVisualScriptDataType::Double:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<double>(), "");
      return GetData<double>(dataOffset);

    case plVisualScriptDataType::Color:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plColor>(), "");
      return GetData<plColor>(dataOffset);

    case plVisualScriptDataType::Vector3:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plVec3>(), "");
      return GetData<plVec3>(dataOffset);

    case plVisualScriptDataType::Quaternion:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plQuat>(), "");
      return GetData<plQuat>(dataOffset);

    case plVisualScriptDataType::Transform:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plTransform>(), "");
      return GetData<plTransform>(dataOffset);

    case plVisualScriptDataType::Time:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plTime>(), "");
      return GetData<plTime>(dataOffset);

    case plVisualScriptDataType::Angle:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plAngle>(), "");
      return GetData<plAngle>(dataOffset);

    case plVisualScriptDataType::String:
      if (pExpectedType == nullptr || pExpectedType == plGetStaticRTTI<plString>() || pExpectedType == plGetStaticRTTI<const char*>())
      {
        return GetData<plString>(dataOffset);
      }
      else if (pExpectedType == plGetStaticRTTI<plStringView>())
      {
        return plVariant(GetData<plString>(dataOffset).GetView(), false);
      }
      PL_ASSERT_NOT_IMPLEMENTED;

    case plVisualScriptDataType::HashedString:
      if (pExpectedType == nullptr || pExpectedType == plGetStaticRTTI<plHashedString>())
      {
        return GetData<plHashedString>(dataOffset);
      }
      else if (pExpectedType == plGetStaticRTTI<plTempHashedString>())
      {
        return plTempHashedString(GetData<plHashedString>(dataOffset));
      }
      PL_ASSERT_NOT_IMPLEMENTED;

    case plVisualScriptDataType::GameObject:
      if (pExpectedType == nullptr || pExpectedType == plGetStaticRTTI<plGameObject>())
      {
        return GetPointerData(dataOffset, uiExecutionCounter);
      }
      else if (pExpectedType == plGetStaticRTTI<plGameObjectHandle>())
      {
        return GetData<plGameObjectHandle>(dataOffset);
      }
      PL_ASSERT_NOT_IMPLEMENTED;

    case plVisualScriptDataType::Component:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plComponentHandle>(), "");
      return GetData<plComponentHandle>(dataOffset);

    case plVisualScriptDataType::TypedPointer:
      return GetPointerData(dataOffset, uiExecutionCounter);

    case plVisualScriptDataType::Variant:
      return GetData<plVariant>(dataOffset);

    case plVisualScriptDataType::Array:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plVariantArray>(), "");
      return GetData<plVariantArray>(dataOffset);

    case plVisualScriptDataType::Map:
      PL_ASSERT_DEBUG(pExpectedType == plGetStaticRTTI<plVariantDictionary>(), "");
      return GetData<plVariantDictionary>(dataOffset);

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return plVariant();
}

void plVisualScriptDataStorage::SetDataFromVariant(DataOffset dataOffset, const plVariant& value, plUInt32 uiExecutionCounter)
{
  if (dataOffset.IsValid() == false)
    return;

  auto scriptDataType = dataOffset.GetType();
  switch (scriptDataType)
  {
    case plVisualScriptDataType::Bool:
      SetData(dataOffset, value.Get<bool>());
      break;
    case plVisualScriptDataType::Byte:
      if (value.IsA<plInt8>())
      {
        SetData(dataOffset, plUInt8(value.Get<plInt8>()));
      }
      else
      {
        SetData(dataOffset, value.Get<plUInt8>());
      }
      break;
    case plVisualScriptDataType::Int:
      if (value.IsA<plInt16>())
      {
        SetData(dataOffset, plInt32(value.Get<plInt16>()));
      }
      else if (value.IsA<plUInt16>())
      {
        SetData(dataOffset, plInt32(value.Get<plUInt16>()));
      }
      else if (value.IsA<plInt32>())
      {
        SetData(dataOffset, value.Get<plInt32>());
      }
      else
      {
        SetData(dataOffset, plInt32(value.Get<plUInt32>()));
      }
      break;
    case plVisualScriptDataType::Int64:
      if (value.IsA<plInt64>())
      {
        SetData(dataOffset, value.Get<plInt64>());
      }
      else
      {
        SetData(dataOffset, plInt64(value.Get<plUInt64>()));
      }
      break;
    case plVisualScriptDataType::Float:
      SetData(dataOffset, value.Get<float>());
      break;
    case plVisualScriptDataType::Double:
      SetData(dataOffset, value.Get<double>());
      break;
    case plVisualScriptDataType::Color:
      SetData(dataOffset, value.Get<plColor>());
      break;
    case plVisualScriptDataType::Vector3:
      SetData(dataOffset, value.Get<plVec3>());
      break;
    case plVisualScriptDataType::Quaternion:
      SetData(dataOffset, value.Get<plQuat>());
      break;
    case plVisualScriptDataType::Transform:
      SetData(dataOffset, value.Get<plTransform>());
      break;
    case plVisualScriptDataType::Time:
      SetData(dataOffset, value.Get<plTime>());
      break;
    case plVisualScriptDataType::Angle:
      SetData(dataOffset, value.Get<plAngle>());
      break;
    case plVisualScriptDataType::String:
      if (value.IsA<plStringView>())
      {
        SetData(dataOffset, plString(value.Get<plStringView>()));
      }
      else
      {
        SetData(dataOffset, value.Get<plString>());
      }
      break;
    case plVisualScriptDataType::HashedString:
      if (value.IsA<plTempHashedString>())
      {
        PL_ASSERT_NOT_IMPLEMENTED;
      }
      else
      {
        SetData(dataOffset, value.Get<plHashedString>());
      }
      break;
    case plVisualScriptDataType::GameObject:
      if (value.IsA<plGameObjectHandle>())
      {
        SetData(dataOffset, value.Get<plGameObjectHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<plGameObject*>(), plGetStaticRTTI<plGameObject>(), uiExecutionCounter);
      }
      break;
    case plVisualScriptDataType::Component:
      if (value.IsA<plComponentHandle>())
      {
        SetData(dataOffset, value.Get<plComponentHandle>());
      }
      else
      {
        SetPointerData(dataOffset, value.Get<plComponent*>(), plGetStaticRTTI<plComponent>(), uiExecutionCounter);
      }
      break;
    case plVisualScriptDataType::TypedPointer:
    {
      plTypedPointer typedPtr = value.Get<plTypedPointer>();
      SetPointerData(dataOffset, typedPtr.m_pObject, typedPtr.m_pType, uiExecutionCounter);
    }
    break;
    case plVisualScriptDataType::Variant:
      SetData(dataOffset, value);
      break;
    case plVisualScriptDataType::Array:
      SetData(dataOffset, value.Get<plVariantArray>());
      break;
    case plVisualScriptDataType::Map:
      SetData(dataOffset, value.Get<plVariantDictionary>());
      break;
    case plVisualScriptDataType::Coroutine:
      SetData(dataOffset, value.Get<plScriptCoroutineHandle>());
      break;

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
