#pragma once

#include <Foundation/Strings/StringUtils.h>

/// \brief STL forward iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the first character of the string and ends at the address beyond the last character of the string.
struct plStringIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = plUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = plUInt32;

  PL_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  PL_ALWAYS_INLINE plStringIterator() = default; // [tested]

  /// \brief Constructs either a begin or end iterator for the given string.
  PL_FORCE_INLINE explicit plStringIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr)
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;
  }

  /// \brief Checks whether this iterator points to a valid element. Invalid iterators either point to m_pEndPtr or were never initialized.
  PL_ALWAYS_INLINE bool IsValid() const { return m_pCurPtr != nullptr && m_pCurPtr != m_pEndPtr; } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  PL_ALWAYS_INLINE plUInt32 GetCharacter() const { return IsValid() ? plUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : plUInt32(0); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  PL_ALWAYS_INLINE plUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  PL_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  PL_ALWAYS_INLINE bool operator==(const plStringIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const plStringIterator&);

  /// \brief Advances the iterated to the next character, same as operator++, but returns how many bytes were consumed in the source string.
  PL_ALWAYS_INLINE plUInt32 Advance()
  {
    const char* pPrevElement = m_pCurPtr;

    if (m_pCurPtr < m_pEndPtr)
    {
      plUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return static_cast<plUInt32>(m_pCurPtr - pPrevElement);
  }

  /// \brief Move to the next Utf8 character
  PL_ALWAYS_INLINE plStringIterator& operator++() // [tested]
  {
    if (m_pCurPtr < m_pEndPtr)
    {
      plUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();
    }

    return *this;
  }

  /// \brief Move to the previous Utf8 character
  PL_ALWAYS_INLINE plStringIterator& operator--() // [tested]
  {
    if (m_pStartPtr < m_pCurPtr)
    {
      plUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }

    return *this;
  }

  /// \brief Move to the next Utf8 character
  PL_ALWAYS_INLINE plStringIterator operator++(int) // [tested]
  {
    plStringIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  PL_ALWAYS_INLINE plStringIterator operator--(int) // [tested]
  {
    plStringIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  PL_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// \brief Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  PL_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// \brief Returns an iterator that is advanced forwards by d characters.
  PL_ALWAYS_INLINE plStringIterator operator+(difference_type d) const // [tested]
  {
    plStringIterator it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  PL_ALWAYS_INLINE plStringIterator operator-(difference_type d) const // [tested]
  {
    plStringIterator it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  void SetCurrentPosition(const char* szCurPos)
  {
    PL_ASSERT_DEV((szCurPos >= m_pStartPtr) && (szCurPos <= m_pEndPtr), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};


/// \brief STL reverse iterator used by all string classes. Iterates over unicode characters.
///  The iterator starts at the last character of the string and ends at the address before the first character of the string.
struct plStringReverseIterator
{
  using iterator_category = std::bidirectional_iterator_tag;
  using value_type = plUInt32;
  using difference_type = std::ptrdiff_t;
  using pointer = const char*;
  using reference = plUInt32;

  PL_DECLARE_POD_TYPE();

  /// \brief Constructs an invalid iterator.
  PL_ALWAYS_INLINE plStringReverseIterator() = default; // [tested]

  /// \brief Constructs either a rbegin or rend iterator for the given string.
  PL_FORCE_INLINE explicit plStringReverseIterator(const char* pStartPtr, const char* pEndPtr, const char* pCurPtr) // [tested]
  {
    m_pStartPtr = pStartPtr;
    m_pEndPtr = pEndPtr;
    m_pCurPtr = pCurPtr;

    if (m_pStartPtr >= m_pEndPtr)
    {
      m_pCurPtr = nullptr;
    }
    else if (m_pCurPtr == m_pEndPtr)
    {
      plUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    }
  }

  /// \brief Checks whether this iterator points to a valid element.
  PL_ALWAYS_INLINE bool IsValid() const { return (m_pCurPtr != nullptr); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  PL_ALWAYS_INLINE plUInt32 GetCharacter() const { return IsValid() ? plUnicodeUtils::ConvertUtf8ToUtf32(m_pCurPtr) : plUInt32(0); } // [tested]

  /// \brief Returns the currently pointed to character in Utf32 encoding.
  PL_ALWAYS_INLINE plUInt32 operator*() const { return GetCharacter(); } // [tested]

  /// \brief Returns the address the iterator currently points to.
  PL_ALWAYS_INLINE const char* GetData() const { return m_pCurPtr; } // [tested]

  /// \brief Checks whether the two iterators point to the same element.
  PL_ALWAYS_INLINE bool operator==(const plStringReverseIterator& it2) const { return (m_pCurPtr == it2.m_pCurPtr); } // [tested]
  PL_ADD_DEFAULT_OPERATOR_NOTEQUAL(const plStringReverseIterator&);

  /// \brief Move to the next Utf8 character
  PL_FORCE_INLINE plStringReverseIterator& operator++() // [tested]
  {
    if (m_pCurPtr != nullptr && m_pStartPtr < m_pCurPtr)
      plUnicodeUtils::MoveToPriorUtf8(m_pCurPtr, m_pStartPtr).AssertSuccess();
    else
      m_pCurPtr = nullptr;

    return *this;
  }

  /// \brief Move to the previous Utf8 character
  PL_FORCE_INLINE plStringReverseIterator& operator--() // [tested]
  {
    if (m_pCurPtr != nullptr)
    {
      const char* szOldPos = m_pCurPtr;
      plUnicodeUtils::MoveToNextUtf8(m_pCurPtr).AssertSuccess();

      if (m_pCurPtr == m_pEndPtr)
        m_pCurPtr = szOldPos;
    }
    else
    {
      // Set back to the first character.
      m_pCurPtr = m_pStartPtr;
    }
    return *this;
  }

  /// \brief Move to the next Utf8 character
  PL_ALWAYS_INLINE plStringReverseIterator operator++(int) // [tested]
  {
    plStringReverseIterator tmp = *this;
    ++(*this);
    return tmp;
  }

  /// \brief Move to the previous Utf8 character
  PL_ALWAYS_INLINE plStringReverseIterator operator--(int) // [tested]
  {
    plStringReverseIterator tmp = *this;
    --(*this);
    return tmp;
  }

  /// \brief Advances the iterator forwards by d characters. Does not move it beyond the range's end.
  PL_FORCE_INLINE void operator+=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      ++(*this);
      --d;
    }
    while (d < 0)
    {
      --(*this);
      ++d;
    }
  }

  /// \brief Moves the iterator backwards by d characters. Does not move it beyond the range's start.
  PL_FORCE_INLINE void operator-=(difference_type d) // [tested]
  {
    while (d > 0)
    {
      --(*this);
      --d;
    }
    while (d < 0)
    {
      ++(*this);
      ++d;
    }
  }

  /// \brief Returns an iterator that is advanced forwards by d characters.
  PL_ALWAYS_INLINE plStringReverseIterator operator+(difference_type d) const // [tested]
  {
    plStringReverseIterator it = *this;
    it += d;
    return it;
  }

  /// \brief Returns an iterator that is advanced backwards by d characters.
  PL_ALWAYS_INLINE plStringReverseIterator operator-(difference_type d) const // [tested]
  {
    plStringReverseIterator it = *this;
    it -= d;
    return it;
  }

  /// \brief Allows to set the 'current' iteration position to a different value.
  ///
  /// Must be between the iterators start and end range.
  PL_FORCE_INLINE void SetCurrentPosition(const char* szCurPos)
  {
    PL_ASSERT_DEV((szCurPos == nullptr) || ((szCurPos >= m_pStartPtr) && (szCurPos < m_pEndPtr)), "New position must still be inside the iterator's range.");

    m_pCurPtr = szCurPos;
  }

private:
  const char* m_pStartPtr = nullptr;
  const char* m_pEndPtr = nullptr;
  const char* m_pCurPtr = nullptr;
};
