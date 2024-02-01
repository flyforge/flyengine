#pragma once

/// \file

#include <Foundation/Reflection/Implementation/EnumProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief [internal] An implementation of plTypedEnumProperty that uses custom getter / setter functions to access a bitflags property.
template <typename Class, typename EnumType, typename Type>
class plBitflagsAccessorProperty : public plTypedEnumProperty<EnumType>
{
public:
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetterFunc = Type (Class::*)() const;
  using SetterFunc = void (Class::*)(Type value);

  /// \brief Constructor.
  plBitflagsAccessorProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter)
    : plTypedEnumProperty<EnumType>(szPropertyName)
  {
    PL_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::Bitflags);

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
    typename EnumType::StorageType enumTemp = (static_cast<const Class*>(pInstance)->*m_Getter)().GetValue();
    return (plInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, plInt64 value) const override // [tested]
  {
    PL_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    if (m_Setter)
      (static_cast<Class*>(pInstance)->*m_Setter)((typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
};


/// \brief [internal] An implementation of plTypedEnumProperty that accesses the bitflags property data directly.
template <typename Class, typename EnumType, typename Type>
class plBitflagsMemberProperty : public plTypedEnumProperty<EnumType>
{
public:
  using GetterFunc = Type (*)(const Class* pInstance);
  using SetterFunc = void (*)(Class* pInstance, Type value);
  using PointerFunc = void* (*)(const Class* pInstance);

  /// \brief Constructor.
  plBitflagsMemberProperty(const char* szPropertyName, GetterFunc getter, SetterFunc setter, PointerFunc pointer)
    : plTypedEnumProperty<EnumType>(szPropertyName)
  {
    PL_ASSERT_DEBUG(getter != nullptr, "The getter of a property cannot be nullptr.");
    plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::Bitflags);

    m_Getter = getter;
    m_Setter = setter;
    m_Pointer = pointer;

    if (m_Setter == nullptr)
      plAbstractMemberProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual void* GetPropertyPointer(const void* pInstance) const override { return m_Pointer(static_cast<const Class*>(pInstance)); }

  virtual plInt64 GetValue(const void* pInstance) const override // [tested]
  {
    typename EnumType::StorageType enumTemp = m_Getter(static_cast<const Class*>(pInstance)).GetValue();
    return (plInt64)enumTemp;
  }

  virtual void SetValue(void* pInstance, plInt64 value) const override // [tested]
  {
    PL_ASSERT_DEV(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());

    if (m_Setter)
      m_Setter(static_cast<Class*>(pInstance), (typename EnumType::Enum)value);
  }

private:
  GetterFunc m_Getter;
  SetterFunc m_Setter;
  PointerFunc m_Pointer;
};
