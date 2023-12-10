#pragma once

#include <Foundation/Types/Delegate.h>

/// \brief This class uses delegates to define a range of values that can be enumerated using a forward iterator.
///
/// Can be used to create a contiguous view to elements of a certain type without the need for them to actually
/// exist in the same space or format. Think of IEnumerable in c# using composition via plDelegate instead of derivation.
/// ValueType defines the value type we are iterating over and IteratorType is the internal key to identify an element.
/// An example that creates a RangeView of strings that are stored in a linear array of structs.
/// \code{.cpp}
/// auto range = plRangeView<const char*, plUInt32>(
///   [this]()-> plUInt32 { return 0; },
///   [this]()-> plUInt32 { return array.GetCount(); },
///   [this](plUInt32& it) { ++it; },
///   [this](const plUInt32& it)-> const char* { return array[it].m_String; });
///
/// for (const char* szValue : range)
/// {
/// }
/// \endcode
template <typename ValueType, typename IteratorType>
class plRangeView
{
public:
  using BeginCallback = plDelegate<IteratorType()>;
  using EndCallback = plDelegate<IteratorType()>;
  using NextCallback = plDelegate<void(IteratorType&)>;
  using ValueCallback = plDelegate<ValueType(const IteratorType&)>;

  /// \brief Initializes the plRangeView with the delegates used to enumerate the range.
  PLASMA_ALWAYS_INLINE plRangeView(BeginCallback begin, EndCallback end, NextCallback next, ValueCallback value);

  /// \brief Const iterator, don't use directly, use ranged based for loops or call begin() end().
  struct ConstIterator
  {
    PLASMA_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = ConstIterator;
    using pointer = ConstIterator*;
    using reference = ConstIterator&;

    PLASMA_ALWAYS_INLINE ConstIterator(const ConstIterator& rhs) = default;
    PLASMA_FORCE_INLINE void Next();
    PLASMA_FORCE_INLINE ValueType Value() const;
    PLASMA_ALWAYS_INLINE ValueType operator*() const { return Value(); }
    PLASMA_ALWAYS_INLINE void operator++() { Next(); }
    PLASMA_FORCE_INLINE bool operator==(const typename plRangeView<ValueType, IteratorType>::ConstIterator& it2) const;
    PLASMA_FORCE_INLINE bool operator!=(const typename plRangeView<ValueType, IteratorType>::ConstIterator& it2) const;

  protected:
    PLASMA_FORCE_INLINE explicit ConstIterator(const plRangeView<ValueType, IteratorType>* view, IteratorType pos);

    friend class plRangeView<ValueType, IteratorType>;
    const plRangeView<ValueType, IteratorType>* m_pView = nullptr;
    IteratorType m_Pos;
  };

  /// \brief Iterator, don't use directly, use ranged based for loops or call begin() end().
  struct Iterator : public ConstIterator
  {
    PLASMA_DECLARE_POD_TYPE();

    using iterator_category = std::forward_iterator_tag;
    using value_type = Iterator;
    using pointer = Iterator*;
    using reference = Iterator&;

    using ConstIterator::Value;
    PLASMA_ALWAYS_INLINE Iterator(const Iterator& rhs) = default;
    PLASMA_FORCE_INLINE ValueType Value();
    PLASMA_ALWAYS_INLINE ValueType operator*() { return Value(); }

  protected:
    PLASMA_FORCE_INLINE explicit Iterator(const plRangeView<ValueType, IteratorType>* view, IteratorType pos);
  };

  Iterator begin() { return Iterator(this, m_Begin()); }
  Iterator end() { return Iterator(this, m_End()); }
  ConstIterator begin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator end() const { return ConstIterator(this, m_End()); }
  ConstIterator cbegin() const { return ConstIterator(this, m_Begin()); }
  ConstIterator cend() const { return ConstIterator(this, m_End()); }

private:
  friend struct Iterator;
  friend struct ConstIterator;

  BeginCallback m_Begin;
  EndCallback m_End;
  NextCallback m_Next;
  ValueCallback m_Value;
};

template <typename V, typename I>
typename plRangeView<V, I>::Iterator begin(plRangeView<V, I>& in_container)
{
  return in_container.begin();
}

template <typename V, typename I>
typename plRangeView<V, I>::ConstIterator begin(const plRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename plRangeView<V, I>::ConstIterator cbegin(const plRangeView<V, I>& container)
{
  return container.cbegin();
}

template <typename V, typename I>
typename plRangeView<V, I>::Iterator end(plRangeView<V, I>& in_container)
{
  return in_container.end();
}

template <typename V, typename I>
typename plRangeView<V, I>::ConstIterator end(const plRangeView<V, I>& container)
{
  return container.cend();
}

template <typename V, typename I>
typename plRangeView<V, I>::ConstIterator cend(const plRangeView<V, I>& container)
{
  return container.cend();
}

#include <Foundation/Types/Implementation/RangeView_inl.h>
