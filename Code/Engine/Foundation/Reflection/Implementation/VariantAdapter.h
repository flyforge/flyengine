#pragma once

#include <Foundation/Types/Variant.h>

template <typename T>
struct plCleanType2
{
  using Type = T;
  using RttiType = T;
};

template <typename T>
struct plCleanType2<plEnum<T>>
{
  using Type = plEnum<T>;
  using RttiType = T;
};

template <typename T>
struct plCleanType2<plBitflags<T>>
{
  using Type = plBitflags<T>;
  using RttiType = T;
};

template <typename T>
struct plCleanType
{
  using Type = typename plTypeTraits<T>::NonConstReferencePointerType;
  using RttiType = typename plCleanType2<typename plTypeTraits<T>::NonConstReferencePointerType>::RttiType;
};

template <>
struct plCleanType<const char*>
{
  using Type = const char*;
  using RttiType = const char*;
};

//////////////////////////////////////////////////////////////////////////

template <typename T>
struct plIsOutParam
{
  enum
  {
    value = false,
  };
};

template <typename T>
struct plIsOutParam<T&>
{
  enum
  {
    value = !std::is_const<typename plTypeTraits<T>::NonReferencePointerType>::value,
  };
};

template <typename T>
struct plIsOutParam<T*>
{
  enum
  {
    value = !std::is_const<typename plTypeTraits<T>::NonReferencePointerType>::value,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type is a build-in standard variant type.
template <class T, class C = typename plCleanType<T>::Type>
struct plIsStandardType
{
  enum
  {
    value = plVariant::TypeDeduction<C>::value >= plVariantType::FirstStandardType && plVariant::TypeDeduction<C>::value <= plVariantType::LastStandardType,
  };
};

template <class T>
struct plIsStandardType<T, plVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to determine if the given type can be stored by value inside an plVariant (either standard type or custom type).
template <class T, class C = typename plCleanType<T>::Type>
struct plIsValueType
{
  enum
  {
    value = (plVariant::TypeDeduction<C>::value >= plVariantType::FirstStandardType && plVariant::TypeDeduction<C>::value <= plVariantType::LastStandardType) || plVariantTypeDeduction<C>::classification == plVariantClass::CustomTypeCast,
  };
};

template <class T>
struct plIsValueType<T, plVariant>
{
  enum
  {
    value = true,
  };
};

//////////////////////////////////////////////////////////////////////////
/// \brief Used to automatically assign any value to an plVariant using the assignment rules
/// outlined in plAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the value.
  class C = typename plCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = plIsValueType<T>::value> ///< Is 1 if T is a plTypeFlags::StandardType or a custom type
struct plVariantAssignmentAdapter
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(RealType* rhs) { m_value = rhs; }
  void operator=(RealType&& rhs)
  {
    if (m_value.IsValid())
      *m_value.Get<RealType*>() = rhs;
  }
  plVariant& m_value;
};

template <class T, class S>
struct plVariantAssignmentAdapter<T, plEnum<S>, 0>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(plEnum<S>&& rhs) { m_value = static_cast<plInt64>(rhs.GetValue()); }

  plVariant& m_value;
};

template <class T, class S>
struct plVariantAssignmentAdapter<T, plBitflags<S>, 0>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(plBitflags<S>&& rhs) { m_value = static_cast<plInt64>(rhs.GetValue()); }

  plVariant& m_value;
};

template <class T, class C>
struct plVariantAssignmentAdapter<T, C, 1>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  plVariant& m_value;
};

template <class T>
struct plVariantAssignmentAdapter<T, plVariantArray, 0>
{
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  plVariant& m_value;
};

template <class T>
struct plVariantAssignmentAdapter<T, plVariantDictionary, 0>
{
  plVariantAssignmentAdapter(plVariant& value)
    : m_value(value)
  {
  }

  void operator=(T&& rhs) { m_value = rhs; }

  plVariant& m_value;
};

//////////////////////////////////////////////////////////////////////////

/// \brief Used to implicitly retrieve any value from an plVariant to be used as a function argument
/// using the assignment rules outlined in plAbstractFunctionProperty::Execute.
template <class T,                          ///< Only this parameter needs to be provided, the actual type of the argument. Rest is used to force specializations.
  class C = typename plCleanType<T>::Type,  ///< Same as T but without the const&* fluff.
  int VALUE_TYPE = plIsValueType<T>::value, ///< Is 1 if T is a plTypeFlags::StandardType or a custom type
  int OUT_PARAM = plIsOutParam<T>::value>   ///< Is 1 if T a non-const reference or pointer.
struct plVariantAdapter
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;

  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator RealType&() { return *m_value.Get<RealType*>(); }

  operator RealType*() { return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr; }

  plVariant& m_value;
};

template <class T, class S>
struct plVariantAdapter<T, plEnum<S>, 0, 0>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<plInt64>());
  }

  operator const plEnum<S>&() { return m_realValue; }
  operator const plEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  plVariant& m_value;
  plEnum<S> m_realValue;
};

template <class T, class S>
struct plVariantAdapter<T, plEnum<S>, 0, 1>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue = static_cast<typename S::Enum>(m_value.ConvertTo<plInt64>());
  }
  ~plVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<plInt64>(m_realValue.GetValue());
  }

  operator plEnum<S>&() { return m_realValue; }
  operator plEnum<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  plVariant& m_value;
  plEnum<S> m_realValue;
};

template <class T, class S>
struct plVariantAdapter<T, plBitflags<S>, 0, 0>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<plInt64>()));
  }

  operator const plBitflags<S>&() { return m_realValue; }
  operator const plBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  plVariant& m_value;
  plBitflags<S> m_realValue;
};

template <class T, class S>
struct plVariantAdapter<T, plBitflags<S>, 0, 1>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
    if (m_value.IsValid())
      m_realValue.SetValue(static_cast<typename S::StorageType>(m_value.ConvertTo<plInt64>()));
  }
  ~plVariantAdapter()
  {
    if (m_value.IsValid())
      m_value = static_cast<plInt64>(m_realValue.GetValue());
  }

  operator plBitflags<S>&() { return m_realValue; }
  operator plBitflags<S>*() { return m_value.IsValid() ? &m_realValue : nullptr; }

  plVariant& m_value;
  plBitflags<S> m_realValue;
};

template <class T, class C>
struct plVariantAdapter<T, C, 1, 0>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator const C&()
  {
    if constexpr (plVariantTypeDeduction<C>::classification == plVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == plVariantType::TypedPointer)
        return *m_value.Get<RealType*>();
    }
    return m_value.Get<RealType>();
  }

  operator const C*()
  {
    if constexpr (plVariantTypeDeduction<C>::classification == plVariantClass::CustomTypeCast)
    {
      if (m_value.GetType() == plVariantType::TypedPointer)
        return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    }
    return m_value.IsValid() ? &m_value.Get<RealType>() : nullptr;
  }

  plVariant& m_value;
};

template <class T, class C>
struct plVariantAdapter<T, C, 1, 1>
{
  using RealType = typename plTypeTraits<T>::NonConstReferencePointerType;
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
    // We ignore the return value here instead const_cast the Get<> result to profit from the Get methods runtime type checks.
    m_value.GetWriteAccess();
  }

  operator C&()
  {
    if (m_value.GetType() == plVariantType::TypedPointer)
      return *m_value.Get<RealType*>();
    else
      return const_cast<RealType&>(m_value.Get<RealType>());
  }
  operator C*()
  {
    if (m_value.GetType() == plVariantType::TypedPointer)
      return m_value.IsValid() ? m_value.Get<RealType*>() : nullptr;
    else
      return m_value.IsValid() ? &const_cast<RealType&>(m_value.Get<RealType>()) : nullptr;
  }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariant, 1, 0>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator const plVariant&() { return m_value; }
  operator const plVariant*() { return &m_value; }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariant, 1, 1>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator plVariant&() { return m_value; }
  operator plVariant*() { return &m_value; }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariantArray, 0, 0>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator const plVariantArray&() { return m_value.Get<plVariantArray>(); }
  operator const plVariantArray*() { return m_value.IsValid() ? &m_value.Get<plVariantArray>() : nullptr; }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariantArray, 0, 1>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator plVariantArray&() { return m_value.GetWritable<plVariantArray>(); }
  operator plVariantArray*() { return m_value.IsValid() ? &m_value.GetWritable<plVariantArray>() : nullptr; }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariantDictionary, 0, 0>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator const plVariantDictionary&() { return m_value.Get<plVariantDictionary>(); }
  operator const plVariantDictionary*() { return m_value.IsValid() ? &m_value.Get<plVariantDictionary>() : nullptr; }

  plVariant& m_value;
};

template <class T>
struct plVariantAdapter<T, plVariantDictionary, 0, 1>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator plVariantDictionary&() { return m_value.GetWritable<plVariantDictionary>(); }
  operator plVariantDictionary*() { return m_value.IsValid() ? &m_value.GetWritable<plVariantDictionary>() : nullptr; }

  plVariant& m_value;
};

template <>
struct plVariantAdapter<const char*, const char*, 1, 0>
{
  plVariantAdapter(plVariant& value)
    : m_value(value)
  {
  }

  operator const char*() { return m_value.IsValid() ? m_value.Get<plString>().GetData() : nullptr; }

  plVariant& m_value;
};
