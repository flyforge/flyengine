
#pragma once

#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/TagRegistry.h>

class plTag;
using plTagSetBlockStorage = plUInt64;

/// \brief A dynamic collection of tags featuring fast lookups.
///
/// This class can be used to store a (dynamic) collection of tags. Tags are registered within
/// the global tag registry and allocated a bit index. The tag set allows comparatively fast lookups
/// to check if a given tag is in the set or not.
/// Adding a tag may have some overhead depending whether the block storage for the tag
/// bit indices needs to be expanded or not (if the storage needs to be expanded the hybrid array will be resized).
/// Typical storage requirements for a given tag set instance should be small since the block storage is a sliding
/// window. The standard class which can be used is plTagSet, usage of plTagSetTemplate is only necessary
/// if the allocator needs to be overridden.
template <typename BlockStorageAllocator = plDefaultAllocatorWrapper>
class plTagSetTemplate
{
public:
  plTagSetTemplate();

  bool operator==(const plTagSetTemplate& other) const;
  bool operator!=(const plTagSetTemplate& other) const;

  /// \brief Adds the given tag to the set.
  void Set(const plTag& tag); // [tested]

  /// \brief Removes the given tag.
  void Remove(const plTag& tag); // [tested]

  /// \brief Returns true, if the given tag is in the set.
  bool IsSet(const plTag& tag) const; // [tested]

  /// \brief Returns true if this tag set contains any tag set in the given other tag set.
  bool IsAnySet(const plTagSetTemplate& otherSet) const; // [tested]

  /// \brief Returns how many tags are in this set.
  plUInt32 GetNumTagsSet() const;

  /// \brief True if the tag set never contained any tag or was cleared.
  bool IsEmpty() const;

  /// \brief Removes all tags from the set
  void Clear();

  /// \brief Adds the tag with the given name. If the tag does not exist, it will be registered.
  void SetByName(plStringView sTag);

  /// \brief Removes the given tag. If it doesn't exist, nothing happens.
  void RemoveByName(plStringView sTag);

  /// \brief Checks whether the named tag is part of this set. Returns false if the tag does not exist.
  bool IsSetByName(plStringView sTag) const;

  /// \brief Allows to iterate over all tags in this set
  class Iterator
  {
  public:
    Iterator(const plTagSetTemplate<BlockStorageAllocator>* pSet, bool bEnd = false);

    /// \brief Returns a reference to the current tag
    const plTag& operator*() const;

    /// \brief Returns a pointer to the current tag
    const plTag* operator->() const;

    /// \brief Returns whether the iterator is still pointing to a valid item
    PLASMA_ALWAYS_INLINE bool IsValid() const { return m_uiIndex != 0xFFFFFFFF; }

    PLASMA_ALWAYS_INLINE bool operator!=(const Iterator& rhs) const { return m_pTagSet != rhs.m_pTagSet || m_uiIndex != rhs.m_uiIndex; }

    /// \brief Advances the iterator to the next item
    void operator++();

  private:
    bool IsBitSet() const;

    const plTagSetTemplate<BlockStorageAllocator>* m_pTagSet;
    plUInt32 m_uiIndex = 0;
  };

  /// \brief Returns an iterator to list all tags in this set
  Iterator GetIterator() const { return Iterator(this); }

  /// \brief Writes the tag set state to a stream. Tags itself are serialized as strings.
  void Save(plStreamWriter& inout_stream) const;

  /// \brief Reads the tag set state from a stream and registers the tags with the given registry.
  void Load(plStreamReader& inout_stream, plTagRegistry& inout_registry);

private:
  friend class Iterator;

  bool IsTagInAllocatedRange(const plTag& Tag) const;

  void Reallocate(plUInt32 uiNewTagBlockStart, plUInt32 uiNewMaxBlockIndex);

  plSmallArray<plTagSetBlockStorage, 1, BlockStorageAllocator> m_TagBlocks;

  struct UserData
  {
    plUInt16 m_uiTagBlockStart;
    plUInt16 m_uiTagCount;
  };

  plUInt16 GetTagBlockStart() const;
  plUInt16 GetTagBlockEnd() const;
  void SetTagBlockStart(plUInt16 uiTagBlockStart);

  plUInt16 GetTagCount() const;
  void SetTagCount(plUInt16 uiTagCount);
  void IncreaseTagCount();
  void DecreaseTagCount();
};

/// Default tag set, uses plDefaultAllocatorWrapper for allocations.
using plTagSet = plTagSetTemplate<>;

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FOUNDATION_DLL, plTagSet);

template <typename BlockStorageAllocator>
typename plTagSetTemplate<BlockStorageAllocator>::Iterator cbegin(const plTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename plTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename plTagSetTemplate<BlockStorageAllocator>::Iterator cend(const plTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename plTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

template <typename BlockStorageAllocator>
typename plTagSetTemplate<BlockStorageAllocator>::Iterator begin(const plTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename plTagSetTemplate<BlockStorageAllocator>::Iterator(&cont);
}

template <typename BlockStorageAllocator>
typename plTagSetTemplate<BlockStorageAllocator>::Iterator end(const plTagSetTemplate<BlockStorageAllocator>& cont)
{
  return typename plTagSetTemplate<BlockStorageAllocator>::Iterator(&cont, true);
}

#include <Foundation/Types/Implementation/TagSet_inl.h>
