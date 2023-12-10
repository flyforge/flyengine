#pragma once

/// \file

#include <Foundation/Reflection/Implementation/DynamicRTTI.h>

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType* d = plStaticCast<DerivedType*>(pObj);
template <typename T>
PLASMA_ALWAYS_INLINE T plStaticCast(plReflectedClass* pObject)
{
  using NonPointerT = typename plTypeTraits<T>::NonPointerType;
  PLASMA_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), plGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType* d = plStaticCast<const DerivedType*>(pConstObj);
template <typename T>
PLASMA_ALWAYS_INLINE T plStaticCast(const plReflectedClass* pObject)
{
  using NonPointerT = typename plTypeTraits<T>::NonConstReferencePointerType;
  PLASMA_ASSERT_DEV(pObject == nullptr || pObject->IsInstanceOf<NonPointerT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    pObject->GetDynamicRTTI()->GetTypeName(), plGetStaticRTTI<NonPointerT>()->GetTypeName());
  return static_cast<T>(pObject);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. DerivedType& d = plStaticCast<DerivedType&>(obj);
template <typename T>
PLASMA_ALWAYS_INLINE T plStaticCast(plReflectedClass& in_object)
{
  using NonReferenceT = typename plTypeTraits<T>::NonReferenceType;
  PLASMA_ASSERT_DEV(in_object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    in_object.GetDynamicRTTI()->GetTypeName(), plGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(in_object);
}

/// \brief Casts the given object to the given type with no runtime cost (like C++ static_cast).
/// This function will assert when the object is not an instance of the given type.
/// E.g. const DerivedType& d = plStaticCast<const DerivedType&>(constObj);
template <typename T>
PLASMA_ALWAYS_INLINE T plStaticCast(const plReflectedClass& object)
{
  using NonReferenceT = typename plTypeTraits<T>::NonConstReferenceType;
  PLASMA_ASSERT_DEV(object.IsInstanceOf<NonReferenceT>(), "Invalid static cast: Object of type '{0}' is not an instance of '{1}'",
    object.GetDynamicRTTI()->GetTypeName(), plGetStaticRTTI<NonReferenceT>()->GetTypeName());
  return static_cast<T>(object);
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. DerivedType* d = plDynamicCast<DerivedType*>(pObj);
template <typename T>
PLASMA_ALWAYS_INLINE T plDynamicCast(plReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename plTypeTraits<T>::NonPointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}

/// \brief Casts the given object to the given type with by checking if the object is actually an instance of the given type (like C++
/// dynamic_cast). This function will return a nullptr if the object is not an instance of the given type.
/// E.g. const DerivedType* d = plDynamicCast<const DerivedType*>(pConstObj);
template <typename T>
PLASMA_ALWAYS_INLINE T plDynamicCast(const plReflectedClass* pObject)
{
  if (pObject)
  {
    using NonPointerT = typename plTypeTraits<T>::NonConstReferencePointerType;
    if (pObject->IsInstanceOf<NonPointerT>())
    {
      return static_cast<T>(pObject);
    }
  }
  return nullptr;
}
