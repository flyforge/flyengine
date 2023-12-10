#pragma once

/// \file

#include <Foundation/Reflection/Implementation/AbstractProperty.h>

class plRTTI;

/// \brief Do not cast into this class or any of its derived classes, use plTypedArrayProperty instead.
template <typename Type>
class plTypedArrayProperty : public plAbstractArrayProperty
{
public:
  plTypedArrayProperty(const char* szPropertyName)
    : plAbstractArrayProperty(szPropertyName)
  {
    m_Flags = plPropertyFlags::GetParameterFlags<Type>();
    PLASMA_CHECK_AT_COMPILETIME_MSG(!std::is_pointer<Type>::value ||
                                  plVariantTypeDeduction<typename plTypeTraits<Type>::NonConstReferencePointerType>::value ==
                                    plVariantType::Invalid,
      "Pointer to standard types are not supported.");
  }

  virtual const plRTTI* GetSpecificType() const override { return plGetStaticRTTI<typename plTypeTraits<Type>::NonConstReferencePointerType>(); }
};

/// \brief Specialization of plTypedArrayProperty to retain the pointer in const char*.
template <>
class plTypedArrayProperty<const char*> : public plAbstractArrayProperty
{
public:
  plTypedArrayProperty(const char* szPropertyName)
    : plAbstractArrayProperty(szPropertyName)
  {
    m_Flags = plPropertyFlags::GetParameterFlags<const char*>();
  }

  virtual const plRTTI* GetSpecificType() const override { return plGetStaticRTTI<const char*>(); }
};


template <typename Class, typename Type>
class plAccessorArrayProperty : public plTypedArrayProperty<Type>
{
public:
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetCountFunc = plUInt32 (Class::*)() const;
  using GetValueFunc = Type (Class::*)(plUInt32 uiIndex) const;
  using SetValueFunc = void (Class::*)(plUInt32 uiIndex, Type value);
  using InsertFunc = void (Class::*)(plUInt32 uiIndex, Type value);
  using RemoveFunc = void (Class::*)(plUInt32 uiIndex);


  plAccessorArrayProperty(
    const char* szPropertyName, GetCountFunc getCount, GetValueFunc getter, SetValueFunc setter, InsertFunc insert, RemoveFunc remove)
    : plTypedArrayProperty<Type>(szPropertyName)
  {
    PLASMA_ASSERT_DEBUG(getCount != nullptr, "The get count function of an array property cannot be nullptr.");
    PLASMA_ASSERT_DEBUG(getter != nullptr, "The get value function of an array property cannot be nullptr.");

    m_GetCount = getCount;
    m_Getter = getter;
    m_Setter = setter;
    m_Insert = insert;
    m_Remove = remove;

    if (m_Setter == nullptr)
      plAbstractArrayProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }


  virtual plUInt32 GetCount(const void* pInstance) const override { return (static_cast<const Class*>(pInstance)->*m_GetCount)(); }

  virtual void GetValue(const void* pInstance, plUInt32 uiIndex, void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = (static_cast<const Class*>(pInstance)->*m_Getter)(uiIndex);
  }

  virtual void SetValue(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Setter != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Setter)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Insert(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Insert != nullptr, "The property '{0}' has no insert function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Insert)(uiIndex, *static_cast<const RealType*>(pObject));
  }

  virtual void Remove(void* pInstance, plUInt32 uiIndex) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Remove != nullptr, "The property '{0}' has no setter function, thus it is read-only.", plAbstractProperty::GetPropertyName());
    (static_cast<Class*>(pInstance)->*m_Remove)(uiIndex);
  }

  virtual void Clear(void* pInstance) const override { SetCount(pInstance, 0); }

  virtual void SetCount(void* pInstance, plUInt32 uiCount) const override
  {
    PLASMA_ASSERT_DEBUG(m_Insert != nullptr && m_Remove != nullptr, "The property '{0}' has no remove and insert function, thus it is fixed-size.",
      plAbstractProperty::GetPropertyName());
    while (uiCount < GetCount(pInstance))
    {
      Remove(pInstance, GetCount(pInstance) - 1);
    }
    while (uiCount > GetCount(pInstance))
    {
      RealType elem = RealType();
      Insert(pInstance, GetCount(pInstance), &elem);
    }
  }

private:
  GetCountFunc m_GetCount;
  GetValueFunc m_Getter;
  SetValueFunc m_Setter;
  InsertFunc m_Insert;
  RemoveFunc m_Remove;
};



template <typename Class, typename Container, Container Class::*Member>
struct plArrayPropertyAccessor
{
  using ContainerType = typename plTypeTraits<Container>::NonConstReferenceType;
  using Type = typename plTypeTraits<typename plContainerSubTypeResolver<ContainerType>::Type>::NonConstReferenceType;

  static const ContainerType& GetConstContainer(const Class* pInstance) { return (*pInstance).*Member; }

  static ContainerType& GetContainer(Class* pInstance) { return (*pInstance).*Member; }
};


template <typename Class, typename Container, typename Type>
class plMemberArrayProperty : public plTypedArrayProperty<typename plTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);
  using GetContainerFunc = Container& (*)(Class* pInstance);

  plMemberArrayProperty(const char* szPropertyName, GetConstContainerFunc constGetter, GetContainerFunc getter)
    : plTypedArrayProperty<RealType>(szPropertyName)
  {
    PLASMA_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    m_Getter = getter;

    if (m_Getter == nullptr)
      plAbstractArrayProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual plUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, plUInt32 uiIndex, void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "SetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance))[uiIndex] = *static_cast<const RealType*>(pObject);
  }

  virtual void Insert(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex <= GetCount(pInstance), "Insert: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Insert(*static_cast<const RealType*>(pObject), uiIndex);
  }

  virtual void Remove(void* pInstance, plUInt32 uiIndex) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "Remove: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    PLASMA_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).RemoveAtAndCopy(uiIndex);
  }

  virtual void Clear(void* pInstance) const override
  {
    PLASMA_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).Clear();
  }

  virtual void SetCount(void* pInstance, plUInt32 uiCount) const override
  {
    PLASMA_ASSERT_DEBUG(m_Getter != nullptr, "The property '{0}' has no non-const array accessor function, thus it is read-only.",
      plAbstractProperty::GetPropertyName());
    m_Getter(static_cast<Class*>(pInstance)).SetCount(uiCount);
  }

  virtual void* GetValuePointer(void* pInstance, plUInt32 uiIndex) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    return &(m_Getter(static_cast<Class*>(pInstance))[uiIndex]);
  }

private:
  GetConstContainerFunc m_ConstGetter;
  GetContainerFunc m_Getter;
};

/// \brief Read only version of plMemberArrayProperty that does not call any functions that modify the array. This is needed to reflect plArrayPtr members.
template <typename Class, typename Container, typename Type>
class plMemberArrayReadOnlyProperty : public plTypedArrayProperty<typename plTypeTraits<Type>::NonConstReferenceType>
{
public:
  using RealType = typename plTypeTraits<Type>::NonConstReferenceType;
  using GetConstContainerFunc = const Container& (*)(const Class* pInstance);

  plMemberArrayReadOnlyProperty(const char* szPropertyName, GetConstContainerFunc constGetter)
    : plTypedArrayProperty<RealType>(szPropertyName)
  {
    PLASMA_ASSERT_DEBUG(constGetter != nullptr, "The const get count function of an array property cannot be nullptr.");

    m_ConstGetter = constGetter;
    plAbstractArrayProperty::m_Flags.Add(plPropertyFlags::ReadOnly);
  }

  virtual plUInt32 GetCount(const void* pInstance) const override { return m_ConstGetter(static_cast<const Class*>(pInstance)).GetCount(); }

  virtual void GetValue(const void* pInstance, plUInt32 uiIndex, void* pObject) const override
  {
    PLASMA_ASSERT_DEBUG(uiIndex < GetCount(pInstance), "GetValue: uiIndex ('{0}') is out of range ('{1}')", uiIndex, GetCount(pInstance));
    *static_cast<RealType*>(pObject) = m_ConstGetter(static_cast<const Class*>(pInstance))[uiIndex];
  }

  virtual void SetValue(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_REPORT_FAILURE("The property '{0}' is read-only.", plAbstractProperty::GetPropertyName());
  }

  virtual void Insert(void* pInstance, plUInt32 uiIndex, const void* pObject) const override
  {
    PLASMA_REPORT_FAILURE("The property '{0}' is read-only.", plAbstractProperty::GetPropertyName());
  }

  virtual void Remove(void* pInstance, plUInt32 uiIndex) const override
  {
    PLASMA_REPORT_FAILURE("The property '{0}' is read-only.", plAbstractProperty::GetPropertyName());
  }

  virtual void Clear(void* pInstance) const override
  {
    PLASMA_REPORT_FAILURE("The property '{0}' is read-only.", plAbstractProperty::GetPropertyName());
  }

  virtual void SetCount(void* pInstance, plUInt32 uiCount) const override
  {
    PLASMA_REPORT_FAILURE("The property '{0}' is read-only.", plAbstractProperty::GetPropertyName());
  }

private:
  GetConstContainerFunc m_ConstGetter;
};
