
PL_FORCE_INLINE void plVisualScriptDataDescription::CheckOffset(DataOffset dataOffset, const plRTTI* pType) const
{
#if PL_ENABLED(PL_COMPILE_FOR_DEBUG)
  auto givenDataType = dataOffset.GetType();
  auto& offsetAndCount = m_PerTypeInfo[givenDataType];
  PL_ASSERT_DEBUG(offsetAndCount.m_uiCount > 0, "Invalid data offset");
  const plUInt32 uiLastOffset = offsetAndCount.m_uiStartOffset + (offsetAndCount.m_uiCount - 1) * plVisualScriptDataType::GetStorageSize(givenDataType);
  PL_ASSERT_DEBUG(dataOffset.m_uiByteOffset >= offsetAndCount.m_uiStartOffset && dataOffset.m_uiByteOffset <= uiLastOffset, "Invalid data offset");

  if (pType != nullptr)
  {
    auto expectedDataType = plVisualScriptDataType::FromRtti(pType);
    PL_ASSERT_DEBUG(expectedDataType == givenDataType, "Data type mismatch, expected '{}'({}) but got '{}'", plVisualScriptDataType::GetName(expectedDataType), pType->GetTypeName(), plVisualScriptDataType::GetName(givenDataType));
  }
#endif
}

PL_FORCE_INLINE plVisualScriptDataDescription::DataOffset plVisualScriptDataDescription::GetOffset(plVisualScriptDataType::Enum dataType, plUInt32 uiIndex, DataOffset::Source::Enum source) const
{
  auto& offsetAndCount = m_PerTypeInfo[dataType];
  plUInt32 uiByteOffset = plInvalidIndex;
  if (uiIndex < offsetAndCount.m_uiCount)
  {
    uiByteOffset = offsetAndCount.m_uiStartOffset + uiIndex * plVisualScriptDataType::GetStorageSize(dataType);
  }

  return DataOffset(uiByteOffset, dataType, source);
}

//////////////////////////////////////////////////////////////////////////

PL_ALWAYS_INLINE bool plVisualScriptDataStorage::IsAllocated() const
{
  return m_Storage.GetByteBlobPtr().IsEmpty() == false;
}

template <typename T>
const T& plVisualScriptDataStorage::GetData(DataOffset dataOffset) const
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, plTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, plGetStaticRTTI<T>());

  return *reinterpret_cast<const T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
T& plVisualScriptDataStorage::GetWritableData(DataOffset dataOffset)
{
  static_assert(!std::is_pointer<T>::value && !std::is_same<T, plTypedPointer>::value, "Use GetPointerData instead");

  m_pDesc->CheckOffset(dataOffset, plGetStaticRTTI<T>());

  return *reinterpret_cast<T*>(m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset);
}

template <typename T>
void plVisualScriptDataStorage::SetData(DataOffset dataOffset, const T& value)
{
  static_assert(!std::is_pointer<T>::value, "Use SetPointerData instead");

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    m_pDesc->CheckOffset(dataOffset, plGetStaticRTTI<T>());

    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, plGameObjectHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<plVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, plComponentHandle>::value)
    {
      auto& storedHandle = *reinterpret_cast<plVisualScriptComponentHandle*>(pData);
      storedHandle.AssignHandle(value);
    }
    else if constexpr (std::is_same<T, plStringView>::value)
    {
      *reinterpret_cast<plString*>(pData) = value;
    }
    else
    {
      *reinterpret_cast<T*>(pData) = value;
    }
  }
}

template <typename T>
void plVisualScriptDataStorage::SetPointerData(DataOffset dataOffset, T ptr, const plRTTI* pType, plUInt32 uiExecutionCounter)
{
  static_assert(std::is_pointer<T>::value);

  if (dataOffset.m_uiByteOffset < m_Storage.GetByteBlobPtr().GetCount())
  {
    auto pData = m_Storage.GetByteBlobPtr().GetPtr() + dataOffset.m_uiByteOffset;

    if constexpr (std::is_same<T, plGameObject*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, plGetStaticRTTI<plGameObject>());

      auto& storedHandle = *reinterpret_cast<plVisualScriptGameObjectHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else if constexpr (std::is_same<T, plComponent*>::value)
    {
      m_pDesc->CheckOffset(dataOffset, plGetStaticRTTI<plComponent>());

      auto& storedHandle = *reinterpret_cast<plVisualScriptComponentHandle*>(pData);
      storedHandle.AssignPtr(ptr, uiExecutionCounter);
    }
    else
    {
      PL_ASSERT_DEBUG(!pType || pType->IsDerivedFrom<plComponent>() == false, "Component type '{}' is stored as typed pointer, cast to plComponent first to ensure correct storage", pType->GetTypeName());

      m_pDesc->CheckOffset(dataOffset, pType);

      auto& typedPointer = *reinterpret_cast<plTypedPointer*>(pData);
      typedPointer.m_pObject = ptr;
      typedPointer.m_pType = pType;
    }
  }
}

//////////////////////////////////////////////////////////////////////////

inline plResult plVisualScriptInstanceData::Serialize(plStreamWriter& inout_stream) const
{
  PL_SUCCEED_OR_RETURN(m_DataOffset.Serialize(inout_stream));
  inout_stream << m_DefaultValue;
  return PL_SUCCESS;
}

inline plResult plVisualScriptInstanceData::Deserialize(plStreamReader& inout_stream)
{
  PL_SUCCEED_OR_RETURN(m_DataOffset.Deserialize(inout_stream));
  inout_stream >> m_DefaultValue;
  return PL_SUCCESS;
}
