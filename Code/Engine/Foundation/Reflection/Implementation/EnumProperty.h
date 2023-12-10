#pragma once

/// \file

#include <Foundation/Reflection/Implementation/MemberProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief The base class for enum and bitflags member properties.
///
/// Cast any property whose type derives from plEnumBase or plBitflagsBase class to access its value.
class plAbstractEnumerationProperty : public plAbstractMemberProperty
{
public:
  /// \brief Passes the property name through to plAbstractMemberProperty.
  plAbstractEnumerationProperty(const char* szPropertyName)
    : plAbstractMemberProperty(szPropertyName)
  {
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual plInt64 GetValue(const void* pInstance) const = 0;

  /// \brief Modifies the value of the property. Pass the instance pointer to the surrounding class along.
  ///
  /// \note Make sure the property is not read-only before calling this, otherwise an assert will fire.
  virtual void SetValue(void* pInstance, plInt64 value) const = 0;

  virtual void GetValuePtr(const void* pInstance, void* pObject) const override
  {
    *static_cast<plInt64*>(pObject) = GetValue(pInstance);
  }

  virtual void SetValuePtr(void* pInstance, const void* pObject) const override
  {
    SetValue(pInstance, *static_cast<const plInt64*>(pObject));
  }
};


/// \brief [internal] Base class for enum / bitflags properties that already defines the type.
template <typename EnumType>
class plTypedEnumProperty : public plAbstractEnumerationProperty
{
public:
  /// \brief Passes the property name through to plAbstractEnumerationProperty.
  plTypedEnumProperty(const char* szPropertyName)
    : plAbstractEnumerationProperty(szPropertyName)
  {
  }

  /// \brief Returns the actual type of the property. You can then test whether it derives from plEnumBase or
  ///  plBitflagsBase to determine whether we are dealing with an enum or bitflags property.
  virtual const plRTTI* GetSpecificType() const override // [tested]
  {
    return plGetStaticRTTI<typename plTypeTraits<EnumType>::NonConstReferenceType>();
  }
};


/// \brief [internal] An implementation of plTypedEnumProperty that uses custom getter / setter functions to access an enum property.
template <typename Class, typename EnumType, typename Type>
class plEnumAccessorProperty : public plTypedEnumProperty<EnumType>
{
public:
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// \brief Constructor.
  plEnumAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : plTypedEnumProperty<EnumType>(szPropertyName)
  {
    PLASMA_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::IsEnum);

    m_Getter = getter;
    m_Setter = setter;

    if (m_Setter == nullptr)
      plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override
  {
    // No access to sub-properties, if we have accessors for this property
    return nullptr;
  }

  virtual plInt64 GetValue(const void* pInstance) const override // [tested]
  {
    plEnum<EnumType> enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)();
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, plInt64 value) const override // [tested]
  {
    PLASMA_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// \brief [internal] An implementation of plTypedEnumProperty that accesses the enum property data directly.
template <typename Class, typename EnumType, typename Type>
class plEnumMemberProperty : public plTypedEnumProperty<EnumType>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// \brief Constructor.
  plEnumMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : plTypedEnumProperty<EnumType>(szPropertyName)
  {
    PLASMA_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::IsEnum);

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual plInt64 GetValue(const void* pInstance) const override // [tested]
  {
    plEnum<EnumType> enumTemp = m_Getter(static_cast<const Class*>(pInstance));
    return enumTemp.GetValue();
  }

  virtual void SetValue(void* pInstance, plInt64 value) const override // [tested]
  {
    PLASMA_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
