#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/Implementation/StringBase.h>
#include <Foundation/Strings/StringConversion.h>
#include <Foundation/Strings/StringUtils.h>
#include <Foundation/Strings/StringView.h>

class plStringBuilder;
class plStreamReader;

/// \brief A string class for storing and passing around strings.
///
/// This class only allows read-access to its data. It does not allow modifications.
/// To build / modify strings, use the plStringBuilder class.
/// plHybridString has an internal array to store short strings without any memory allocations, it will dynamically
/// allocate additional memory, if that cache is insufficient. Thus a hybrid string will always take up a certain amount
/// of memory, which might be of concern when it is used as a member variable, in such cases you might want to use an
/// plHybridString with a very small internal array (1 would basically make it into a completely dynamic string).
/// On the other hand, creating plHybridString instances on the stack and working locally with them, is quite fast.
/// Prefer to use the typedef'd string types \a plString, \a plDynamicString, \a plString32 etc.
/// Most strings in an application are rather short, typically shorter than 20 characters.
/// Use \a plString, which is a typedef'd plHybridString to use a cache size that is sufficient for more than 90%
/// of all use cases.
template <plUInt16 Size>
struct plHybridStringBase : public plStringBase<plHybridStringBase<Size>>
{
protected:
  /// \brief Creates an empty string.
  plHybridStringBase(plAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  plHybridStringBase(const plHybridStringBase& rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  plHybridStringBase(plHybridStringBase&& rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  plHybridStringBase(const char* rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  plHybridStringBase(const wchar_t* rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  plHybridStringBase(const plStringView& rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Copies the data from \a rhs.
  plHybridStringBase(const plStringBuilder& rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Moves the data from \a rhs.
  plHybridStringBase(plStringBuilder&& rhs, plAllocatorBase* pAllocator); // [tested]

  /// \brief Destructor.
  ~plHybridStringBase(); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const plHybridStringBase& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(plHybridStringBase&& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const char* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const wchar_t* rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const plStringView& rhs); // [tested]

  /// \brief Copies the data from \a rhs.
  void operator=(const plStringBuilder& rhs); // [tested]

  /// \brief Moves the data from \a rhs.
  void operator=(plStringBuilder&& rhs); // [tested]

public:

  /// \brief Resets this string to an empty string.
  ///
  /// This will not deallocate any previously allocated data, but reuse that memory.
  void Clear(); // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  const char* GetData() const; // [tested]

  /// \brief Returns the amount of bytes that this string takes (excluding the '\0' terminator).
  plUInt32 GetElementCount() const; // [tested]

  /// \brief Returns the number of characters in this string.
  plUInt32 GetCharacterCount() const; // [tested]

  /// \brief Returns a view to a sub-string of this string, starting at character uiFirstCharacter, up until uiFirstCharacter +
  /// uiNumCharacters.
  ///
  /// Note that this view will only be valid as long as this plHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  plStringView GetSubString(plUInt32 uiFirstCharacter, plUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the first uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this plHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  plStringView GetFirst(plUInt32 uiNumCharacters) const; // [tested]

  /// \brief Returns a view to the sub-string containing the last uiNumCharacters characters of this string.
  ///
  /// Note that this view will only be valid as long as this plHybridString lives.
  /// Once the original string is destroyed, all views to them will point into invalid memory.
  plStringView GetLast(plUInt32 uiNumCharacters) const; // [tested]

  /// \brief Replaces the current string with the content from the stream. Reads the stream to its end.
  void ReadAll(plStreamReader& inout_stream);

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  plUInt64 GetHeapMemoryUsage() const { return m_Data.GetHeapMemoryUsage(); }

private:
  friend class plStringBuilder;

  plHybridArray<char, Size> m_Data;
  plUInt32 m_uiCharacterCount = 0;
};


/// \brief \see plHybridStringBase
template <plUInt16 Size, typename AllocatorWrapper = plDefaultAllocatorWrapper>
struct plHybridString : public plHybridStringBase<Size>
{
public:
  plHybridString();
  plHybridString(plAllocatorBase* pAllocator);

  plHybridString(const plHybridString<Size, AllocatorWrapper>& other);
  plHybridString(const plHybridStringBase<Size>& other);
  plHybridString(const char* rhs);
  plHybridString(const wchar_t* rhs);
  plHybridString(const plStringView& rhs);
  plHybridString(const plStringBuilder& rhs);
  plHybridString(plStringBuilder&& rhs);

  plHybridString(plHybridString<Size, AllocatorWrapper>&& other);
  plHybridString(plHybridStringBase<Size>&& other);


  void operator=(const plHybridString<Size, AllocatorWrapper>& rhs);
  void operator=(const plHybridStringBase<Size>& rhs);
  void operator=(const char* szString);
  void operator=(const wchar_t* pString);
  void operator=(const plStringView& rhs);
  void operator=(const plStringBuilder& rhs);
  void operator=(plStringBuilder&& rhs);

  void operator=(plHybridString<Size, AllocatorWrapper>&& rhs);
  void operator=(plHybridStringBase<Size>&& rhs);
};

using plDynamicString = plHybridString<1>;
/// \brief String that uses the static allocator to prevent leak reports in RTTI attributes.
using plUntrackedString = plHybridString<32, plStaticAllocatorWrapper>;
using plString = plHybridString<32>;
using plString16 = plHybridString<16>;
using plString24 = plHybridString<24>;
using plString32 = plHybridString<32>;
using plString48 = plHybridString<48>;
using plString64 = plHybridString<64>;
using plString128 = plHybridString<128>;
using plString256 = plHybridString<256>;

static_assert(plGetTypeClass<plString>::value == plTypeIsClass::value);

template <plUInt16 Size>
struct plCompareHelper<plHybridString<Size>>
{
  PLASMA_ALWAYS_INLINE bool Less(plStringView lhs, plStringView rhs) const
  {
    return lhs.Compare(rhs) < 0;
  }

  PLASMA_ALWAYS_INLINE bool Equal(plStringView lhs, plStringView rhs) const
  {
    return lhs.IsEqual(rhs);
  }
};

struct plCompareString_NoCase
{
  PLASMA_ALWAYS_INLINE bool Less(plStringView lhs, plStringView rhs) const
  {
    return lhs.Compare_NoCase(rhs) < 0;
  }

  PLASMA_ALWAYS_INLINE bool Equal(plStringView lhs, plStringView rhs) const
  {
    return lhs.IsEqual_NoCase(rhs);
  }
};

struct CompareConstChar
{
  /// \brief Returns true if a is less than b
  PLASMA_ALWAYS_INLINE bool Less(const char* a, const char* b) const { return plStringUtils::Compare(a, b) < 0; }

  /// \brief Returns true if a is equal to b
  PLASMA_ALWAYS_INLINE bool Equal(const char* a, const char* b) const { return plStringUtils::IsEqual(a, b); }
};

// For plFormatString
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plString& sArg);
PLASMA_FOUNDATION_DLL plStringView BuildString(char* szTmp, plUInt32 uiLength, const plUntrackedString& sArg);

#include <Foundation/Strings/Implementation/String_inl.h>
