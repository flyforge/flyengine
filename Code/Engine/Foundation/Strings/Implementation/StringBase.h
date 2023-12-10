#pragma once

#include <Foundation/Strings/StringView.h>

namespace plInternal
{
  template <typename T, bool isString>
  struct HashHelperImpl;
}

/// Base class for strings, which implements all read-only string functions.
template <typename Derived>
struct plStringBase : public plThisIsAString
{
public:
  using iterator = plStringIterator;
  using const_iterator = plStringIterator;
  using reverse_iterator = plStringReverseIterator;
  using const_reverse_iterator = plStringReverseIterator;

  /// Returns whether the string is an empty string.
  bool IsEmpty() const; // [tested]

  /// Returns true, if this string starts with the given string.
  bool StartsWith(plStringView sStartsWith) const; // [tested]

  /// Returns true, if this string starts with the given string. Case insensitive.
  bool StartsWith_NoCase(plStringView sStartsWith) const; // [tested]

  /// Returns true, if this string ends with the given string.
  bool EndsWith(plStringView sEndsWith) const; // [tested]

  /// Returns true, if this string ends with the given string. Case insensitive.
  bool EndsWith_NoCase(plStringView sEndsWith) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found.
  /// To find the next occurrence, use an plStringView which points to the next position and call FindSubString again.
  const char* FindSubString(plStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the first occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// To find the next occurrence, use an plStringView which points to the next position and call FindSubString again.
  const char* FindSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString(plStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Returns a pointer to the last occurrence of szStringToFind, or nullptr if none was found. Case insensitive.
  /// szStartSearchAt allows to start searching at the end of the string (if it is nullptr) or at an earlier position.
  const char* FindLastSubString_NoCase(plStringView sStringToFind, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr.
  const char* FindWholeWord(const char* szSearchFor, plStringUtils::PLASMA_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Searches for the word szSearchFor. If IsDelimiterCB returns true for both characters in front and back of the word, the position is
  /// returned. Otherwise nullptr. Ignores case.
  const char* FindWholeWord_NoCase(const char* szSearchFor, plStringUtils::PLASMA_CHARACTER_FILTER isDelimiterCB, const char* szStartSearchAt = nullptr) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise.
  plInt32 Compare(plStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise.
  plInt32 CompareN(plStringView sOther, plUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other one. Returns 0 for equality, -1 if this string is 'smaller', 1 otherwise. Case insensitive.
  plInt32 Compare_NoCase(plStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other one. Returns 0 for equality, -1 if this string is 'smaller',
  /// 1 otherwise. Case insensitive.
  plInt32 CompareN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other string for equality.
  bool IsEqual(plStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN(plStringView sOther, plUInt32 uiCharsToCompare) const; // [tested]

  /// Compares this string with the other string for equality.
  bool IsEqual_NoCase(plStringView sOther) const; // [tested]

  /// Compares up to a given number of characters of this string with the other string for equality. Case insensitive.
  bool IsEqualN_NoCase(plStringView sOther, plUInt32 uiCharsToCompare) const; // [tested]

  /// \brief Computes the pointer to the n-th character in the string. This is a linear search from the start.
  const char* ComputeCharacterPosition(plUInt32 uiCharacterIndex) const;

  /// \brief Returns an iterator to this string, which points to the very first character.
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  iterator GetIteratorFront() const;

  /// \brief Returns an iterator to this string, which points to the very last character (NOT the end).
  ///
  /// Note that this iterator will only be valid as long as this string lives.
  /// Once the original string is destroyed, all iterators to them will point into invalid memory.
  reverse_iterator GetIteratorBack() const;

  /// \brief Returns a std::string_view to this string.
  std::string_view GetAsStdView() const;

  /// \brief Returns a string view to this string's data.
  operator std::string_view() const; // [tested]

  /// \brief Returns a string view to this string's data.
  operator plStringView() const; // [tested]

  /// \brief Returns a string view to this string's data.
  plStringView GetView() const; // [tested]

  /// \brief Returns a pointer to the internal Utf8 string.
  PLASMA_ALWAYS_INLINE operator const char*() const { return InternalGetData(); }

  /// \brief Returns a pointer to the internal Utf8 string.
  PLASMA_ALWAYS_INLINE operator const char8_t*() const { return reinterpret_cast<const char8_t*>(InternalGetData()); }

  /// \brief Fills the given container with plStringView's which represent each found substring.
  /// If bReturnEmptyStrings is true, even empty strings between separators are returned.
  /// Output must be a container that stores plStringView's and provides the functions 'Clear' and 'Append'.
  /// szSeparator1 to szSeparator6 are strings which act as separators and indicate where to split the string.
  /// This string itself will not be modified.
  template <typename Container>
  void Split(bool bReturnEmptyStrings, Container& ref_output, const char* szSeparator1, const char* szSeparator2 = nullptr, const char* szSeparator3 = nullptr, const char* szSeparator4 = nullptr, const char* szSeparator5 = nullptr, const char* szSeparator6 = nullptr) const; // [tested]

  /// \brief Checks whether the given path has any file extension
  bool HasAnyExtension() const; // [tested]

  /// \brief Checks whether the given path ends with the given extension. szExtension should start with a '.' for performance reasons, but
  /// it will work without a '.' too.
  bool HasExtension(plStringView sExtension) const; // [tested]

  /// \brief Returns the file extension of the given path. Will be empty, if the path does not end with a proper extension.
  plStringView GetFileExtension() const; // [tested]

  /// \brief Returns the file name of a path, excluding the path and extension.
  ///
  /// If the path already ends with a path separator, the result will be empty.
  plStringView GetFileName() const; // [tested]

  /// \brief Returns the substring that represents the file name including the file extension.
  ///
  /// Returns an empty string, if sPath already ends in a path separator, or is empty itself.
  plStringView GetFileNameAndExtension() const; // [tested]

  /// \brief Returns the directory of the given file, which is the substring up to the last path separator.
  ///
  /// If the path already ends in a path separator, and thus points to a folder, instead of a file, the unchanged path is returned.
  /// "path/to/file" -> "path/to/"
  /// "path/to/folder/" -> "path/to/folder/"
  /// "filename" -> ""
  /// "/file_at_root_level" -> "/"
  plStringView GetFileDirectory() const; // [tested]

  /// \brief Returns true, if the given path represents an absolute path on the current OS.
  bool IsAbsolutePath() const; // [tested]

  /// \brief Returns true, if the given path represents a relative path on the current OS.
  bool IsRelativePath() const; // [tested]

  /// \brief Returns true, if the given path represents a 'rooted' path. See plFileSystem for details.
  bool IsRootedPath() const; // [tested]

  /// \brief Extracts the root name from a rooted path
  ///
  /// ":MyRoot" -> "MyRoot"
  /// ":MyRoot\folder" -> "MyRoot"
  /// ":\MyRoot\folder" -> "MyRoot"
  /// ":/MyRoot\folder" -> "MyRoot"
  /// Returns an empty string, if the path is not rooted.
  plStringView GetRootedPathRootName() const; // [tested]

private:
  const char* InternalGetData() const;
  const char* InternalGetDataEnd() const;
  plUInt32 InternalGetElementCount() const;

  template <typename Derived2>
  friend typename plStringBase<Derived2>::iterator begin(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::const_iterator cbegin(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::iterator end(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::const_iterator cend(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::reverse_iterator rbegin(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::const_reverse_iterator crbegin(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::reverse_iterator rend(const plStringBase<Derived2>& container);

  template <typename Derived2>
  friend typename plStringBase<Derived2>::const_reverse_iterator crend(const plStringBase<Derived2>& container);
};


template <typename Derived>
typename plStringBase<Derived>::iterator begin(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetData());
}

template <typename Derived>
typename plStringBase<Derived>::const_iterator cbegin(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::const_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetData());
}

template <typename Derived>
typename plStringBase<Derived>::iterator end(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename plStringBase<Derived>::const_iterator cend(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::const_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}


template <typename Derived>
typename plStringBase<Derived>::reverse_iterator rbegin(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename plStringBase<Derived>::const_reverse_iterator crbegin(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::const_reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), container.InternalGetDataEnd());
}

template <typename Derived>
typename plStringBase<Derived>::reverse_iterator rend(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), nullptr);
}

template <typename Derived>
typename plStringBase<Derived>::const_reverse_iterator crend(const plStringBase<Derived>& container)
{
  return typename plStringBase<Derived>::const_reverse_iterator(container.InternalGetData(), container.InternalGetDataEnd(), nullptr);
}

#include <Foundation/Strings/Implementation/StringBase_inl.h>
