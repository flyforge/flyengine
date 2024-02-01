#pragma once

#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Tag.h>


// Template specialization to be able to use plTagSet properties as PL_SET_MEMBER_PROPERTY.
template <typename T>
struct plContainerSubTypeResolver<plTagSetTemplate<T>>
{
  using Type = const char*;
};

// Template specialization to be able to use plTagSet properties as PL_SET_MEMBER_PROPERTY.
template <typename Class>
class plMemberSetProperty<Class, plTagSet, const char*> : public plTypedSetProperty<typename plTypeTraits<const char*>::NonConstReferenceType>
{
public:
  using Container = plTagSet;
  using Type = plConstCharPtr;
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class*);
  using GetContainerFunc = Container& (*)(Class*);

  plMemberSetProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : plTypedSetProperty<RealType>(szPropertyName)
  {
    PL_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an set property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      plAbstractSetProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual bool IsEmpty(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    PL_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    PL_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    PL_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const set accessor function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveByName(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return m_ConstGetter(static_cast<const Class*>(pInstance)).IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, plDynamicArray<plVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : m_ConstGetter(static_cast<const Class*>(pInstance)))
    {
      out_keys.PushBack(plVariant(value.GetTagString()));
    }
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

// Template specialization to be able to use plTagSet properties as PL_SET_ACCESSOR_PROPERTY.
template <typename Class>
class plAccessorSetProperty<Class, const char*, const plTagSet&> : public plTypedSetProperty<const char*>
{
public:
  using Container = const plTagSet&;
  using Type = plConstCharPtr;

  using ContainerType = typename plTypeTraits<Container>::NonConstReferenceType;
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;

  using InsertFunc = void (Class::*)(Type value);
  using RemoveFunc = void (Class::*)(Type value);
  using GetValuesFunc = Container (Class::*)() const;

  plAccessorSetProperty(const char* szPropertyName, GetValuesFunc getValues, InsertFunc insert, RemoveFunc remove)
    : plTypedSetProperty<Type>(szPropertyName)
  {
    PL_ASSERT_DEBUG(getValues != nullptr, "The get values function of an set property cannot be nullptr.");

    m_GetValues = getValues;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Insert == nullptr || m_Remove == nullptr)
      plAbstractSetProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }


  virtual bool IsEmpty(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsEmpty(); }

  virtual void Clear(void* pInstance) const override
  {
    PL_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is read-only",
      plAbstractProperty::GetPropertyName());

    // We must not cache the container c here as the Remove can make it invalid
    // e.g. plArrayPtr by value.
    while (!IsEmpty(pInstance))
    {
      // this should be decltype(auto) c = ...; but MSVC 16 is too dumb for that (MSVC 15 works fine)
      decltype((static_cast<const Class*>(pInstance)->*m_GetValues)()) c = (static_cast<const Class*>(pInstance)->*m_GetValues)();
      auto it = cbegin(c);
      const plTag& value = *it;
      Remove(pInstance, value.GetTagString().GetData());
    }
  }

  virtual void Insert(void* pInstance, const void* pObject) const override
  {
    PL_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(*static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, const void* pObject) const override
  {
    PL_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(*static_cast<const RealType*>(pObject));
  }

  virtual bool Contains(const void* pInstance, const void* pObject) const override
  {
    return (static_cast<const Class*>(pInstance)->*m_GetValues)().IsSetByName(*static_cast<const RealType*>(pObject));
  }

  virtual void GetValues(const void* pInstance, plDynamicArray<plVariant>& out_keys) const override
  {
    out_keys.Clear();
    for (const auto& value : (static_cast<const Class*>(pInstance)->*m_GetValues)())
    {
      out_keys.PushBack(plVariant(value.GetTagString()));
    }
  }

private:
  GetValuesFunc m_GetValues;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};


template <typename BlockStorageAllocator>
plTagSetTemplate<BlockStorageAllocator>::Iterator::Iterator(const plTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd)
  : m_pTagSet(pSet)

{
  if (!bEnd)
  {
    m_uiIndex = m_pTagSet->GetTagBlockStart() * (sizeof(plTagSetBlockStorage) * 8);

    if (m_pTagSet->IsEmpty())
      m_uiIndex = 0xFFFFFFFF;
    else
    {
      if (!IsBitSet())
        operator++();
    }
  }
  else
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::Iterator::IsBitSet() const
{
  plTag TempTag;
  TempTag.m_uiBlockIndex = m_uiIndex / (sizeof(plTagSetBlockStorage) * 8);
  TempTag.m_uiBitIndex = m_uiIndex - (TempTag.m_uiBlockIndex * sizeof(plTagSetBlockStorage) * 8);

  return m_pTagSet->IsSet(TempTag);
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::Iterator::operator++()
{
  const plUInt32 uiMax = m_pTagSet->GetTagBlockEnd() * (sizeof(plTagSetBlockStorage) * 8);

  do
  {
    ++m_uiIndex;
  } while (m_uiIndex < uiMax && !IsBitSet());

  if (m_uiIndex >= uiMax)
    m_uiIndex = 0xFFFFFFFF;
}

template <typename BlockStorageAllocator>
const plTag& plTagSetTemplate<BlockStorageAllocator>::Iterator::operator*() const
{
  return *plTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
const plTag* plTagSetTemplate<BlockStorageAllocator>::Iterator::operator->() const
{
  return plTagRegistry::GetGlobalRegistry().GetTagByIndex(m_uiIndex);
}

template <typename BlockStorageAllocator>
plTagSetTemplate<BlockStorageAllocator>::plTagSetTemplate()
{
  SetTagBlockStart(plSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::operator==(const plTagSetTemplate& other) const
{
  return m_TagBlocks == other.m_TagBlocks && m_TagBlocks.template GetUserData<plUInt32>() == other.m_TagBlocks.template GetUserData<plUInt32>();
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::operator!=(const plTagSetTemplate& other) const
{
  return !(*this == other);
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::Set(const plTag& tag)
{
  PL_ASSERT_DEV(tag.IsValid(), "Only valid tags can be set in a tag set!");

  if (m_TagBlocks.IsEmpty())
  {
    Reallocate(tag.m_uiBlockIndex, tag.m_uiBlockIndex);
  }
  else if (IsTagInAllocatedRange(tag) == false)
  {
    const plUInt32 uiNewBlockStart = plMath::Min<plUInt32>(tag.m_uiBlockIndex, GetTagBlockStart());
    const plUInt32 uiNewBlockEnd = plMath::Max<plUInt32>(tag.m_uiBlockIndex, GetTagBlockEnd());

    Reallocate(uiNewBlockStart, uiNewBlockEnd);
  }

  plUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

  const plUInt64 bitMask = PL_BIT(tag.m_uiBitIndex);
  const bool bBitWasSet = ((tagBlock & bitMask) != 0);

  tagBlock |= bitMask;

  if (!bBitWasSet)
  {
    IncreaseTagCount();
  }
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::Remove(const plTag& tag)
{
  PL_ASSERT_DEV(tag.IsValid(), "Only valid tags can be cleared from a tag set!");

  if (IsTagInAllocatedRange(tag))
  {
    plUInt64& tagBlock = m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()];

    const plUInt64 bitMask = PL_BIT(tag.m_uiBitIndex);
    const bool bBitWasSet = ((tagBlock & bitMask) != 0);

    tagBlock &= ~bitMask;

    if (bBitWasSet)
    {
      DecreaseTagCount();
    }
  }
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::IsSet(const plTag& tag) const
{
  PL_ASSERT_DEV(tag.IsValid(), "Only valid tags can be checked!");

  if (IsTagInAllocatedRange(tag))
  {
    return (m_TagBlocks[tag.m_uiBlockIndex - GetTagBlockStart()] & PL_BIT(tag.m_uiBitIndex)) != 0;
  }
  else
  {
    return false;
  }
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::IsAnySet(const plTagSetTemplate& otherSet) const
{
  // If any of the sets is empty nothing can match
  if (IsEmpty() || otherSet.IsEmpty())
    return false;

  // Calculate range to compare
  const plUInt32 uiMaxBlockStart = plMath::Max(GetTagBlockStart(), otherSet.GetTagBlockStart());
  const plUInt32 uiMinBlockEnd = plMath::Min(GetTagBlockEnd(), otherSet.GetTagBlockEnd());

  if (uiMaxBlockStart > uiMinBlockEnd)
    return false;

  for (plUInt32 i = uiMaxBlockStart; i < uiMinBlockEnd; ++i)
  {
    const plUInt32 uiThisBlockStorageIndex = i - GetTagBlockStart();
    const plUInt32 uiOtherBlockStorageIndex = i - otherSet.GetTagBlockStart();

    if ((m_TagBlocks[uiThisBlockStorageIndex] & otherSet.m_TagBlocks[uiOtherBlockStorageIndex]) != 0)
    {
      return true;
    }
  }

  return false;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plUInt32 plTagSetTemplate<BlockStorageAllocator>::GetNumTagsSet() const
{
  return GetTagCount();
}

template <typename BlockStorageAllocator>
PL_ALWAYS_INLINE bool plTagSetTemplate<BlockStorageAllocator>::IsEmpty() const
{
  return GetTagCount() == 0;
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::Clear()
{
  m_TagBlocks.Clear();
  SetTagBlockStart(plSmallInvalidIndex);
  SetTagCount(0);
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::SetByName(plStringView sTag)
{
  const plTag& tag = plTagRegistry::GetGlobalRegistry().RegisterTag(sTag);
  Set(tag);
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::RemoveByName(plStringView sTag)
{
  if (const plTag* tag = plTagRegistry::GetGlobalRegistry().GetTagByName(plTempHashedString(sTag)))
  {
    Remove(*tag);
  }
}

template <typename BlockStorageAllocator>
bool plTagSetTemplate<BlockStorageAllocator>::IsSetByName(plStringView sTag) const
{
  if (const plTag* tag = plTagRegistry::GetGlobalRegistry().GetTagByName(plTempHashedString(sTag)))
  {
    return IsSet(*tag);
  }

  return false;
}

template <typename BlockStorageAllocator>
PL_ALWAYS_INLINE bool plTagSetTemplate<BlockStorageAllocator>::IsTagInAllocatedRange(const plTag& Tag) const
{
  return Tag.m_uiBlockIndex >= GetTagBlockStart() && Tag.m_uiBlockIndex < GetTagBlockEnd();
}

template <typename BlockStorageAllocator>
void plTagSetTemplate<BlockStorageAllocator>::Reallocate(plUInt32 uiNewTagBlockStart, plUInt32 uiNewMaxBlockIndex)
{
  PL_ASSERT_DEV(uiNewTagBlockStart < plSmallInvalidIndex, "Tag block start is too big");
  const plUInt16 uiNewBlockArraySize = static_cast<plUInt16>((uiNewMaxBlockIndex - uiNewTagBlockStart) + 1);

  // Early out for non-filled tag sets
  if (m_TagBlocks.IsEmpty())
  {
    m_TagBlocks.SetCount(uiNewBlockArraySize);
    SetTagBlockStart(static_cast<plUInt16>(uiNewTagBlockStart));

    return;
  }

  PL_ASSERT_DEBUG(uiNewTagBlockStart <= GetTagBlockStart(), "New block start must be smaller or equal to current block start!");

  plSmallArray<plUInt64, 32, BlockStorageAllocator> helperArray;
  helperArray.SetCount(uiNewBlockArraySize);

  const plUInt32 uiOldBlockStartOffset = GetTagBlockStart() - uiNewTagBlockStart;

  // Copy old data to the new array
  plMemoryUtils::Copy(helperArray.GetData() + uiOldBlockStartOffset, m_TagBlocks.GetData(), m_TagBlocks.GetCount());

  // Use array ptr copy assignment so it doesn't modify the user data in m_TagBlocks
  m_TagBlocks = helperArray.GetArrayPtr();
  SetTagBlockStart(static_cast<plUInt16>(uiNewTagBlockStart));
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plUInt16 plTagSetTemplate<BlockStorageAllocator>::GetTagBlockStart() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plUInt16 plTagSetTemplate<BlockStorageAllocator>::GetTagBlockEnd() const
{
  return static_cast<plUInt16>(GetTagBlockStart() + m_TagBlocks.GetCount());
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plTagSetTemplate<BlockStorageAllocator>::SetTagBlockStart(plUInt16 uiTagBlockStart)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagBlockStart = uiTagBlockStart;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE plUInt16 plTagSetTemplate<BlockStorageAllocator>::GetTagCount() const
{
  return m_TagBlocks.template GetUserData<UserData>().m_uiTagCount;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plTagSetTemplate<BlockStorageAllocator>::SetTagCount(plUInt16 uiTagCount)
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount = uiTagCount;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plTagSetTemplate<BlockStorageAllocator>::IncreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount++;
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
PL_ALWAYS_INLINE void plTagSetTemplate<BlockStorageAllocator>::DecreaseTagCount()
{
  m_TagBlocks.template GetUserData<UserData>().m_uiTagCount--;
}

static plTypeVersion s_TagSetVersion = 1;

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
void plTagSetTemplate<BlockStorageAllocator>::Save(plStreamWriter& inout_stream) const
{
  const plUInt16 uiNumTags = static_cast<plUInt16>(GetNumTagsSet());
  inout_stream << uiNumTags;

  inout_stream.WriteVersion(s_TagSetVersion);

  for (Iterator it = GetIterator(); it.IsValid(); ++it)
  {
    const plTag& tag = *it;

    inout_stream << tag.m_sTagString;
  }
}

template <typename BlockStorageAllocator /*= plDefaultAllocatorWrapper*/>
void plTagSetTemplate<BlockStorageAllocator>::Load(plStreamReader& inout_stream, plTagRegistry& inout_registry)
{
  plUInt16 uiNumTags = 0;
  inout_stream >> uiNumTags;

  // Manually read version value since 0 can be a valid version here
  plTypeVersion version;
  inout_stream.ReadWordValue(&version).IgnoreResult();

  if (version == 0)
  {
    for (plUInt32 i = 0; i < uiNumTags; ++i)
    {
      plUInt32 uiTagMurmurHash = 0;
      inout_stream >> uiTagMurmurHash;

      if (const plTag* pTag = inout_registry.GetTagByMurmurHash(uiTagMurmurHash))
      {
        Set(*pTag);
      }
    }
  }
  else
  {
    for (plUInt32 i = 0; i < uiNumTags; ++i)
    {
      plHashedString tagString;
      inout_stream >> tagString;

      const plTag& tag = inout_registry.RegisterTag(tagString);
      Set(tag);
    }
  }
}
