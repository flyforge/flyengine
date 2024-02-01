#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/AllocatorWrapper.h>

/// \brief Implementation of a hashset.
///
/// The hashset stores values by using the hash as an index into the table.
/// This implementation uses linear-probing to resolve hash collisions which means all values are stored
/// in a linear array.
/// All insertion/erasure/lookup functions take O(1) time if the table does not need to be expanded,
/// which happens when the load gets greater than 60%.
/// The hash function can be customized by providing a Hasher helper class like plHashHelper.

/// \see plHashHelper
template <typename KeyType, typename Hasher>
class plHashSetBase
{
public:
  /// \brief Const iterator.
  class ConstIterator
  {
  public:
    /// \brief Checks whether this iterator points to a valid element.
    bool IsValid() const; // [tested]

    /// \brief Checks whether the two iterators point to the same element.
    bool operator==(const typename plHashSetBase<KeyType, Hasher>::ConstIterator& rhs) const;

    PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const typename plHashSetBase<KeyType, Hasher>::ConstIterator&);

    /// \brief Returns the 'key' of the element that this iterator points to.
    const KeyType& Key() const; // [tested]

    /// \brief Returns the 'key' of the element that this iterator points to.
    PL_ALWAYS_INLINE const KeyType& operator*() { return Key(); } // [tested]

    /// \brief Advances the iterator to the next element in the map. The iterator will not be valid anymore, if the end is reached.
    void Next(); // [tested]

    /// \brief Shorthand for 'Next'
    void operator++(); // [tested]

  protected:
    friend class plHashSetBase<KeyType, Hasher>;

    explicit ConstIterator(const plHashSetBase<KeyType, Hasher>& hashSet);
    void SetToBegin();
    void SetToEnd();

    const plHashSetBase<KeyType, Hasher>* m_pHashSet = nullptr;
    plUInt32 m_uiCurrentIndex = 0; // current element index that this iterator points to.
    plUInt32 m_uiCurrentCount = 0; // current number of valid elements that this iterator has found so far.
  };

protected:
  /// \brief Creates an empty hashset. Does not allocate any data yet.
  explicit plHashSetBase(plAllocator* pAllocator); // [tested]

  /// \brief Creates a copy of the given hashset.
  plHashSetBase(const plHashSetBase<KeyType, Hasher>& rhs, plAllocator* pAllocator); // [tested]

  /// \brief Moves data from an existing hashtable into this one.
  plHashSetBase(plHashSetBase<KeyType, Hasher>&& rhs, plAllocator* pAllocator); // [tested]

  /// \brief Destructor.
  ~plHashSetBase(); // [tested]

  /// \brief Copies the data from another hashset into this one.
  void operator=(const plHashSetBase<KeyType, Hasher>& rhs); // [tested]

  /// \brief Moves data from an existing hashset into this one.
  void operator=(plHashSetBase<KeyType, Hasher>&& rhs); // [tested]

public:
  /// \brief Compares this table to another table.
  bool operator==(const plHashSetBase<KeyType, Hasher>& rhs) const; // [tested]
  PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const plHashSetBase<KeyType, Hasher>&);

  /// \brief Expands the hashset by over-allocating the internal storage so that the load factor is lower or equal to 60% when inserting the
  /// given number of entries.
  void Reserve(plUInt32 uiCapacity); // [tested]

  /// \brief Tries to compact the hashset to avoid wasting memory.
  ///
  /// The resulting capacity is at least 'GetCount' (no elements get removed).
  /// Will deallocate all data, if the hashset is empty.
  void Compact(); // [tested]

  /// \brief Returns the number of active entries in the table.
  plUInt32 GetCount() const; // [tested]

  /// \brief Returns true, if the hashset does not contain any elements.
  bool IsEmpty() const; // [tested]

  /// \brief Clears the table.
  void Clear(); // [tested]

  /// \brief Inserts the key. Returns whether the key was already existing.
  template <typename CompatibleKeyType>
  bool Insert(CompatibleKeyType&& key); // [tested]

  /// \brief Removes the entry with the given key. Returns if an entry was removed.
  template <typename CompatibleKeyType>
  bool Remove(const CompatibleKeyType& key); // [tested]

  /// \brief Erases the key at the given Iterator. Returns an iterator to the element after the given iterator.
  ConstIterator Remove(const ConstIterator& pos); // [tested]

  /// \brief Returns if an entry with given key exists in the table.
  template <typename CompatibleKeyType>
  bool Contains(const CompatibleKeyType& key) const; // [tested]

  /// \brief Checks whether all keys of the given set are in the container.
  bool ContainsSet(const plHashSetBase<KeyType, Hasher>& operand) const; // [tested]

  /// \brief Makes this set the union of itself and the operand.
  void Union(const plHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Makes this set the difference of itself and the operand, i.e. subtracts operand.
  void Difference(const plHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Makes this set the intersection of itself and the operand.
  void Intersection(const plHashSetBase<KeyType, Hasher>& operand); // [tested]

  /// \brief Returns a constant Iterator to the very first element.
  ConstIterator GetIterator() const; // [tested]

  /// \brief Returns a constant Iterator to the first element that is not part of the hashset. Needed to implement range based for loop
  /// support.
  ConstIterator GetEndIterator() const;

  /// \brief Returns the allocator that is used by this instance.
  plAllocator* GetAllocator() const;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  plUInt64 GetHeapMemoryUsage() const; // [tested]

  /// \brief Swaps this map with the other one.
  void Swap(plHashSetBase<KeyType, Hasher>& other); // [tested]

private:
  KeyType* m_pEntries;
  plUInt32* m_pEntryFlags;

  plUInt32 m_uiCount;
  plUInt32 m_uiCapacity;

  plAllocator* m_pAllocator;

  enum
  {
    FREE_ENTRY = 0,
    VALID_ENTRY = 1,
    DELETED_ENTRY = 2,
    FLAGS_MASK = 3,
    CAPACITY_ALIGNMENT = 32
  };

  void SetCapacity(plUInt32 uiCapacity);

  void RemoveInternal(plUInt32 uiIndex);

  template <typename CompatibleKeyType>
  plUInt32 FindEntry(const CompatibleKeyType& key) const;

  template <typename CompatibleKeyType>
  plUInt32 FindEntry(plUInt32 uiHash, const CompatibleKeyType& key) const;

  plUInt32 GetFlagsCapacity() const;
  plUInt32 GetFlags(plUInt32* pFlags, plUInt32 uiEntryIndex) const;
  void SetFlags(plUInt32 uiEntryIndex, plUInt32 uiFlags);

  bool IsFreeEntry(plUInt32 uiEntryIndex) const;
  bool IsValidEntry(plUInt32 uiEntryIndex) const;
  bool IsDeletedEntry(plUInt32 uiEntryIndex) const;

  void MarkEntryAsFree(plUInt32 uiEntryIndex);
  void MarkEntryAsValid(plUInt32 uiEntryIndex);
  void MarkEntryAsDeleted(plUInt32 uiEntryIndex);
};

/// \brief \see plHashSetBase
template <typename KeyType, typename Hasher = plHashHelper<KeyType>, typename AllocatorWrapper = plDefaultAllocatorWrapper>
class plHashSet : public plHashSetBase<KeyType, Hasher>
{
public:
  plHashSet();
  explicit plHashSet(plAllocator* pAllocator);

  plHashSet(const plHashSet<KeyType, Hasher, AllocatorWrapper>& other);
  plHashSet(const plHashSetBase<KeyType, Hasher>& other);

  plHashSet(plHashSet<KeyType, Hasher, AllocatorWrapper>&& other);
  plHashSet(plHashSetBase<KeyType, Hasher>&& other);

  void operator=(const plHashSet<KeyType, Hasher, AllocatorWrapper>& rhs);
  void operator=(const plHashSetBase<KeyType, Hasher>& rhs);

  void operator=(plHashSet<KeyType, Hasher, AllocatorWrapper>&& rhs);
  void operator=(plHashSetBase<KeyType, Hasher>&& rhs);
};

template <typename KeyType, typename Hasher>
typename plHashSetBase<KeyType, Hasher>::ConstIterator begin(const plHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename plHashSetBase<KeyType, Hasher>::ConstIterator cbegin(const plHashSetBase<KeyType, Hasher>& set)
{
  return set.GetIterator();
}

template <typename KeyType, typename Hasher>
typename plHashSetBase<KeyType, Hasher>::ConstIterator end(const plHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

template <typename KeyType, typename Hasher>
typename plHashSetBase<KeyType, Hasher>::ConstIterator cend(const plHashSetBase<KeyType, Hasher>& set)
{
  return set.GetEndIterator();
}

#include <Foundation/Containers/Implementation/HashSet_inl.h>
