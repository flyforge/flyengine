
// static
template <typename T, plUInt32 Size>
void plVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(plArrayPtr<const T> a, plUInt32& inout_uiAdditionalDataSize)
{
  if (a.GetCount() > Size)
  {
    inout_uiAdditionalDataSize = plMemoryUtils::AlignSize<plUInt32>(inout_uiAdditionalDataSize, PLASMA_ALIGNMENT_OF(T));
    inout_uiAdditionalDataSize += a.GetCount() * sizeof(T);
  }
}

// static
template <typename T, plUInt32 Size>
void plVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::AddAdditionalDataSize(plUInt32 uiSize, plUInt32 uiAlignment, plUInt32& inout_uiAdditionalDataSize)
{
  if (uiSize > Size * sizeof(T))
  {
    inout_uiAdditionalDataSize = plMemoryUtils::AlignSize<plUInt32>(inout_uiAdditionalDataSize, uiAlignment);
    inout_uiAdditionalDataSize += uiSize;
  }
}

template <typename T, plUInt32 Size>
T* plVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::Init(plUInt8 uiCount, plUInt8*& inout_pAdditionalData)
{
  if (uiCount <= Size)
  {
    return m_Embedded;
  }

  inout_pAdditionalData = plMemoryUtils::AlignForwards(inout_pAdditionalData, PLASMA_ALIGNMENT_OF(T));
  m_Ptr = reinterpret_cast<T*>(inout_pAdditionalData);
  inout_pAdditionalData += uiCount * sizeof(T);
  return m_Ptr;
}

template <typename T, plUInt32 Size>
plResult plVisualScriptGraphDescription::EmbeddedArrayOrPointer<T, Size>::ReadFromStream(plUInt8& out_uiCount, plStreamReader& inout_stream, plUInt8*& inout_pAdditionalData)
{
  plUInt16 uiCount = 0;
  inout_stream >> uiCount;

  if (uiCount > plMath::MaxValue<plUInt8>())
  {
    return PLASMA_FAILURE;
  }
  out_uiCount = static_cast<plUInt8>(uiCount);

  T* pTargetPtr = Init(out_uiCount, inout_pAdditionalData);
  const plUInt64 uiNumBytesToRead = uiCount * sizeof(T);
  if (inout_stream.ReadBytes(pTargetPtr, uiNumBytesToRead) != uiNumBytesToRead)
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ALWAYS_INLINE plUInt32 plVisualScriptGraphDescription::Node::GetExecutionIndex(plUInt32 uiSlot) const
{
  if (uiSlot < m_NumExecutionIndices)
  {
    return m_NumExecutionIndices <= PLASMA_ARRAY_SIZE(m_ExecutionIndices.m_Embedded) ? m_ExecutionIndices.m_Embedded[uiSlot] : m_ExecutionIndices.m_Ptr[uiSlot];
  }

  return plInvalidIndex;
}

PLASMA_ALWAYS_INLINE plVisualScriptGraphDescription::DataOffset plVisualScriptGraphDescription::Node::GetInputDataOffset(plUInt32 uiSlot) const
{
  if (uiSlot < m_NumInputDataOffsets)
  {
    return m_NumInputDataOffsets <= PLASMA_ARRAY_SIZE(m_InputDataOffsets.m_Embedded) ? m_InputDataOffsets.m_Embedded[uiSlot] : m_InputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

PLASMA_ALWAYS_INLINE plVisualScriptGraphDescription::DataOffset plVisualScriptGraphDescription::Node::GetOutputDataOffset(plUInt32 uiSlot) const
{
  if (uiSlot < m_NumOutputDataOffsets)
  {
    return m_NumOutputDataOffsets <= PLASMA_ARRAY_SIZE(m_OutputDataOffsets.m_Embedded) ? m_OutputDataOffsets.m_Embedded[uiSlot] : m_OutputDataOffsets.m_Ptr[uiSlot];
  }

  return {};
}

template <typename T>
PLASMA_ALWAYS_INLINE const T& plVisualScriptGraphDescription::Node::GetUserData() const
{
  PLASMA_ASSERT_DEBUG(m_UserDataByteSize >= sizeof(T), "Invalid data");
  return *reinterpret_cast<const T*>(m_UserDataByteSize <= sizeof(m_UserData.m_Embedded) ? m_UserData.m_Embedded : m_UserData.m_Ptr);
}

template <typename T>
T& plVisualScriptGraphDescription::Node::InitUserData(plUInt8*& inout_pAdditionalData, plUInt32 uiByteSize /*= sizeof(T)*/)
{
  m_UserDataByteSize = uiByteSize;
  auto pUserData = m_UserData.Init(uiByteSize / sizeof(plUInt32), inout_pAdditionalData);
  PLASMA_CHECK_ALIGNMENT(pUserData, PLASMA_ALIGNMENT_OF(T));
  return *reinterpret_cast<T*>(pUserData);
}

//////////////////////////////////////////////////////////////////////////

PLASMA_ALWAYS_INLINE const plVisualScriptGraphDescription::Node* plVisualScriptGraphDescription::GetNode(plUInt32 uiIndex) const
{
  return uiIndex < m_Nodes.GetCount() ? &m_Nodes.GetPtr()[uiIndex] : nullptr;
}

PLASMA_ALWAYS_INLINE bool plVisualScriptGraphDescription::IsCoroutine() const
{
  auto entryNodeType = GetNode(0)->m_Type;
  return entryNodeType == plVisualScriptNodeDescription::Type::EntryCall_Coroutine || entryNodeType == plVisualScriptNodeDescription::Type::MessageHandler_Coroutine;
}

PLASMA_ALWAYS_INLINE const plSharedPtr<const plVisualScriptDataDescription>& plVisualScriptGraphDescription::GetLocalDataDesc() const
{
  return m_pLocalDataDesc;
}

//////////////////////////////////////////////////////////////////////////

template <typename T>
PLASMA_FORCE_INLINE const T& plVisualScriptExecutionContext::GetData(DataOffset dataOffset) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetData<T>(dataOffset);
}

template <typename T>
PLASMA_FORCE_INLINE T& plVisualScriptExecutionContext::GetWritableData(DataOffset dataOffset)
{
  PLASMA_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Can't write to constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetWritableData<T>(dataOffset);
}

template <typename T>
PLASMA_FORCE_INLINE void plVisualScriptExecutionContext::SetData(DataOffset dataOffset, const T& value)
{
  PLASMA_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetData<T>(dataOffset, value);
}

PLASMA_FORCE_INLINE plTypedPointer plVisualScriptExecutionContext::GetPointerData(DataOffset dataOffset)
{
  PLASMA_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  return m_DataStorage[dataOffset.m_uiSource]->GetPointerData(dataOffset, m_uiExecutionCounter);
}

template <typename T>
PLASMA_FORCE_INLINE void plVisualScriptExecutionContext::SetPointerData(DataOffset dataOffset, T ptr, const plRTTI* pType)
{
  PLASMA_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Pointers can't be constant data");
  m_DataStorage[dataOffset.m_uiSource]->SetPointerData(dataOffset, ptr, pType, m_uiExecutionCounter);
}

PLASMA_FORCE_INLINE plVariant plVisualScriptExecutionContext::GetDataAsVariant(DataOffset dataOffset, const plRTTI* pExpectedType) const
{
  return m_DataStorage[dataOffset.m_uiSource]->GetDataAsVariant(dataOffset, pExpectedType, m_uiExecutionCounter);
}

PLASMA_FORCE_INLINE void plVisualScriptExecutionContext::SetDataFromVariant(DataOffset dataOffset, const plVariant& value)
{
  PLASMA_ASSERT_DEBUG(dataOffset.IsConstant() == false, "Outputs can't set constant data");
  return m_DataStorage[dataOffset.m_uiSource]->SetDataFromVariant(dataOffset, value, m_uiExecutionCounter);
}

PLASMA_ALWAYS_INLINE void plVisualScriptExecutionContext::SetCurrentCoroutine(plScriptCoroutine* pCoroutine)
{
  m_pCurrentCoroutine = pCoroutine;
}

inline plTime plVisualScriptExecutionContext::GetDeltaTimeSinceLastExecution()
{
  PLASMA_ASSERT_DEBUG(m_pDesc->IsCoroutine(), "Delta time is only valid for coroutines");
  return m_DeltaTimeSinceLastExecution;
}
