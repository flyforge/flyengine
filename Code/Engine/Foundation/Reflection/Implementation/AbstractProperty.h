#pragma once

/// \file

#include <Foundation/Basics.h>

#include <Foundation/Containers/HashSet.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Containers/StaticArray.h>
#include <Foundation/Reflection/Implementation/RTTI.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>

class plRTTI;
class plPropertyAttribute;

/// \brief Determines whether a type is plIsBitflags.
template <typename T>
struct plIsBitflags
{
  static constexpr bool value = false;
};

template <typename T>
struct plIsBitflags<plBitflags<T>>
{
  static constexpr bool value = true;
};

/// \brief Determines whether a type is plIsBitflags.
template <typename T>
struct plIsEnum
{
  static constexpr bool value = std::is_enum<T>::value;
};

template <typename T>
struct plIsEnum<plEnum<T>>
{
  static constexpr bool value = true;
};

/// \brief Flags used to describe a property and its type.
struct plPropertyFlags
{
  using StorageType = plUInt16;

  enum Enum : plUInt16
  {
    StandardType = PL_BIT(0), ///< Anything that can be stored inside an plVariant except for pointers and containers.
    IsEnum = PL_BIT(1),       ///< enum property, cast to plAbstractEnumerationProperty.
    Bitflags = PL_BIT(2),     ///< Bitflags property, cast to plAbstractEnumerationProperty.
    Class = PL_BIT(3),        ///< A struct or class. All of the above are mutually exclusive.

    Const = PL_BIT(4),     ///< Property value is const.
    Reference = PL_BIT(5), ///< Property value is a reference.
    Pointer = PL_BIT(6),   ///< Property value is a pointer.

    PointerOwner = PL_BIT(7), ///< This pointer property takes ownership of the passed pointer.
    ReadOnly = PL_BIT(8),     ///< Can only be read but not modified.
    Hidden = PL_BIT(9),       ///< This property should not appear in the UI.
    Phantom = PL_BIT(10),     ///< Phantom types are mirrored types on the editor side. Ie. they do not exist as actual classes in the process. Also used
                              ///< for data driven types, e.g. by the Visual Shader asset.

    VarOut = PL_BIT(11),   ///< Tag for non-const-ref function parameters to indicate usage 'out'
    VarInOut = PL_BIT(12), ///< Tag for non-const-ref function parameters to indicate usage 'inout'

    Default = 0,
    Void = 0
  };

  struct Bits
  {
    StorageType StandardType : 1;
    StorageType IsEnum : 1;
    StorageType Bitflags : 1;
    StorageType Class : 1;

    StorageType Const : 1;
    StorageType Reference : 1;
    StorageType Pointer : 1;

    StorageType PointerOwner : 1;
    StorageType ReadOnly : 1;
    StorageType Hidden : 1;
    StorageType Phantom : 1;
  };

  template <class Type>
  static plBitflags<plPropertyFlags> GetParameterFlags()
  {
    using CleanType = typename plTypeTraits<Type>::NonConstReferencePointerType;
    plBitflags<plPropertyFlags> flags;
    constexpr plVariantType::Enum type = static_cast<plVariantType::Enum>(plVariantTypeDeduction<CleanType>::value);
    if constexpr (std::is_same<CleanType, plVariant>::value ||
                  std::is_same<Type, const char*>::value || // We treat const char* as a basic type and not a pointer.
                  (type >= plVariantType::FirstStandardType && type <= plVariantType::LastStandardType))
      flags.Add(plPropertyFlags::StandardType);
    else if constexpr (plIsEnum<CleanType>::value)
      flags.Add(plPropertyFlags::IsEnum);
    else if constexpr (plIsBitflags<CleanType>::value)
      flags.Add(plPropertyFlags::Bitflags);
    else
      flags.Add(plPropertyFlags::Class);

    if constexpr (std::is_const<typename plTypeTraits<Type>::NonReferencePointerType>::value)
      flags.Add(plPropertyFlags::Const);

    if constexpr (std::is_pointer<Type>::value && !std::is_same<Type, const char*>::value)
      flags.Add(plPropertyFlags::Pointer);

    if constexpr (std::is_reference<Type>::value)
      flags.Add(plPropertyFlags::Reference);

    return flags;
  }
};

template <>
inline plBitflags<plPropertyFlags> plPropertyFlags::GetParameterFlags<void>()
{
  return plBitflags<plPropertyFlags>();
}

PL_DECLARE_FLAGS_OPERATORS(plPropertyFlags)

/// \brief Describes what category a property belongs to.
struct plPropertyCategory
{
  using StorageType = plUInt8;

  enum Enum
  {
    Constant, ///< The property is a constant value that is stored inside the RTTI data.
    Member,   ///< The property is a 'member property', i.e. it represents some accessible value. Cast to plAbstractMemberProperty.
    Function, ///< The property is a function which can be called. Cast to plAbstractFunctionProperty.
    Array,    ///< The property is actually an array of values. The array dimensions might be changeable. Cast to plAbstractArrayProperty.
    Set,      ///< The property is actually a set of values. Cast to plAbstractSetProperty.
    Map,      ///< The property is actually a map from string to values. Cast to plAbstractMapProperty.
    Default = Member
  };
};

/// \brief This is the base interface for all properties in the reflection system. It provides enough information to cast to the next better
/// base class.
class PL_FOUNDATION_DLL plAbstractProperty
{
public:
  /// \brief The constructor must get the name of the property. The string must be a compile-time constant.
  plAbstractProperty(const char* szPropertyName) { m_szPropertyName = szPropertyName; }

  virtual ~plAbstractProperty() = default;

  /// \brief Returns the name of the property.
  const char* GetPropertyName() const { return m_szPropertyName; }

  /// \brief Returns the type information of the constant property. Use this to cast this property to a specific version of
  /// plTypedConstantProperty.
  virtual const plRTTI* GetSpecificType() const = 0;

  /// \brief Returns the category of this property. Cast this property to the next higher type for more information.
  virtual plPropertyCategory::Enum GetCategory() const = 0; // [tested]

  /// \brief Returns the flags of the property.
  const plBitflags<plPropertyFlags>& GetFlags() const { return m_Flags; };

  /// \brief Adds flags to the property. Returns itself to allow to be called during initialization.
  plAbstractProperty* AddFlags(plBitflags<plPropertyFlags> flags)
  {
    m_Flags.Add(flags);
    return this;
  };

  /// \brief Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  plAbstractProperty* AddAttributes(plPropertyAttribute* pAttrib1, plPropertyAttribute* pAttrib2 = nullptr, plPropertyAttribute* pAttrib3 = nullptr,
    plPropertyAttribute* pAttrib4 = nullptr, plPropertyAttribute* pAttrib5 = nullptr, plPropertyAttribute* pAttrib6 = nullptr)
  {
    PL_ASSERT_DEV(pAttrib1 != nullptr, "invalid attribute");

    m_Attributes.PushBack(pAttrib1);
    if (pAttrib2)
      m_Attributes.PushBack(pAttrib2);
    if (pAttrib3)
      m_Attributes.PushBack(pAttrib3);
    if (pAttrib4)
      m_Attributes.PushBack(pAttrib4);
    if (pAttrib5)
      m_Attributes.PushBack(pAttrib5);
    if (pAttrib6)
      m_Attributes.PushBack(pAttrib6);
    return this;
  };

  /// \brief Returns the array of property attributes.
  plArrayPtr<const plPropertyAttribute* const> GetAttributes() const { return m_Attributes; }

  /// \brief Returns the first attribute that derives from the given type, or nullptr if nothing is found.
  template <typename Type>
  const Type* GetAttributeByType() const;

protected:
  plBitflags<plPropertyFlags> m_Flags;
  const char* m_szPropertyName;
  plHybridArray<const plPropertyAttribute*, 2, plStaticsAllocatorWrapper> m_Attributes; // Do not track RTTI data.
};

/// \brief This is the base class for all constant properties that are stored inside the RTTI data.
class PL_FOUNDATION_DLL plAbstractConstantProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractConstantProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns plPropertyCategory::Constant.
  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Constant; } // [tested]

  /// \brief Returns a pointer to the constant data or nullptr. See plAbstractMemberProperty::GetPropertyPointer for more information.
  virtual void* GetPropertyPointer() const = 0;

  /// \brief Returns the constant value as an plVariant
  virtual plVariant GetConstant() const = 0;
};

/// \brief This is the base class for all properties that are members of a class. It provides more information about the actual type.
///
/// If plPropertyFlags::Pointer is set as a flag, you must not cast this property to plTypedMemberProperty, instead use GetValuePtr and
/// SetValuePtr. This is because reference and const-ness of the property are only fixed for the pointer but not the type, so the actual
/// property type cannot be derived.
class PL_FOUNDATION_DLL plAbstractMemberProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractMemberProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns plPropertyCategory::Member.
  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Member; }

  /// \brief Returns a pointer to the property data or nullptr. If a valid pointer is returned, that pointer and the information from
  /// GetSpecificType() can be used to step deeper into the type (if required).
  ///
  /// You need to pass the pointer to an object on which you are operating. This function is mostly of interest when the property itself is
  /// a compound type (a struct or class). If it is a simple type (int, float, etc.) it doesn't make much sense to retrieve the pointer.
  ///
  /// For example GetSpecificType() might return that a property is of type plVec3. In that case one might either stop and just use the code
  /// to handle plVec3 types, or one might continue and enumerate all sub-properties (x, y and z) as well.
  ///
  /// \note There is no guarantee that this function returns a non-nullptr pointer, independent of the type. When a property uses custom
  /// 'accessors' (functions to get / set the property value), it is not possible (or useful) to get the property pointer.
  virtual void* GetPropertyPointer(const void* pInstance) const = 0;

  /// \brief Writes the value of this property in pInstance to pObject.
  /// pObject needs to point to an instance of this property's type.
  virtual void GetValuePtr(const void* pInstance, void* out_pObject) const = 0;

  /// \brief Sets the value of pObject to the property in pInstance.
  /// pObject needs to point to an instance of this property's type.
  virtual void SetValuePtr(void* pInstance, const void* pObject) const = 0;
};


/// \brief The base class for a property that represents an array of values.
class PL_FOUNDATION_DLL plAbstractArrayProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractArrayProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns plPropertyCategory::Array.
  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Array; }

  /// \brief Returns number of elements.
  virtual plUInt32 GetCount(const void* pInstance) const = 0;

  /// \brief Writes element at index uiIndex to the target of pObject.
  virtual void GetValue(const void* pInstance, plUInt32 uiIndex, void* pObject) const = 0;

  /// \brief Writes the target of pObject to the element at index uiIndex.
  virtual void SetValue(void* pInstance, plUInt32 uiIndex, const void* pObject) const = 0;

  /// \brief Inserts the target of pObject into the array at index uiIndex.
  virtual void Insert(void* pInstance, plUInt32 uiIndex, const void* pObject) const = 0;

  /// \brief Removes the element in the array at index uiIndex.
  virtual void Remove(void* pInstance, plUInt32 uiIndex) const = 0;

  /// \brief Clears the array.
  virtual void Clear(void* pInstance) const = 0;

  /// \brief Resizes the array to uiCount.
  virtual void SetCount(void* pInstance, plUInt32 uiCount) const = 0;

  virtual void* GetValuePointer(void* pInstance, plUInt32 uiIndex) const { return nullptr; }
};


/// \brief The base class for a property that represents a set of values.
///
/// The element type must either be a standard type or a pointer.
class PL_FOUNDATION_DLL plAbstractSetProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractSetProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns plPropertyCategory::Set.
  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Set; }

  /// \brief Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// \brief Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// \brief Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, const void* pObject) const = 0;

  /// \brief Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, const void* pObject) const = 0;

  /// \brief Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, const void* pObject) const = 0;

  /// \brief Writes the content of the set to out_keys.
  virtual void GetValues(const void* pInstance, plDynamicArray<plVariant>& out_keys) const = 0;
};


/// \brief The base class for a property that represents a set of values.
///
/// The element type must either be a standard type or a pointer.
class PL_FOUNDATION_DLL plAbstractMapProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractMapProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  /// \brief Returns plPropertyCategory::Map.
  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Map; }

  /// \brief Returns whether the set is empty.
  virtual bool IsEmpty(const void* pInstance) const = 0;

  /// \brief Clears the set.
  virtual void Clear(void* pInstance) const = 0;

  /// \brief Inserts the target of pObject into the set.
  virtual void Insert(void* pInstance, const char* szKey, const void* pObject) const = 0;

  /// \brief Removes the target of pObject from the set.
  virtual void Remove(void* pInstance, const char* szKey) const = 0;

  /// \brief Returns whether the target of pObject is in the set.
  virtual bool Contains(const void* pInstance, const char* szKey) const = 0;

  /// \brief Writes element at index uiIndex to the target of pObject.
  virtual bool GetValue(const void* pInstance, const char* szKey, void* pObject) const = 0;

  /// \brief Writes the content of the set to out_keys.
  virtual void GetKeys(const void* pInstance, plHybridArray<plString, 16>& out_keys) const = 0;
};

/// \brief Use getArgument<N, Args...>::Type to get the type of the Nth argument in Args.
template <int _Index, class... Args>
struct getArgument;

template <class Head, class... Tail>
struct getArgument<0, Head, Tail...>
{
  using Type = Head;
};

template <int _Index, class Head, class... Tail>
struct getArgument<_Index, Head, Tail...>
{
  using Type = typename getArgument<_Index - 1, Tail...>::Type;
};

/// \brief Template that allows to probe a function for a parameter and return type.
template <int I, typename FUNC>
struct plFunctionParameterTypeResolver
{
};

template <int I, typename R, typename... P>
struct plFunctionParameterTypeResolver<I, R (*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  PL_CHECK_AT_COMPILETIME_MSG(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

template <int I, class Class, typename R, typename... P>
struct plFunctionParameterTypeResolver<I, R (Class::*)(P...)>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  PL_CHECK_AT_COMPILETIME_MSG(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

template <int I, class Class, typename R, typename... P>
struct plFunctionParameterTypeResolver<I, R (Class::*)(P...) const>
{
  enum Constants
  {
    Arguments = sizeof...(P),
  };
  PL_CHECK_AT_COMPILETIME_MSG(I < Arguments, "I needs to be smaller than the number of function parameters.");
  using ParameterType = typename getArgument<I, P...>::Type;
  using ReturnType = R;
};

/// \brief Template that allows to probe a single parameter function for parameter and return type.
template <typename FUNC>
struct plMemberFunctionParameterTypeResolver
{
};

template <class Class, typename R, typename P>
struct plMemberFunctionParameterTypeResolver<R (Class::*)(P)>
{
  using ParameterType = P;
  using ReturnType = R;
};

/// \brief Template that allows to probe a container for its element type.
template <typename CONTAINER>
struct plContainerSubTypeResolver
{
};

template <typename T>
struct plContainerSubTypeResolver<plArrayPtr<T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct plContainerSubTypeResolver<plDynamicArray<T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T, plUInt32 Size>
struct plContainerSubTypeResolver<plHybridArray<T, Size>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T, plUInt32 Size>
struct plContainerSubTypeResolver<plStaticArray<T, Size>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T, plUInt16 Size>
struct plContainerSubTypeResolver<plSmallArray<T, Size>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct plContainerSubTypeResolver<plDeque<T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct plContainerSubTypeResolver<plSet<T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename T>
struct plContainerSubTypeResolver<plHashSet<T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct plContainerSubTypeResolver<plHashTable<K, T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};

template <typename K, typename T>
struct plContainerSubTypeResolver<plMap<K, T>>
{
  using Type = typename plTypeTraits<T>::NonConstReferenceType;
};


/// \brief Describes what kind of function a property is.
struct plFunctionType
{
  using StorageType = plUInt8;

  enum Enum
  {
    Member,       ///< A normal member function, a valid instance pointer must be provided to call.
    StaticMember, ///< A static member function, instance pointer will be ignored.
    Constructor,  ///< A constructor. Return value is a void* pointing to the new instance allocated with the default allocator.
    Default = Member
  };
};

/// \brief The base class for a property that represents a function.
class PL_FOUNDATION_DLL plAbstractFunctionProperty : public plAbstractProperty
{
public:
  /// \brief Passes the property name through to plAbstractProperty.
  plAbstractFunctionProperty(const char* szPropertyName)
    : plAbstractProperty(szPropertyName)
  {
  }

  virtual plPropertyCategory::Enum GetCategory() const override { return plPropertyCategory::Function; }
  /// \brief Returns the type of function, see plFunctionPropertyType::Enum.
  virtual plFunctionType::Enum GetFunctionType() const = 0;
  /// \brief Returns the type of the return value.
  virtual const plRTTI* GetReturnType() const = 0;
  /// \brief Returns property flags of the return value.
  virtual plBitflags<plPropertyFlags> GetReturnFlags() const = 0;
  /// \brief Returns the number of arguments.
  virtual plUInt32 GetArgumentCount() const = 0;
  /// \brief Returns the type of the given argument.
  virtual const plRTTI* GetArgumentType(plUInt32 uiParamIndex) const = 0;
  /// \brief Returns the property flags of the given argument.
  virtual plBitflags<plPropertyFlags> GetArgumentFlags(plUInt32 uiParamIndex) const = 0;

  /// \brief Calls the function. Provide the instance on which the function is supposed to be called.
  ///
  /// arguments must be the size of GetArgumentCount, the following rules apply for both arguments and return value:
  /// Any standard type must be provided by value, even if it is a pointer to one. Types must match exactly, no ConvertTo is called.
  /// enum and bitflags are supported if plEnum / plBitflags is used, value must be provided as plInt64.
  /// Out values (&, *) are written back to the variant they were read from.
  /// Any class is provided by pointer, regardless of whether it is a pointer or not.
  /// The returnValue must only be valid if the return value is a ref or by value class. In that case
  /// returnValue must be a ptr to a valid class instance of the returned type.
  /// An invalid variant is equal to a nullptr, except for if the argument is of type plVariant, in which case
  /// it is impossible to pass along a nullptr.
  virtual void Execute(void* pInstance, plArrayPtr<plVariant> arguments, plVariant& out_returnValue) const = 0;

  virtual const plRTTI* GetSpecificType() const override { return GetReturnType(); }

  /// \brief Adds flags to the property. Returns itself to allow to be called during initialization.
  plAbstractFunctionProperty* AddFlags(plBitflags<plPropertyFlags> flags)
  {
    return static_cast<plAbstractFunctionProperty*>(plAbstractProperty::AddFlags(flags));
  }

  /// \brief Adds attributes to the property. Returns itself to allow to be called during initialization. Allocate an attribute using
  /// standard 'new'.
  plAbstractFunctionProperty* AddAttributes(plPropertyAttribute* pAttrib1, plPropertyAttribute* pAttrib2 = nullptr, plPropertyAttribute* pAttrib3 = nullptr,
    plPropertyAttribute* pAttrib4 = nullptr, plPropertyAttribute* pAttrib5 = nullptr, plPropertyAttribute* pAttrib6 = nullptr)
  {
    return static_cast<plAbstractFunctionProperty*>(plAbstractProperty::AddAttributes(pAttrib1, pAttrib2, pAttrib3, pAttrib4, pAttrib5, pAttrib6));
  }
};
