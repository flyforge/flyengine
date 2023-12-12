#pragma once

/// \file

#include <Foundation/Basics.h>

/// Type traits
template <int v>
struct plTraitInt
{
  enum
  {
    value = v
  };
};

using plTypeIsMemRelocatable = plTraitInt<2>;
using plTypeIsPod = plTraitInt<1>;
using plTypeIsClass = plTraitInt<0>;

using plCompileTimeTrueType = char;
using plCompileTimeFalseType = int;

/// \brief Converts a bool condition to CompileTimeTrue/FalseType
template <bool cond>
struct plConditionToCompileTimeBool
{
  using type = plCompileTimeFalseType;
};

template <>
struct plConditionToCompileTimeBool<true>
{
  using type = plCompileTimeTrueType;
};

/// \brief Default % operator for T and TypeIsPod which returns a CompileTimeFalseType.
template <typename T>
plCompileTimeFalseType operator%(const T&, const plTypeIsPod&);

/// \brief If there is an % operator which takes a TypeIsPod and returns a CompileTimeTrueType T is Pod. Default % operator return false.
template <typename T>
struct plIsPodType : public plTraitInt<(sizeof(*((T*)0) % *((const plTypeIsPod*)0)) == sizeof(plCompileTimeTrueType)) ? 1 : 0>
{
};

/// \brief Pointers are POD types.
template <typename T>
struct plIsPodType<T*> : public plTypeIsPod
{
};

/// \brief arrays are POD types
template <typename T, int N>
struct plIsPodType<T[N]> : public plTypeIsPod
{
};

/// \brief Default % operator for T and plTypeIsMemRelocatable which returns a CompileTimeFalseType.
template <typename T>
plCompileTimeFalseType operator%(const T&, const plTypeIsMemRelocatable&);

/// \brief If there is an % operator which takes a plTypeIsMemRelocatable and returns a CompileTimeTrueType T is Pod. Default % operator
/// return false.
template <typename T>
struct plGetTypeClass
  : public plTraitInt<(sizeof(*((T*)0) % *((const plTypeIsMemRelocatable*)0)) == sizeof(plCompileTimeTrueType)) ? 2 : plIsPodType<T>::value>
{
};

/// \brief Static Conversion Test
template <typename From, typename To>
struct plConversionTest
{
  static plCompileTimeTrueType Test(const To&);
  static plCompileTimeFalseType Test(...);
  static From MakeFrom();

  enum
  {
    exists = sizeof(Test(MakeFrom())) == sizeof(plCompileTimeTrueType),
    sameType = 0
  };
};

/// \brief Specialization for above Type.
template <typename T>
struct plConversionTest<T, T>
{
  enum
  {
    exists = 1,
    sameType = 1
  };
};

// remapping of the 0 (not special) type to 3
template <typename T1, typename T2>
struct plGetStrongestTypeClass : public plTraitInt<(T1::value == 0 || T2::value == 0) ? 0 : PLASMA_COMPILE_TIME_MAX(T1::value, T2::value)>
{
};


/// \brief Determines whether a type is a pointer.
template <typename T>
struct plIsPointer
{
  static constexpr bool value = false;
};

template <typename T>
struct plIsPointer<T*>
{
  static constexpr bool value = true;
};


#ifdef __INTELLISENSE__

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define PLASMA_DECLARE_POD_TYPE()

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define PLASMA_DECLARE_MEM_RELOCATABLE_TYPE()

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define PLASMA_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define PLASMA_DETECT_TYPE_CLASS(...)

#else

/// \brief Embed this into a class to mark it as a POD type.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#  define PLASMA_DECLARE_POD_TYPE() \
    plCompileTimeTrueType operator%(const plTypeIsPod&) const { return {}; }

/// \brief Embed this into a class to mark it as memory relocatable.
/// Memory relocatable types will get special treatment from allocators and container classes, such that they are faster to construct and
/// copy. A type is memory relocatable if it does not have any internal references. e.g: struct example { char[16] buffer; char* pCur;
/// example() pCur(buffer) {} }; A memory relocatable type also must not give out any pointers to its own location. If these two conditions
/// are met, a type is memory relocatable.
#  define PLASMA_DECLARE_MEM_RELOCATABLE_TYPE() \
    plCompileTimeTrueType operator%(const plTypeIsMemRelocatable&) const { return {}; }

/// \brief mark a class as memory relocatable if the passed type is relocatable or pod.
#  define PLASMA_DECLARE_MEM_RELOCATABLE_TYPE_CONDITIONAL(T)                                                                                       \
    typename plConditionToCompileTimeBool<plGetTypeClass<T>::value == plTypeIsMemRelocatable::value || plIsPodType<T>::value>::type operator%( \
      const plTypeIsMemRelocatable&) const { return {}; }

#  define PLASMA_DETECT_TYPE_CLASS_1(T1) plGetTypeClass<T1>
#  define PLASMA_DETECT_TYPE_CLASS_2(T1, T2) plGetStrongestTypeClass<PLASMA_DETECT_TYPE_CLASS_1(T1), PLASMA_DETECT_TYPE_CLASS_1(T2)>
#  define PLASMA_DETECT_TYPE_CLASS_3(T1, T2, T3) plGetStrongestTypeClass<PLASMA_DETECT_TYPE_CLASS_2(T1, T2), PLASMA_DETECT_TYPE_CLASS_1(T3)>
#  define PLASMA_DETECT_TYPE_CLASS_4(T1, T2, T3, T4) plGetStrongestTypeClass<PLASMA_DETECT_TYPE_CLASS_2(T1, T2), PLASMA_DETECT_TYPE_CLASS_2(T3, T4)>
#  define PLASMA_DETECT_TYPE_CLASS_5(T1, T2, T3, T4, T5) plGetStrongestTypeClass<PLASMA_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), PLASMA_DETECT_TYPE_CLASS_1(T5)>
#  define PLASMA_DETECT_TYPE_CLASS_6(T1, T2, T3, T4, T5, T6) \
    plGetStrongestTypeClass<PLASMA_DETECT_TYPE_CLASS_4(T1, T2, T3, T4), PLASMA_DETECT_TYPE_CLASS_2(T5, T6)>

// \brief embed this into a class to automatically detect which type class it belongs to
// This macro is only guaranteed to work for classes / structs which don't have any constructor / destructor / assignment operator!
// As arguments you have to list the types of all the members of the class / struct.
#  define PLASMA_DETECT_TYPE_CLASS(...)  \
    plCompileTimeTrueType operator%( \
      const plTraitInt<PLASMA_CALL_MACRO(PLASMA_CONCAT(PLASMA_DETECT_TYPE_CLASS_, PLASMA_VA_NUM_ARGS(__VA_ARGS__)), (__VA_ARGS__))::value>&) const { return {}; }
#endif

/// \brief Defines a type T as Pod.
/// POD types will get special treatment from allocators and container classes, such that they are faster to construct and copy.
#define PLASMA_DEFINE_AS_POD_TYPE(T)             \
  template <>                                \
  struct plIsPodType<T> : public plTypeIsPod \
  {                                          \
  }

PLASMA_DEFINE_AS_POD_TYPE(bool);
PLASMA_DEFINE_AS_POD_TYPE(float);
PLASMA_DEFINE_AS_POD_TYPE(double);

PLASMA_DEFINE_AS_POD_TYPE(char);
PLASMA_DEFINE_AS_POD_TYPE(plInt8);
PLASMA_DEFINE_AS_POD_TYPE(plInt16);
PLASMA_DEFINE_AS_POD_TYPE(plInt32);
PLASMA_DEFINE_AS_POD_TYPE(plInt64);
PLASMA_DEFINE_AS_POD_TYPE(plUInt8);
PLASMA_DEFINE_AS_POD_TYPE(plUInt16);
PLASMA_DEFINE_AS_POD_TYPE(plUInt32);
PLASMA_DEFINE_AS_POD_TYPE(plUInt64);
PLASMA_DEFINE_AS_POD_TYPE(wchar_t);
PLASMA_DEFINE_AS_POD_TYPE(unsigned long);
PLASMA_DEFINE_AS_POD_TYPE(long);

/// \brief Checks inheritance at compile time.
#define PLASMA_IS_DERIVED_FROM_STATIC(BaseClass, DerivedClass) \
  (plConversionTest<const DerivedClass*, const BaseClass*>::exists && !plConversionTest<const BaseClass*, const void*>::sameType)

/// \brief Checks whether A and B are the same type
#define PLASMA_IS_SAME_TYPE(TypeA, TypeB) plConversionTest<TypeA, TypeB>::sameType

template <typename T>
struct plTypeTraits
{
  /// \brief removes const qualifier
  using NonConstType = typename std::remove_const<T>::type;

  /// \brief removes reference
  using NonReferenceType = typename std::remove_reference<T>::type;

  /// \brief removes pointer
  using NonPointerType = typename std::remove_pointer<T>::type;

  /// \brief removes reference and const qualifier
  using NonConstReferenceType = typename std::remove_const<typename std::remove_reference<T>::type>::type;

  /// \brief removes reference and pointer qualifier
  using NonReferencePointerType = typename std::remove_pointer<typename std::remove_reference<T>::type>::type;

  /// \brief removes reference, const and pointer qualifier
  /// Note that this removes the const and reference of the type pointed too, not of the pointer.
  using NonConstReferencePointerType = typename std::remove_const<typename std::remove_reference<typename std::remove_pointer<T>::type>::type>::type;
};

/// generates a template named 'checkerName' which checks for the existence of a member function with
/// the name 'functionName' and the signature 'Signature'
#define PLASMA_MAKE_MEMBERFUNCTION_CHECKER(functionName, checkerName)                \
  template <typename T, typename Signature>                                      \
  struct checkerName                                                             \
  {                                                                              \
    template <typename U, U>                                                     \
    struct type_check;                                                           \
    template <typename O>                                                        \
    static plCompileTimeTrueType& chk(type_check<Signature, &O::functionName>*); \
    template <typename>                                                          \
    static plCompileTimeFalseType& chk(...);                                     \
    enum                                                                         \
    {                                                                            \
      value = (sizeof(chk<T>(0)) == sizeof(plCompileTimeTrueType)) ? 1 : 0       \
    };                                                                           \
  }
