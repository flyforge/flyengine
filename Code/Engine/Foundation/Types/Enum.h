#pragma once

/// \file

/// \brief A custom enum implementation that allows to define the underlying storage type to control its memory footprint.
///
/// Advantages over a simple C++ enum:
/// 1) Storage type can be defined
/// 2) Enum is default initialized automatically
/// 3) Definition of the enum itself, the storage type and the default init value is in one place
/// 4) It makes function definitions shorter, instead of:
///      void function(plExampleEnumBase::Enum value)
///    you can write:
///      void function(plExampleEnum value)
/// 5) In all other ways it works exactly like a C++ enum
///
/// Example:
///
/// struct plExampleEnumBase
/// {
///   using StorageType = plUInt8;
///
///   enum Enum
///   {
///     Value1 = 1,          // normal value
///     Value2 = 2,          // normal value
///     Value3 = 3,          // normal value
///     Default = Value1 // Default initialization value (required)
///   };
/// };
/// using plExampleEnum = plEnum<plExampleEnumBase>;
///
/// This defines an "plExampleEnum" which is stored in an plUInt8 and is default initialized with Value1
/// For more examples see the enum test.
template <typename Derived>
struct plEnum : public Derived
{
public:
  using SelfType = plEnum<Derived>;
  using StorageType = typename Derived::StorageType;

  /// \brief Default constructor
  PLASMA_ALWAYS_INLINE plEnum()
    : m_Value((StorageType)Derived::Default)
  {
  } // [tested]

  /// \brief Copy constructor
  PLASMA_ALWAYS_INLINE plEnum(const SelfType& rh)
    : m_Value(rh.m_Value)
  {
  }

  /// \brief Construct from a C++ enum, and implicit conversion from enum type
  PLASMA_ALWAYS_INLINE plEnum(typename Derived::Enum init)
    : m_Value((StorageType)init)
  {
  } // [tested]

  /// \brief Assignment operator
  PLASMA_ALWAYS_INLINE void operator=(const SelfType& rh) // [tested]
  {
    m_Value = rh.m_Value;
  }

  /// \brief Assignment operator.
  PLASMA_ALWAYS_INLINE void operator=(const typename Derived::Enum value) // [tested]
  {
    m_Value = (StorageType)value;
  }

  /// \brief Comparison operators
  PLASMA_ALWAYS_INLINE bool operator==(const SelfType& rhs) const { return m_Value == rhs.m_Value; }
  PLASMA_ALWAYS_INLINE bool operator!=(const SelfType& rhs) const { return m_Value != rhs.m_Value; }
  PLASMA_ALWAYS_INLINE bool operator>(const SelfType& rhs) const { return m_Value > rhs.m_Value; }
  PLASMA_ALWAYS_INLINE bool operator<(const SelfType& rhs) const { return m_Value < rhs.m_Value; }
  PLASMA_ALWAYS_INLINE bool operator>=(const SelfType& rhs) const { return m_Value >= rhs.m_Value; }
  PLASMA_ALWAYS_INLINE bool operator<=(const SelfType& rhs) const { return m_Value <= rhs.m_Value; }

  PLASMA_ALWAYS_INLINE bool operator==(typename Derived::Enum value) const { return m_Value == value; }
  PLASMA_ALWAYS_INLINE bool operator!=(typename Derived::Enum value) const { return m_Value != value; }
  PLASMA_ALWAYS_INLINE bool operator>(typename Derived::Enum value) const { return m_Value > value; }
  PLASMA_ALWAYS_INLINE bool operator<(typename Derived::Enum value) const { return m_Value < value; }
  PLASMA_ALWAYS_INLINE bool operator>=(typename Derived::Enum value) const { return m_Value >= value; }
  PLASMA_ALWAYS_INLINE bool operator<=(typename Derived::Enum value) const { return m_Value <= value; }

  /// brief Bitwise operators
  PLASMA_ALWAYS_INLINE SelfType operator|(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value | rhs.m_Value); } // [tested]
  PLASMA_ALWAYS_INLINE SelfType operator&(const SelfType& rhs) const { return static_cast<typename Derived::Enum>(m_Value & rhs.m_Value); } // [tested]

  /// \brief Implicit conversion to enum type.
  PLASMA_ALWAYS_INLINE operator typename Derived::Enum() const // [tested]
  {
    return static_cast<typename Derived::Enum>(m_Value);
  }

  /// \brief Returns the enum value as an integer
  PLASMA_ALWAYS_INLINE StorageType GetValue() const // [tested]
  {
    return m_Value;
  }

  /// \brief Sets the enum value through an integer
  PLASMA_ALWAYS_INLINE void SetValue(StorageType value) // [tested]
  {
    m_Value = value;
  }

private:
  StorageType m_Value;
};


#define PLASMA_ENUM_VALUE_TO_STRING(name) \
  case name:                          \
    return PLASMA_STRINGIZE(name);

/// \brief Helper macro to generate a 'ToString' function for enum values.
///
/// Usage: PLASMA_ENUM_TO_STRING(Value1, Value2, Value3, Value4)
/// Embed it into a struct (which defines the enums).
/// Example:
/// struct plExampleEnum
/// {
///   enum Enum
///   {
///     A,
///     B,
///     C,
///   };
///
///   PLASMA_ENUM_TO_STRING(A, B, C);
/// };
#define PLASMA_ENUM_TO_STRING(...)                               \
  const char* ToString(plUInt32 value)                       \
  {                                                          \
    switch (value)                                           \
    {                                                        \
      PLASMA_EXPAND_ARGS(PLASMA_ENUM_VALUE_TO_STRING, ##__VA_ARGS__) \
      default:                                               \
        return nullptr;                                      \
    }                                                        \
  }
