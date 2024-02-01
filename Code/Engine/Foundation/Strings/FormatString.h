#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Strings/Implementation/FormatStringArgs.h>

class plStringBuilder;
struct plStringView;

/// \brief Implements formating of strings with placeholders and formatting options.
///
/// plFormatString can be used anywhere where a string should be formatable when passing it into a function.
/// Good examples are plStringBuilder::Format() or plLog::Info().
///
/// A function taking an plFormatString can internally call plFormatString::GetText() to retrieve he formatted result.
/// When calling such a function, one must wrap the parameter into 'plFmt' to enable formatting options, example:
///   void MyFunc(const plFormatString& text);
///   MyFunc(plFmt("Cool Story {}", "Bro"));
///
/// To provide more convenience, one can add a template-function overload like this:
///   template <typename... ARGS>
///   void MyFunc(const char* szFormat, ARGS&&... args)
///   {
///     MyFunc(plFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...));
///   }
///
/// This allows to call MyFunc() without the 'plFmt' wrapper.
///
///
/// === Formatting ===
///
/// Placeholders for variables are specified using '{}'. These may use numbers from 0 to 9,
/// ie. {0}, {3}, {2}, etc. which allows to change the order or insert duplicates.
/// If no number is provided, each {} instance represents the next argument.
///
/// To specify special formatting, wrap the argument into an plArgXY call:
///   plArgC - for characters
///   plArgI - for integer formatting
///   plArgU - for unsigned integer formatting (e.g. HEX)
///   plArgF - for floating point formatting
///   plArgP - for pointer formatting
///   plArgDateTime - for plDateTime formatting options
///   plArgErrorCode - for Windows error code formatting
///   plArgHumanReadable - for shortening numbers with common abbreviations
///   plArgFileSize - for representing file sizes
///
/// Example:
///   plStringBuilder::Format("HEX: {}", plArgU(1337, 8 /*width*/, true /*pad with zeros*/, 16 /*base16*/, true/*upper case*/));
///
/// Arbitrary other types can support special formatting even without an plArgXY call. E.g. plTime and plAngle do special formatting.
/// plArgXY calls are only necessary if formatting options are needed for a specific formatting should be enforced (e.g. plArgErrorCode
/// would otherwise just use uint32 formatting).
///
/// To implement custom formatting see the various free standing 'BuildString' functions.
class PL_FOUNDATION_DLL plFormatString
{
  PL_DISALLOW_COPY_AND_ASSIGN(plFormatString); // pass by reference, never pass by value

public:
  PL_ALWAYS_INLINE plFormatString() = default;
  PL_ALWAYS_INLINE plFormatString(const char* szString) { m_sString = szString; }
  PL_ALWAYS_INLINE plFormatString(plStringView sString) { m_sString = sString; }
  plFormatString(const plStringBuilder& s);
  virtual ~plFormatString() = default;

  /// \brief Generates the formatted text. Make sure to only call this function once and only when the formatted string is really needed.
  ///
  /// Requires an plStringBuilder as storage, ie. POTENTIALLY writes the formatted text into it.
  /// However, if no formatting is required, it may not touch the string builder at all and just return a string directly.
  ///
  /// \note Do not assume that the result is stored in \a sb. Always only use the return value. The string builder is only used
  /// when necessary.
  [[nodiscard]] virtual plStringView GetText(plStringBuilder&) const { return m_sString; }

  /// \brief Similar to GetText() but guaranteed to copy the string into the given string builder,
  /// and thus guaranteeing that the generated string is zero terminated.
  virtual const char* GetTextCStr(plStringBuilder& out_sString) const;

  bool IsEmpty() const { return m_sString.IsEmpty(); }

  /// \brief Helper function to build the formatted text with the given arguments.
  ///
  /// \note We can't use plArrayPtr here because of include order.
  plStringView BuildFormattedText(plStringBuilder& ref_sStorage, plStringView* pArgs, plUInt32 uiNumArgs) const;

protected:
  plStringView m_sString;
};

#include <Foundation/Strings/Implementation/FormatStringImpl.h>

template <typename... ARGS>
PL_ALWAYS_INLINE plFormatStringImpl<ARGS...> plFmt(const char* szFormat, ARGS&&... args)
{
  return plFormatStringImpl<ARGS...>(szFormat, std::forward<ARGS>(args)...);
}
