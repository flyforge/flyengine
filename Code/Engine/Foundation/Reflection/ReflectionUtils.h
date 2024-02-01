#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>

class plVariant;
class plAbstractProperty;

/// \brief Helper functions for handling reflection related operations.
class PL_FOUNDATION_DLL plReflectionUtils
{
public:
  static const plRTTI* GetCommonBaseType(const plRTTI* pRtti1, const plRTTI* pRtti2);

  /// \brief Returns whether a type can be stored directly inside a plVariant.
  static bool IsBasicType(const plRTTI* pRtti);

  /// \brief Returns whether the property is a non-ptr basic type or custom type.
  static bool IsValueType(const plAbstractProperty* pProp);

  /// \brief Returns the RTTI type matching the variant's type.
  static const plRTTI* GetTypeFromVariant(const plVariant& value);
  static const plRTTI* GetTypeFromVariant(plVariantType::Enum type);

  /// \brief Sets the Nth component of the vector to the given value.
  ///
  /// vector's type needs to be in between plVariant::Type::Vector2 and plVariant::Type::Vector4U.
  static plUInt32 GetComponentCount(plVariantType::Enum type);
  static void SetComponent(plVariant& ref_vector, plUInt32 uiComponent, double fValue); // [tested]
  static double GetComponent(const plVariant& vector, plUInt32 uiComponent);

  static plVariant GetMemberPropertyValue(const plAbstractMemberProperty* pProp, const void* pObject);        // [tested] via ToolsFoundation
  static void SetMemberPropertyValue(const plAbstractMemberProperty* pProp, void* pObject, const plVariant& value); // [tested] via ToolsFoundation

  static plVariant GetArrayPropertyValue(const plAbstractArrayProperty* pProp, const void* pObject, plUInt32 uiIndex);
  static void SetArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex, const plVariant& value);

  static void InsertSetPropertyValue(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value);
  static void RemoveSetPropertyValue(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value);

  static plVariant GetMapPropertyValue(const plAbstractMapProperty* pProp, const void* pObject, const char* szKey);
  static void SetMapPropertyValue(const plAbstractMapProperty* pProp, void* pObject, const char* szKey, const plVariant& value);

  static void InsertArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, const plVariant& value, plUInt32 uiIndex);
  static void RemoveArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex);

  static const plAbstractMemberProperty* GetMemberProperty(const plRTTI* pRtti, plUInt32 uiPropertyIndex);
  static const plAbstractMemberProperty* GetMemberProperty(const plRTTI* pRtti, const char* szPropertyName); // [tested] via ToolsFoundation

  /// \brief Gathers all RTTI types that are derived from pRtti.
  ///
  /// This includes all classes that have pRtti as a base class, either direct or indirect.
  ///
  /// \sa GatherDependentTypes
  static void GatherTypesDerivedFromClass(const plRTTI* pRtti, plSet<const plRTTI*>& out_types);

  /// \brief Gathers all RTTI types that pRtti depends on and adds them to inout_types.
  ///
  /// Dependencies are either member properties or base classes. The output contains the transitive closure of the dependencies.
  /// Note that inout_typesAsSet is not cleared when this function is called.
  /// out_pTypesAsStack is all the dependencies sorted by their appearance in the dependency chain.
  /// The last entry is the lowest in the chain and has no dependencies on its own.
  static void GatherDependentTypes(const plRTTI* pRtti, plSet<const plRTTI*>& inout_typesAsSet, plDynamicArray<const plRTTI*>* out_pTypesAsStack = nullptr);

  /// \brief Sorts the input types according to their dependencies.
  ///
  /// Types that have no dependences come first in the output followed by types that have their dependencies met by
  /// the previous entries in the output.
  /// If a dependent type is not in the given types set the function will fail.
  static plResult CreateDependencySortedTypeArray(const plSet<const plRTTI*>& types, plDynamicArray<const plRTTI*>& out_sortedTypes);

  struct EnumConversionMode
  {
    enum Enum
    {
      FullyQualifiedName,
      ValueNameOnly,
      Default = FullyQualifiedName
    };

    using StorageType = plUInt8;
  };

  /// \brief Converts an enum or bitfield value into its string representation.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of out_sOutput equals MSVC debugger output.
  static bool EnumerationToString(const plRTTI* pEnumerationRtti, plInt64 iValue, plStringBuilder& out_sOutput,
    plEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default); // [tested]

  /// \brief Helper template to shorten the call for plEnums
  template <typename T>
  static bool EnumerationToString(plEnum<T> value, plStringBuilder& out_sOutput, plEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(plGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  /// \brief Helper template to shorten the call for plBitflags
  template <typename T>
  static bool BitflagsToString(plBitflags<T> value, plStringBuilder& out_sOutput, plEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default)
  {
    return EnumerationToString(plGetStaticRTTI<T>(), value.GetValue(), out_sOutput, conversionMode);
  }

  struct EnumKeyValuePair
  {
    plString m_sKey;
    plInt32 m_iValue = 0;
  };

  /// \brief If the given type is an enum, \a entries will be filled with all available keys (strings) and values (integers).
  static void GetEnumKeysAndValues(const plRTTI* pEnumerationRtti, plDynamicArray<EnumKeyValuePair>& ref_entries, plEnum<EnumConversionMode> conversionMode = EnumConversionMode::Default);

  /// \brief Converts an enum or bitfield in its string representation to its value.
  ///
  /// The type of pEnumerationRtti will be automatically detected. The syntax of szValue must equal the MSVC debugger output.
  static bool StringToEnumeration(const plRTTI* pEnumerationRtti, const char* szValue, plInt64& out_iValue); // [tested]

  /// \brief Helper template to shorten the call for plEnums
  template <typename T>
  static bool StringToEnumeration(const char* szValue, plEnum<T>& out_value)
  {
    plInt64 value;
    const auto retval = StringToEnumeration(plGetStaticRTTI<T>(), szValue, value);
    out_value = static_cast<typename T::Enum>(value);
    return retval;
  }

  /// \brief Returns the default value (Enum::Default) for the given enumeration type.
  static plInt64 DefaultEnumerationValue(const plRTTI* pEnumerationRtti); // [tested]

  /// \brief Makes sure the given value is valid under the given enumeration type.
  ///
  /// Invalid bitflag bits are removed and an invalid enum value is replaced by the default value.
  static plInt64 MakeEnumerationValid(const plRTTI* pEnumerationRtti, plInt64 iValue); // [tested]

  /// \brief Templated convenience function that calls IsEqual and automatically deduces the type.
  template <typename T>
  static bool IsEqual(const T* pObject, const T* pObject2)
  {
    return IsEqual(pObject, pObject2, plGetStaticRTTI<T>());
  }

  /// \brief Compares pObject with pObject2 of type pType and returns whether they are equal.
  ///
  /// In case a class derived from plReflectedClass is passed in the correct derived type
  /// will automatically be determined so it is not necessary to put the exact type into pType,
  /// any derived class type will do. However, the function will return false  pObject and pObject2
  /// actually have a different type.
  static bool IsEqual(const void* pObject, const void* pObject2, const plRTTI* pType); // [tested]

  /// \brief Compares property pProp of pObject and pObject2 and returns whether it is equal in both.
  static bool IsEqual(const void* pObject, const void* pObject2, const plAbstractProperty* pProp);

  /// \brief Deletes pObject using the allocator found in the owning property's type.
  static void DeleteObject(void* pObject, const plAbstractProperty* pOwnerProperty);

  /// \brief Returns a global default initialization value for the given variant type.
  static plVariant GetDefaultVariantFromType(plVariant::Type::Enum type); // [tested]

  /// \brief Returns the default value for the specific type
  static plVariant GetDefaultVariantFromType(const plRTTI* pRtti);

  /// \brief Returns the default value for the specific type of the given property.
  static plVariant GetDefaultValue(const plAbstractProperty* pProperty, plVariant index = plVariant());


  /// \brief Sets all member properties in \a pObject of type \a pRtti to the value returned by plToolsReflectionUtils::GetDefaultValue()
  static void SetAllMemberPropertiesToDefault(const plRTTI* pRtti, void* pObject);

  /// \brief If pAttrib is valid and its min/max values are compatible, value will be clamped to them.
  /// Returns false if a clamp attribute exists but no clamp code was executed.
  static plResult ClampValue(plVariant& value, const plClampValueAttribute* pAttrib);
};
