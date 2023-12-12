#pragma once

#include <Foundation/Containers/Set.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Strings/HashedString.h>
#include <Foundation/Types/Bitflags.h>
#include <Foundation/Types/Enum.h>
#include <Foundation/Types/Id.h>
#include <Foundation/Types/Variant.h>
#include <ToolsFoundation/ToolsFoundationDLL.h>

class plRTTI;
class plPhantomRttiManager;
class plReflectedTypeStorageManager;

/// \brief Event message used by the plPhantomRttiManager.
struct PLASMA_TOOLSFOUNDATION_DLL plPhantomTypeChange
{
  const plRTTI* m_pChangedType = nullptr;
};

struct PLASMA_TOOLSFOUNDATION_DLL plAttributeHolder
{
  plAttributeHolder();
  plAttributeHolder(const plAttributeHolder& rhs);
  virtual ~plAttributeHolder();

  plUInt32 GetCount() const;
  const plPropertyAttribute* GetValue(plUInt32 uiIndex) const;
  void SetValue(plUInt32 uiIndex, const plPropertyAttribute* value);
  void Insert(plUInt32 uiIndex, const plPropertyAttribute* value);
  void Remove(plUInt32 uiIndex);

  void operator=(const plAttributeHolder& rhs);

  mutable plHybridArray<const plPropertyAttribute*, 2> m_Attributes;
  plArrayPtr<const plPropertyAttribute* const> m_ReferenceAttributes;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plAttributeHolder);

/// \brief Stores the description of a reflected property in a serializable form, used by plReflectedTypeDescriptor.
struct PLASMA_TOOLSFOUNDATION_DLL plReflectedPropertyDescriptor : public plAttributeHolder
{
  plReflectedPropertyDescriptor() = default;
  plReflectedPropertyDescriptor(plPropertyCategory::Enum category, plStringView sName, plStringView sType, plBitflags<plPropertyFlags> flags);
  plReflectedPropertyDescriptor(plPropertyCategory::Enum category, plStringView sName, plStringView sType, plBitflags<plPropertyFlags> flags,
    plArrayPtr<const plPropertyAttribute* const> attributes); // [tested]
  /// \brief Initialize to a constant.
  plReflectedPropertyDescriptor(plStringView sName, const plVariant& constantValue, plArrayPtr<const plPropertyAttribute* const> attributes); // [tested]
  plReflectedPropertyDescriptor(const plReflectedPropertyDescriptor& rhs);
  ~plReflectedPropertyDescriptor();

  void operator=(const plReflectedPropertyDescriptor& rhs);

  plEnum<plPropertyCategory> m_Category;
  plString m_sName; ///< The name of this property. E.g. what plAbstractProperty::GetPropertyName() returns.
  plString m_sType; ///< The name of the type of the property. E.g. plAbstractProperty::GetSpecificType().GetTypeName()

  plBitflags<plPropertyFlags> m_Flags;
  plVariant m_ConstantValue;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plReflectedPropertyDescriptor);

struct PLASMA_TOOLSFOUNDATION_DLL plFunctionArgumentDescriptor
{
  plFunctionArgumentDescriptor();
  plFunctionArgumentDescriptor(plStringView sType, plBitflags<plPropertyFlags> flags);
  plString m_sType;
  plBitflags<plPropertyFlags> m_Flags;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plFunctionArgumentDescriptor);

/// \brief Stores the description of a reflected function in a serializable form, used by plReflectedTypeDescriptor.
struct PLASMA_TOOLSFOUNDATION_DLL plReflectedFunctionDescriptor : public plAttributeHolder
{
  plReflectedFunctionDescriptor();
  plReflectedFunctionDescriptor(plStringView sName, plBitflags<plPropertyFlags> flags, plEnum<plFunctionType> type, plArrayPtr<const plPropertyAttribute* const> attributes);

  plReflectedFunctionDescriptor(const plReflectedFunctionDescriptor& rhs);
  ~plReflectedFunctionDescriptor();

  void operator=(const plReflectedFunctionDescriptor& rhs);

  plString m_sName;
  plBitflags<plPropertyFlags> m_Flags;
  plEnum<plFunctionType> m_Type;
  plFunctionArgumentDescriptor m_ReturnValue;
  plDynamicArray<plFunctionArgumentDescriptor> m_Arguments;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plReflectedFunctionDescriptor);


/// \brief Stores the description of a reflected type in a serializable form. Used by plPhantomRttiManager to add new types.
struct PLASMA_TOOLSFOUNDATION_DLL plReflectedTypeDescriptor : public plAttributeHolder
{
  ~plReflectedTypeDescriptor();

  plString m_sTypeName;
  plString m_sPluginName;
  plString m_sParentTypeName;

  plBitflags<plTypeFlags> m_Flags;
  plDynamicArray<plReflectedPropertyDescriptor> m_Properties;
  plDynamicArray<plReflectedFunctionDescriptor> m_Functions;
  plUInt32 m_uiTypeVersion = 1;
};
PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_TOOLSFOUNDATION_DLL, plReflectedTypeDescriptor);