#pragma once

// ***** Definition of types *****

using plUInt8 = unsigned char;
using plUInt16 = unsigned short;
using plUInt32 = unsigned int;
using plUInt64 = unsigned long long;

using plInt8 = signed char;
using plInt16 = short;
using plInt32 = int;
using plInt64 = long long;

// no float-types, since those are well portable

// Do some compile-time checks on the types
PL_CHECK_AT_COMPILETIME(sizeof(bool) == 1);
PL_CHECK_AT_COMPILETIME(sizeof(char) == 1);
PL_CHECK_AT_COMPILETIME(sizeof(float) == 4);
PL_CHECK_AT_COMPILETIME(sizeof(double) == 8);
PL_CHECK_AT_COMPILETIME(sizeof(plInt8) == 1);
PL_CHECK_AT_COMPILETIME(sizeof(plInt16) == 2);
PL_CHECK_AT_COMPILETIME(sizeof(plInt32) == 4);
PL_CHECK_AT_COMPILETIME(sizeof(plInt64) == 8); // must be defined in the specific compiler header
PL_CHECK_AT_COMPILETIME(sizeof(plUInt8) == 1);
PL_CHECK_AT_COMPILETIME(sizeof(plUInt16) == 2);
PL_CHECK_AT_COMPILETIME(sizeof(plUInt32) == 4);
PL_CHECK_AT_COMPILETIME(sizeof(plUInt64) == 8); // must be defined in the specific compiler header
PL_CHECK_AT_COMPILETIME(sizeof(long long int) == 8);

#if PL_ENABLED(PL_PLATFORM_64BIT)
#  define PL_ALIGNMENT_MINIMUM 8
#elif PL_ENABLED(PL_PLATFORM_32BIT)
#  define PL_ALIGNMENT_MINIMUM 4
#else
#  error "Unknown pointer size."
#endif

PL_CHECK_AT_COMPILETIME(sizeof(void*) == PL_ALIGNMENT_MINIMUM);
PL_CHECK_AT_COMPILETIME(alignof(void*) == PL_ALIGNMENT_MINIMUM);

/// \brief Enum values for success and failure. To be used by functions as return values mostly, instead of bool.
enum plResultEnum
{
  PL_FAILURE,
  PL_SUCCESS
};

/// \brief Default enum for returning failure or success, instead of using a bool.
struct [[nodiscard]] PL_FOUNDATION_DLL plResult
{
public:
  plResult(plResultEnum res)
    : m_E(res)
  {
  }

  void operator=(plResultEnum rhs) { m_E = rhs; }
  bool operator==(plResultEnum cmp) const { return m_E == cmp; }
  bool operator!=(plResultEnum cmp) const { return m_E != cmp; }

  [[nodiscard]] PL_ALWAYS_INLINE bool Succeeded() const { return m_E == PL_SUCCESS; }
  [[nodiscard]] PL_ALWAYS_INLINE bool Failed() const { return m_E == PL_FAILURE; }

  /// \brief Used to silence compiler warnings, when success or failure doesn't matter.
  PL_ALWAYS_INLINE void IgnoreResult()
  {
    /* dummy to be called when a return value is [[nodiscard]] but the result is not needed */
  }

  /// \brief Asserts that the function succeeded. In case of failure, the program will terminate.
  ///
  /// If \a msg is given, this will be the assert message. If \a details is provided, \a msg should contain a formatting element ({}), e.g. "Error: {}".
  void AssertSuccess(const char* szMsg = nullptr, const char* szDetails = nullptr) const;

private:
  plResultEnum m_E;
};

/// \brief Explicit conversion to plResult, can be overloaded for arbitrary types.
///
/// This is intentionally not done via casting operator overload (or even additional constructors) since this usually comes with a
/// considerable data loss.
PL_ALWAYS_INLINE plResult plToResult(plResult result)
{
  return result;
}

/// \brief Helper macro to call functions that return plStatus or plResult in a function that returns plStatus (or plResult) as well.
/// If the called function fails, its return value is returned from the calling scope.
#define PL_SUCCEED_OR_RETURN(code) \
  do                               \
  {                                \
    auto s = (code);               \
    if (plToResult(s).Failed())    \
      return s;                    \
  } while (false)

/// \brief Like PL_SUCCEED_OR_RETURN, but with error logging.
#define PL_SUCCEED_OR_RETURN_LOG(code)                                    \
  do                                                                      \
  {                                                                       \
    auto s = (code);                                                      \
    if (plToResult(s).Failed())                                           \
    {                                                                     \
      plLog::Error("Call '{0}' failed with: {1}", PL_STRINGIZE(code), s); \
      return s;                                                           \
    }                                                                     \
  } while (false)

/// \brief Like PL_SUCCEED_OR_RETURN, but with custom error logging.
#define PL_SUCCEED_OR_RETURN_CUSTOM_LOG(code, log)                          \
  do                                                                        \
  {                                                                         \
    auto s = (code);                                                        \
    if (plToResult(s).Failed())                                             \
    {                                                                       \
      plLog::Error("Call '{0}' failed with: {1}", PL_STRINGIZE(code), log); \
      return s;                                                             \
    }                                                                       \
  } while (false)

//////////////////////////////////////////////////////////////////////////

class plRTTI;

/// \brief Dummy type to pass to templates and macros that expect a base type for a class that has no base.
class plNoBase
{
public:
  static const plRTTI* GetStaticRTTI() { return nullptr; }
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an enum class.
class plEnumBase
{
};

/// \brief Dummy type to pass to templates and macros that expect a base type for an bitflags class.
class plBitflagsBase
{
};

/// \brief Helper struct to get a storage type from a size in byte.
template <size_t SizeInByte>
struct plSizeToType;
/// \cond
template <>
struct plSizeToType<1>
{
  using Type = plUInt8;
};
template <>
struct plSizeToType<2>
{
  using Type = plUInt16;
};
template <>
struct plSizeToType<3>
{
  using Type = plUInt32;
};
template <>
struct plSizeToType<4>
{
  using Type = plUInt32;
};
template <>
struct plSizeToType<5>
{
  using Type = plUInt64;
};
template <>
struct plSizeToType<6>
{
  using Type = plUInt64;
};
template <>
struct plSizeToType<7>
{
  using Type = plUInt64;
};
template <>
struct plSizeToType<8>
{
  using Type = plUInt64;
};
/// \endcond
