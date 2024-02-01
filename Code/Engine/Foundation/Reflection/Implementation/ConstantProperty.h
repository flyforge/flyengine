#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>
#include <Foundation/Reflection/Implementation/StaticRTTI.h>

/// \brief The base class for all typed member properties. Ie. once the type of a property is determined, it can be cast to the proper
/// version of this.
///
/// For example, when you have a pointer to an plAbstractMemberProperty and it returns that the property is of type 'int', you can cast the
/// pointer to an pointer to plTypedMemberProperty<int> which then allows you to access its values.
template <typename Type>
class plTypedConstantProperty : public plAbstractConstantProperty
{
public:
  /// \brief Passes the property name through to plAbstractMemberProperty.
  plTypedConstantProperty(const char* szPropertyName)
    : plAbstractConstantProperty(szPropertyName)
  {
    m_Flags = plPropertyFlags::GetParameterFlags<Type>();
  }

  /// \brief Returns the actual type of the property. You can then compare that with known types, eg. compare it to plGetStaticRTTI<int>()
  /// to see whether this is an int property.
  virtual const plRTTI* GetSpecificType() const override // [tested]
  {
    return plGetStaticRTTI<typename plTypeTraits<Type>::NonConstReferenceType>();
  }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const = 0;
};

/// \brief [internal] An implementation of plTypedConstantProperty that accesses the property data directly.
template <typename Type>
class plConstantProperty : public plTypedConstantProperty<Type>
{
public:
  /// \brief Constructor.
  plConstantProperty(const char* szPropertyName, Type value)
    : plTypedConstantProperty<Type>(szPropertyName)
    , m_Value(value)
  {
    PL_ASSERT_DEBUG(this->m_Flags.IsSet(plPropertyFlags::StandardType), "Only constants that can be put in an plVariant are currently supported!");
  }

  /// \brief Returns a pointer to the member property.
  virtual void* GetPropertyPointer() const override { return (void*)&m_Value; }

  /// \brief Returns the value of the property. Pass the instance pointer to the surrounding class along.
  virtual Type GetValue() const override // [tested]
  {
    return m_Value;
  }

  virtual plVariant GetConstant() const override { return plVariant(m_Value); }

private:
  Type m_Value;
};
