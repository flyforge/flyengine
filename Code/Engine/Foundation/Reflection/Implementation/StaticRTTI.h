#pragma once

/// \file

#include <Foundation/Basics.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/VariantType.h>
#include <type_traits>

class plRTTI;
class plReflectedClass;
class plVariant;

/// \brief Flags that describe a reflected type.
struct plTypeFlags
{
  using StorageType = plUInt8;

  enum Enum
  {
    StandardType = PL_BIT(0), ///< Anything that can be stored inside an plVariant except for pointers and containers.
    IsEnum = PL_BIT(1),       ///< enum struct used for plEnum.
    Bitflags = PL_BIT(2),     ///< bitflags struct used for plBitflags.
    Class = PL_BIT(3),        ///< A class or struct. The above flags are mutually exclusive.

    Abstract = PL_BIT(4), ///< Type is abstract.
    Phantom = PL_BIT(5),  ///< De-serialized type information that cannot be created on this process.
    Minimal = PL_BIT(6),  ///< Does not contain any property, function or attribute information. Used only for versioning.
    Default = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;
    StorageType Abstract : 1;
    StorageType Phantom : 1;
  };
};

PL_DECLARE_FLAGS_OPERATORS(plTypeFlags)


// ****************************************************
// ***** Templates for accessing static RTTI data *****

namespace plInternal
{
  /// \brief [internal] Helper struct for accessing static RTTI data.
  template <typename T>
  struct plStaticRTTI
  {
  };

  // Special implementation for types that have no base
  template <>
  struct plStaticRTTI<plNoBase>
  {
    static const plRTTI* GetRTTI() { return nullptr; }
  };

  // Special implementation for void to make function reflection compile void return values without further specialization.
  template <>
  struct plStaticRTTI<void>
  {
    static const plRTTI* GetRTTI() { return nullptr; }
  };

  template <typename T>
  PL_ALWAYS_INLINE const plRTTI* GetStaticRTTI(plTraitInt<1>) // class derived from plReflectedClass
  {
    return T::GetStaticRTTI();
  }

  template <typename T>
  PL_ALWAYS_INLINE const plRTTI* GetStaticRTTI(plTraitInt<0>) // static rtti
  {
    // Since this is pure C++ and no preprocessor macro, calling it with types such as 'int' and 'plInt32' will
    // actually return the same RTTI object, which would not be possible with a purely macro based solution

    return plStaticRTTI<T>::GetRTTI();
  }

  template <typename Type>
  plBitflags<plTypeFlags> DetermineTypeFlags()
  {
    plBitflags<plTypeFlags> flags;
    plVariantType::Enum type =
      static_cast<plVariantType::Enum>(plVariantTypeDeduction<typename plTypeTraits<Type>::NonConstReferenceType>::value);
    if ((type >= plVariantType::FirstStandardType && type <= plVariantType::LastStandardType) || PL_IS_SAME_TYPE(plVariant, Type))
      flags.Add(plTypeFlags::StandardType);
    else
      flags.Add(plTypeFlags::Class);

    if (std::is_abstract<Type>::value)
      flags.Add(plTypeFlags::Abstract);

    return flags;
  }

  template <>
  PL_ALWAYS_INLINE plBitflags<plTypeFlags> DetermineTypeFlags<plVariant>()
  {
    return plTypeFlags::StandardType;
  }

  template <typename T>
  struct plStaticRTTIWrapper
  {
    static_assert(sizeof(T) == 0, "Type has not been declared as reflectable (use PL_DECLARE_REFLECTABLE_TYPE macro)");
  };
} // namespace plInternal

/// \brief Use this function, specialized with the type that you are interested in, to get the static RTTI data for some type.
template <typename T>
PL_ALWAYS_INLINE const plRTTI* plGetStaticRTTI()
{
  return plInternal::GetStaticRTTI<T>(plTraitInt<PL_IS_DERIVED_FROM_STATIC(plReflectedClass, T)>());
}

// **************************************************
// ***** Macros for declaring types reflectable *****

#define PL_NO_LINKAGE

/// \brief Declares a type to be statically reflectable. Insert this into the header of a type to enable reflection on it.
/// This is not needed if the type is already dynamically reflectable.
#define PL_DECLARE_REFLECTABLE_TYPE(Linkage, TYPE)                    \
  namespace plInternal                                                \
  {                                                                   \
    template <>                                                       \
    struct Linkage plStaticRTTIWrapper<TYPE>                          \
    {                                                                 \
      static plRTTI s_RTTI;                                           \
    };                                                                \
                                                                      \
    /* This specialization calls the function to get the RTTI data */ \
    /* This code might get duplicated in different DLLs, but all   */ \
    /* will call the same function, so the RTTI object is unique   */ \
    template <>                                                       \
    struct plStaticRTTI<TYPE>                                         \
    {                                                                 \
      PL_ALWAYS_INLINE static const plRTTI* GetRTTI()                 \
      {                                                               \
        return &plStaticRTTIWrapper<TYPE>::s_RTTI;                    \
      }                                                               \
    };                                                                \
  }

/// \brief Insert this into a class/struct to enable properties that are private members.
/// All types that have dynamic reflection (\see PL_ADD_DYNAMIC_REFLECTION) already have this ability.
#define PL_ALLOW_PRIVATE_PROPERTIES(SELF) friend plRTTI GetRTTI(SELF*)

/// \cond
// internal helper macro
#define PL_RTTIINFO_DECL(Type, BaseType, Version) \
                                                  \
  plStringView GetTypeName(Type*)                 \
  {                                               \
    return #Type;                                 \
  }                                               \
  plUInt32 GetTypeVersion(Type*)                  \
  {                                               \
    return Version;                               \
  }                                               \
                                                  \
  plRTTI GetRTTI(Type*);

// internal helper macro
#define PL_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)              \
  plRTTI GetRTTI(Type*)                                                            \
  {                                                                                \
    using OwnType = Type;                                                          \
    using OwnBaseType = BaseType;                                                  \
    static AllocatorType Allocator;                                                \
    static plBitflags<plTypeFlags> flags = plInternal::DetermineTypeFlags<Type>(); \
    static plArrayPtr<const plAbstractProperty*> Properties;                       \
    static plArrayPtr<const plAbstractFunctionProperty*> Functions;                \
    static plArrayPtr<const plPropertyAttribute*> Attributes;                      \
    static plArrayPtr<plAbstractMessageHandler*> MessageHandlers;                  \
    static plArrayPtr<plMessageSenderInfo> MessageSenders;

/// \endcond

/// \brief Implements the necessary functionality for a type to be statically reflectable.
///
/// \param Type
///   The type for which the reflection functionality should be implemented.
/// \param BaseType
///   The base class type of \a Type. If it has no base class, pass plNoBase
/// \param Version
///   The version of \a Type. Must be increased when the class serialization changes.
/// \param AllocatorType
///   The type of an plRTTIAllocator that can be used to create and destroy instances
///   of \a Type. Pass plRTTINoAllocator for types that should not be created dynamically.
///   Pass plRTTIDefaultAllocator<Type> for types that should be created on the default heap.
///   Pass a custom plRTTIAllocator type to handle allocation differently.
#define PL_BEGIN_STATIC_REFLECTED_TYPE(Type, BaseType, Version, AllocatorType) \
  PL_RTTIINFO_DECL(Type, BaseType, Version)                                    \
  plRTTI plInternal::plStaticRTTIWrapper<Type>::s_RTTI = GetRTTI((Type*)0);    \
  PL_RTTIINFO_GETRTTI_IMPL_BEGIN(Type, BaseType, AllocatorType)


/// \brief Ends the reflection code block that was opened with PL_BEGIN_STATIC_REFLECTED_TYPE.
#define PL_END_STATIC_REFLECTED_TYPE                                                                                                         \
  ;                                                                                                                                          \
  return plRTTI(GetTypeName((OwnType*)0), plGetStaticRTTI<OwnBaseType>(), sizeof(OwnType), GetTypeVersion((OwnType*)0),                      \
    plVariantTypeDeduction<OwnType>::value, flags, &Allocator, Properties, Functions, Attributes, MessageHandlers, MessageSenders, nullptr); \
  }


/// \brief Within a PL_BEGIN_REFLECTED_TYPE / PL_END_REFLECTED_TYPE block, use this to start the block that declares all the properties.
#define PL_BEGIN_PROPERTIES static const plAbstractProperty* PropertyList[] =



/// \brief Ends the block to declare properties that was started with PL_BEGIN_PROPERTIES.
#define PL_END_PROPERTIES \
  ;                       \
  Properties = PropertyList

/// \brief Within a PL_BEGIN_REFLECTED_TYPE / PL_END_REFLECTED_TYPE block, use this to start the block that declares all the functions.
#define PL_BEGIN_FUNCTIONS static const plAbstractFunctionProperty* FunctionList[] =



/// \brief Ends the block to declare functions that was started with PL_BEGIN_FUNCTIONS.
#define PL_END_FUNCTIONS \
  ;                      \
  Functions = FunctionList

/// \brief Within a PL_BEGIN_REFLECTED_TYPE / PL_END_REFLECTED_TYPE block, use this to start the block that declares all the attributes.
#define PL_BEGIN_ATTRIBUTES static const plPropertyAttribute* AttributeList[] =



/// \brief Ends the block to declare attributes that was started with PL_BEGIN_ATTRIBUTES.
#define PL_END_ATTRIBUTES \
  ;                       \
  Attributes = AttributeList

/// \brief Within a PL_BEGIN_FUNCTIONS / PL_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data.
///
/// \param Function
///   The function to be executed, must match the C++ function name.
#define PL_FUNCTION_PROPERTY(Function) (new plFunctionProperty<decltype(&OwnType::Function)>(PL_STRINGIZE(Function), &OwnType::Function))

/// \brief Within a PL_BEGIN_FUNCTIONS / PL_END_FUNCTIONS; block, this adds a member or static function property stored inside the RTTI
/// data. Use this version if you need to change the name of the function or need to cast the function to one of its overload versions.
///
/// \param PropertyName
///   The name under which the property should be registered.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
#define PL_FUNCTION_PROPERTY_EX(PropertyName, Function) (new plFunctionProperty<decltype(&Function)>(PropertyName, &Function))

/// \internal Used by PL_SCRIPT_FUNCTION_PROPERTY
#define _PL_SCRIPT_FUNCTION_PARAM(type, name) plScriptableFunctionAttribute::ArgType::type, name

/// \brief Convenience macro to declare a function that can be called from scripts.
///
/// \param Function
///   The function to be executed, must match the C++ function name including the class name e.g. 'CLASS::NAME'.
///
/// Internally this calls PL_FUNCTION_PROPERTY and adds a plScriptableFunctionAttribute.
/// Use the variadic arguments in pairs to configure how each function parameter gets exposed.
///   Use 'In', 'Out' or 'Inout' to specify whether a function parameter is only read, or also written back to.
///   Follow it with a string to specify the name under which the parameter should show up.
///
/// Example:
///   PL_SCRIPT_FUNCTION_PROPERTY(MyFunc1NoParams)
///   PL_SCRIPT_FUNCTION_PROPERTY(MyFunc2FloatInDoubleOut, In, "FloatValue", Out, "DoubleResult")
#define PL_SCRIPT_FUNCTION_PROPERTY(Function, ...) \
  PL_FUNCTION_PROPERTY(Function)->AddAttributes(new plScriptableFunctionAttribute(PL_EXPAND_ARGS_PAIR_COMMA(_PL_SCRIPT_FUNCTION_PARAM, ##__VA_ARGS__)))

/// \brief Within a PL_BEGIN_FUNCTIONS / PL_END_FUNCTIONS; block, this adds a constructor function property stored inside the RTTI data.
///
/// \param Function
///   The function to be executed in the form of CLASS::FUNCTION_NAME.
#define PL_CONSTRUCTOR_PROPERTY(...) (new plConstructorFunctionProperty<OwnType, ##__VA_ARGS__>())


// [internal] Helper macro to get the return type of a getter function.
#define PL_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
///
/// \note There does not actually need to be a variable for this type of properties, as all accesses go through functions.
/// Thus you can for example expose a 'vector' property that is actually stored as a column of a matrix.
#define PL_ACCESSOR_PROPERTY(PropertyName, Getter, Setter) \
  (new plAccessorProperty<OwnType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// \brief Same as PL_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, Getter) \
  (new plAccessorProperty<OwnType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

// [internal] Helper macro to get the return type of a array getter function.
#define PL_ARRAY_GETTER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc(0))

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom functions to access an array.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetCount
///   Function signature: plUInt32 GetCount() const;
/// \param Getter
///   Function signature: Type GetValue(plUInt32 uiIndex) const;
/// \param Setter
///   Function signature: void SetValue(plUInt32 uiIndex, Type value);
/// \param Insert
///   Function signature: void Insert(plUInt32 uiIndex, Type value);
/// \param Remove
///   Function signature: void Remove(plUInt32 uiIndex);
#define PL_ARRAY_ACCESSOR_PROPERTY(PropertyName, GetCount, Getter, Setter, Insert, Remove) \
  (new plAccessorArrayProperty<OwnType, PL_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>(   \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, &OwnType::Setter, &OwnType::Insert, &OwnType::Remove))

/// \brief Same as PL_ARRAY_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_ARRAY_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetCount, Getter)             \
  (new plAccessorArrayProperty<OwnType, PL_ARRAY_GETTER_TYPE(OwnType, OwnType::Getter)>( \
    PropertyName, &OwnType::GetCount, &OwnType::Getter, nullptr, nullptr, nullptr))

#define PL_SET_CONTAINER_TYPE(Class, GetterFunc) decltype(std::declval<Class>().GetterFunc())

#define PL_SET_CONTAINER_SUB_TYPE(Class, GetterFunc) \
  plContainerSubTypeResolver<plTypeTraits<decltype(std::declval<Class>().GetterFunc())>::NonConstReferenceType>::Type

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom functions to access a set.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetValues
///   Function signature: Container<Type> GetValues() const;
/// \param Insert
///   Function signature: void Insert(Type value);
/// \param Remove
///   Function signature: void Remove(Type value);
///
/// \note Container<Type> can be any container that can be iterated via range based for loops.
#define PL_SET_ACCESSOR_PROPERTY(PropertyName, GetValues, Insert, Remove)                                            \
  (new plAccessorSetProperty<OwnType, plFunctionParameterTypeResolver<0, decltype(&OwnType::Insert)>::ParameterType, \
    PL_SET_CONTAINER_TYPE(OwnType, GetValues)>(PropertyName, &OwnType::GetValues, &OwnType::Insert, &OwnType::Remove))

/// \brief Same as PL_SET_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_SET_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetValues)                                                              \
  (new plAccessorSetProperty<OwnType, PL_SET_CONTAINER_SUB_TYPE(OwnType, GetValues), PL_SET_CONTAINER_TYPE(OwnType, GetValues)>( \
    PropertyName, &OwnType::GetValues, nullptr, nullptr))

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom functions to for write access to a
/// map.
///   Use this if you have a plHashTable or plMap to expose directly and just want to be informed of write operations.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetContainer
///   Function signature: const Container<Key, Type>& GetValues() const;
/// \param Insert
///   Function signature: void Insert(const char* szKey, Type value);
/// \param Remove
///   Function signature: void Remove(const char* szKey);
///
/// \note Container can be plMap or plHashTable
#define PL_MAP_WRITE_ACCESSOR_PROPERTY(PropertyName, GetContainer, Insert, Remove)                                        \
  (new plWriteAccessorMapProperty<OwnType, plFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
    PL_SET_CONTAINER_TYPE(OwnType, GetContainer)>(PropertyName, &OwnType::GetContainer, &OwnType::Insert, &OwnType::Remove))

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom functions to access a map.
///   Use this if you you want to hide the implementation details of the map from the user.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param GetKeyRange
///   Function signature: const Range GetValues() const;
///   Range has to be an object that a ranged based for-loop can iterate over containing the keys
///   implicitly convertible to Type / plString.
/// \param GetValue
///   Function signature: bool GetValue(const char* szKey, Type& value) const;
///   Returns whether the the key existed. value must be a non const ref as it is written to.
/// \param Insert
///   Function signature: void Insert(const char* szKey, Type value);
///   value can also be const and/or a reference.
/// \param Remove
///   Function signature: void Remove(const char* szKey);
///
/// \note Container can be plMap or plHashTable
#define PL_MAP_ACCESSOR_PROPERTY(PropertyName, GetKeyRange, GetValue, Insert, Remove)                                \
  (new plAccessorMapProperty<OwnType, plFunctionParameterTypeResolver<1, decltype(&OwnType::Insert)>::ParameterType, \
    PL_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, &OwnType::Insert, &OwnType::Remove))

/// \brief Same as PL_MAP_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_MAP_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, GetKeyRange, GetValue)                                           \
  (new plAccessorMapProperty<OwnType,                                                                                     \
    plTypeTraits<plFunctionParameterTypeResolver<1, decltype(&OwnType::GetValue)>::ParameterType>::NonConstReferenceType, \
    PL_SET_CONTAINER_TYPE(OwnType, GetKeyRange)>(PropertyName, &OwnType::GetKeyRange, &OwnType::GetValue, nullptr, nullptr))



/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that uses custom getter / setter functions.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   The name of the enum struct used by plEnum.
/// \param Getter
///   The getter function for this property.
/// \param Setter
///   The setter function for this property.
#define PL_ENUM_ACCESSOR_PROPERTY(PropertyName, EnumType, Getter, Setter) \
  (new plEnumAccessorProperty<OwnType, EnumType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// \brief Same as PL_ENUM_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_ENUM_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, EnumType, Getter) \
  (new plEnumAccessorProperty<OwnType, EnumType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))

/// \brief Same as PL_ENUM_ACCESSOR_PROPERTY, but for bitfields.
#define PL_BITFLAGS_ACCESSOR_PROPERTY(PropertyName, BitflagsType, Getter, Setter) \
  (new plBitflagsAccessorProperty<OwnType, BitflagsType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, &OwnType::Setter))

/// \brief Same as PL_BITFLAGS_ACCESSOR_PROPERTY, but no setter is provided, thus making the property read-only.
#define PL_BITFLAGS_ACCESSOR_PROPERTY_READ_ONLY(PropertyName, BitflagsType, Getter) \
  (new plBitflagsAccessorProperty<OwnType, BitflagsType, PL_GETTER_TYPE(OwnType, OwnType::Getter)>(PropertyName, &OwnType::Getter, nullptr))


// [internal] Helper macro to get the type of a class member.
#define PL_MEMBER_TYPE(Class, Member) decltype(std::declval<Class>().Member)

#define PL_MEMBER_CONTAINER_SUB_TYPE(Class, Member) \
  plContainerSubTypeResolver<plTypeTraits<decltype(std::declval<Class>().Member)>::NonConstReferenceType>::Type

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a PL_ENUM_ACCESSOR_PROPERTY instead.
#define PL_MEMBER_PROPERTY(PropertyName, MemberName)                                                   \
  (new plMemberProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                    \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is read-only.
#define PL_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                  \
  (new plMemberProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,                             \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is an array (plHybridArray, plDynamicArray or plDeque).
#define PL_ARRAY_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new plMemberArrayProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &plArrayPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &plArrayPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is a read-only array (plArrayPtr, plHybridArray, plDynamicArray or plDeque).
#define PL_ARRAY_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                                   \
  (new plMemberArrayReadOnlyProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &plArrayPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is a set (plSet, plHashSet).
#define PL_SET_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new plMemberSetProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &plSetPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &plSetPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is a read-only set (plSet, plHashSet).
#define PL_SET_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                           \
  (new plMemberSetProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &plSetPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is a map (plMap, plHashTable).
#define PL_MAP_MEMBER_PROPERTY(PropertyName, MemberName)                                                                                  \
  (new plMemberMapProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>(PropertyName, \
    &plMapPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer,                        \
    &plMapPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetContainer))

/// \brief Same as PL_MEMBER_PROPERTY, but the property is a read-only map (plMap, plHashTable).
#define PL_MAP_MEMBER_PROPERTY_READ_ONLY(PropertyName, MemberName)                                                           \
  (new plMemberMapProperty<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), PL_MEMBER_CONTAINER_SUB_TYPE(OwnType, MemberName)>( \
    PropertyName, &plMapPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetConstContainer, nullptr))

/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a property that actually exists as a member.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param EnumType
///   Name of the struct used by plEnum.
/// \param MemberName
///   The name of the member variable that should get exposed as a property.
///
/// \note Since the member is exposed directly, there is no way to know when the variable was modified. That also means
/// no custom limits to the values can be applied. If that becomes necessary, just add getter / setter functions and
/// expose the property as a PL_ACCESSOR_PROPERTY instead.
#define PL_ENUM_MEMBER_PROPERTY(PropertyName, EnumType, MemberName)                                    \
  (new plEnumMemberProperty<OwnType, EnumType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,      \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// \brief Same as PL_ENUM_MEMBER_PROPERTY, but the property is read-only.
#define PL_ENUM_MEMBER_PROPERTY_READ_ONLY(PropertyName, EnumType, MemberName)                                   \
  (new plEnumMemberProperty<OwnType, EnumType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,               \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// \brief Same as PL_ENUM_MEMBER_PROPERTY, but for bitfields.
#define PL_BITFLAGS_MEMBER_PROPERTY(PropertyName, BitflagsType, MemberName)                               \
  (new plBitflagsMemberProperty<OwnType, BitflagsType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue,    \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::SetValue,    \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))

/// \brief Same as PL_ENUM_MEMBER_PROPERTY_READ_ONLY, but for bitfields.
#define PL_BITFLAGS_MEMBER_PROPERTY_READ_ONLY(PropertyName, BitflagsType, MemberName)                           \
  (new plBitflagsMemberProperty<OwnType, BitflagsType, PL_MEMBER_TYPE(OwnType, MemberName)>(PropertyName,       \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetValue, nullptr, \
    &plPropertyAccessor<OwnType, PL_MEMBER_TYPE(OwnType, MemberName), &OwnType::MemberName>::GetPropertyPointer))



/// \brief Within a PL_BEGIN_PROPERTIES / PL_END_PROPERTIES; block, this adds a constant property stored inside the RTTI data.
///
/// \param PropertyName
///   The unique (in this class) name under which the property should be registered.
/// \param Value
///   The constant value to be stored.
#define PL_CONSTANT_PROPERTY(PropertyName, Value) (new plConstantProperty<decltype(Value)>(PropertyName, Value))



// [internal] Helper macro
#define PL_ENUM_VALUE_TO_CONSTANT_PROPERTY(name) PL_CONSTANT_PROPERTY(PL_STRINGIZE(name), (Storage)name),

/// \brief Within a PL_BEGIN_STATIC_REFLECTED_ENUM / PL_END_STATIC_REFLECTED_ENUM block, this converts a
/// list of enum values into constant RTTI properties.
#define PL_ENUM_CONSTANTS(...) PL_EXPAND_ARGS(PL_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// \brief Within a PL_BEGIN_STATIC_REFLECTED_ENUM / PL_END_STATIC_REFLECTED_ENUM block, this converts a
/// an enum value into a constant RTTI property.
#define PL_ENUM_CONSTANT(Value) PL_CONSTANT_PROPERTY(PL_STRINGIZE(Value), (Storage)Value)

/// \brief Within a PL_BEGIN_STATIC_REFLECTED_BITFLAGS / PL_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// list of bitflags into constant RTTI properties.
#define PL_BITFLAGS_CONSTANTS(...) PL_EXPAND_ARGS(PL_ENUM_VALUE_TO_CONSTANT_PROPERTY, ##__VA_ARGS__)

/// \brief Within a PL_BEGIN_STATIC_REFLECTED_BITFLAGS / PL_END_STATIC_REFLECTED_BITFLAGS block, this converts a
/// an bitflags into a constant RTTI property.
#define PL_BITFLAGS_CONSTANT(Value) PL_CONSTANT_PROPERTY(PL_STRINGIZE(Value), (Storage)Value)



/// \brief Implements the necessary functionality for an enum to be statically reflectable.
///
/// \param Type
///   The enum struct used by plEnum for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define PL_BEGIN_STATIC_REFLECTED_ENUM(Type, Version)                          \
  PL_BEGIN_STATIC_REFLECTED_TYPE(Type, plEnumBase, Version, plRTTINoAllocator) \
    ;                                                                          \
    using Storage = Type::StorageType;                                         \
    PL_BEGIN_PROPERTIES                                                        \
      {                                                                        \
        PL_CONSTANT_PROPERTY(PL_STRINGIZE(Type::Default), (Storage)Type::Default),

#define PL_END_STATIC_REFLECTED_ENUM \
  }                                  \
  PL_END_PROPERTIES                  \
  ;                                  \
  flags |= plTypeFlags::IsEnum;      \
  flags.Remove(plTypeFlags::Class);  \
  PL_END_STATIC_REFLECTED_TYPE


/// \brief Implements the necessary functionality for bitflags to be statically reflectable.
///
/// \param Type
///   The bitflags struct used by plBitflags for which reflection should be defined.
/// \param Version
///   The version of \a Type. Must be increased when the class changes.
#define PL_BEGIN_STATIC_REFLECTED_BITFLAGS(Type, Version)                          \
  PL_BEGIN_STATIC_REFLECTED_TYPE(Type, plBitflagsBase, Version, plRTTINoAllocator) \
    ;                                                                              \
    using Storage = Type::StorageType;                                             \
    PL_BEGIN_PROPERTIES                                                            \
      {                                                                            \
        PL_CONSTANT_PROPERTY(PL_STRINGIZE(Type::Default), (Storage)Type::Default),

#define PL_END_STATIC_REFLECTED_BITFLAGS \
  }                                      \
  PL_END_PROPERTIES                      \
  ;                                      \
  flags |= plTypeFlags::Bitflags;        \
  flags.Remove(plTypeFlags::Class);      \
  PL_END_STATIC_REFLECTED_TYPE



/// \brief Within an PL_BEGIN_REFLECTED_TYPE / PL_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// handlers.
#define PL_BEGIN_MESSAGEHANDLERS static plAbstractMessageHandler* HandlerList[] =


/// \brief Ends the block to declare message handlers that was started with PL_BEGIN_MESSAGEHANDLERS.
#define PL_END_MESSAGEHANDLERS \
  ;                            \
  MessageHandlers = HandlerList


/// \brief Within an PL_BEGIN_MESSAGEHANDLERS / PL_END_MESSAGEHANDLERS; block, this adds another message handler.
///
/// \param MessageType
///   The type of message that this handler function accepts. You may add 'const' in front of it.
/// \param FunctionName
///   The actual C++ name of the message handler function.
///
/// \note A message handler is a function that takes one parameter of type plMessage (or a derived type) and returns void.
#define PL_MESSAGE_HANDLER(MessageType, FunctionName)                                                                                   \
  new plInternal::MessageHandler<PL_IS_CONST_MESSAGE_HANDLER(OwnType, MessageType, &OwnType::FunctionName)>::Impl<OwnType, MessageType, \
    &OwnType::FunctionName>()


/// \brief Within an PL_BEGIN_REFLECTED_TYPE / PL_END_REFLECTED_TYPE block, use this to start the block that declares all the message
/// senders.
#define PL_BEGIN_MESSAGESENDERS static plMessageSenderInfo SenderList[] =


/// \brief Ends the block to declare message senders that was started with PL_BEGIN_MESSAGESENDERS.
#define PL_END_MESSAGESENDERS \
  ;                           \
  MessageSenders = SenderList;

/// \brief Within an PL_BEGIN_MESSAGESENDERS / PL_END_MESSAGESENDERS block, this adds another message sender.
///
/// \param MemberName
///   The name of the member variable that should get exposed as a message sender.
///
/// \note A message sender must be derived from plMessageSenderBase.
#define PL_MESSAGE_SENDER(MemberName)                                                  \
  {                                                                                    \
#    MemberName, plGetStaticRTTI < PL_MEMBER_TYPE(OwnType, MemberName)::MessageType>() \
  }
