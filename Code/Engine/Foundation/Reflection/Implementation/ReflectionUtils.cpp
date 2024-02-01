#include <Foundation/FoundationPCH.h>

#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Types/VariantTypeRegistry.h>

namespace
{
  // for some reason MSVC does not accept the template keyword here
#if PL_ENABLED(PL_COMPILER_MSVC_PURE)
#  define CALL_FUNCTOR(functor, type) functor.operator()<type>(std::forward<Args>(args)...)
#else
#  define CALL_FUNCTOR(functor, type) functor.template operator()<type>(std::forward<Args>(args)...)
#endif

  template <typename Functor, class... Args>
  void DispatchTo(Functor& ref_functor, const plAbstractProperty* pProp, Args&&... args)
  {
    const bool bIsPtr = pProp->GetFlags().IsSet(plPropertyFlags::Pointer);
    if (bIsPtr)
    {
      CALL_FUNCTOR(ref_functor, plTypedPointer);
      return;
    }
    else if (pProp->GetSpecificType() == plGetStaticRTTI<const char*>())
    {
      CALL_FUNCTOR(ref_functor, const char*);
      return;
    }
    else if (pProp->GetSpecificType() == plGetStaticRTTI<plUntrackedString>())
    {
      CALL_FUNCTOR(ref_functor, plUntrackedString);
      return;
    }
    else if (pProp->GetSpecificType() == plGetStaticRTTI<plVariant>())
    {
      CALL_FUNCTOR(ref_functor, plVariant);
      return;
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::StandardType))
    {
      plVariant::DispatchTo(ref_functor, pProp->GetSpecificType()->GetVariantType(), std::forward<Args>(args)...);
      return;
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::IsEnum))
    {
      CALL_FUNCTOR(ref_functor, plEnumBase);
      return;
    }
    else if (pProp->GetFlags().IsSet(plPropertyFlags::Bitflags))
    {
      CALL_FUNCTOR(ref_functor, plBitflagsBase);
      return;
    }
    else if (pProp->GetSpecificType()->GetVariantType() == plVariantType::TypedObject)
    {
      CALL_FUNCTOR(ref_functor, plTypedObject);
      return;
    }

    PL_REPORT_FAILURE("Unknown dispatch type");
  }

#undef CALL_FUNCTOR

  struct GetTypeFromVariantTypeFunc
  {
    template <typename T>
    PL_ALWAYS_INLINE void operator()()
    {
      m_pType = plGetStaticRTTI<T>();
    }
    const plRTTI* m_pType;
  };

  template <>
  PL_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<plTypedPointer>()
  {
    m_pType = nullptr;
  }
  template <>
  PL_ALWAYS_INLINE void GetTypeFromVariantTypeFunc::operator()<plTypedObject>()
  {
    m_pType = nullptr;
  }

  //////////////////////////////////////////////////////////////////////////



  template <typename T>
  struct plPropertyValue
  {
    using Type = T;
    using StorageType = typename plVariantTypeDeduction<T>::StorageType;
  };
  template <>
  struct plPropertyValue<plEnumBase>
  {
    using Type = plInt64;
    using StorageType = plInt64;
  };
  template <>
  struct plPropertyValue<plBitflagsBase>
  {
    using Type = plInt64;
    using StorageType = plInt64;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct plVariantFromProperty
  {
    plVariantFromProperty(plVariant& value, const plAbstractProperty* pProp)
      : m_value(value)
    {
    }
    ~plVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = m_tempValue;
    }

    operator void*()
    {
      return &m_tempValue;
    }

    plVariant& m_value;
    typename plPropertyValue<T>::Type m_tempValue = {};
    bool m_bSuccess = true;
  };

  template <>
  struct plVariantFromProperty<plVariant>
  {
    plVariantFromProperty(plVariant& value, const plAbstractProperty* pProp)
      : m_value(value)
    {
    }

    operator void*()
    {
      return &m_value;
    }

    plVariant& m_value;
    bool m_bSuccess = true;
  };

  template <>
  struct plVariantFromProperty<plTypedPointer>
  {
    plVariantFromProperty(plVariant& value, const plAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
    }
    ~plVariantFromProperty()
    {
      if (m_bSuccess)
        m_value = plVariant(m_ptr, m_pProp->GetSpecificType());
    }

    operator void*()
    {
      return &m_ptr;
    }

    plVariant& m_value;
    const plAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  template <>
  struct plVariantFromProperty<plTypedObject>
  {
    plVariantFromProperty(plVariant& value, const plAbstractProperty* pProp)
      : m_value(value)
      , m_pProp(pProp)
    {
      m_ptr = m_pProp->GetSpecificType()->GetAllocator()->Allocate<void>();
    }
    ~plVariantFromProperty()
    {
      if (m_bSuccess)
        m_value.MoveTypedObject(m_ptr, m_pProp->GetSpecificType());
      else
        m_pProp->GetSpecificType()->GetAllocator()->Deallocate(m_ptr);
    }

    operator void*()
    {
      return m_ptr;
    }

    plVariant& m_value;
    const plAbstractProperty* m_pProp = nullptr;
    void* m_ptr = nullptr;
    bool m_bSuccess = true;
  };

  //////////////////////////////////////////////////////////////////////////

  template <class T>
  struct plVariantToProperty
  {
    plVariantToProperty(const plVariant& value, const plAbstractProperty* pProp)
    {
      m_tempValue = value.ConvertTo<typename plPropertyValue<T>::StorageType>();
    }

    operator const void*()
    {
      return &m_tempValue;
    }

    typename plPropertyValue<T>::Type m_tempValue = {};
  };

  template <>
  struct plVariantToProperty<const char*>
  {
    plVariantToProperty(const plVariant& value, const plAbstractProperty* pProp)
    {
      m_sData = value.ConvertTo<plString>();
      m_pValue = m_sData;
    }

    operator const void*()
    {
      return &m_pValue;
    }
    plString m_sData;
    const char* m_pValue;
  };

  template <>
  struct plVariantToProperty<plVariant>
  {
    plVariantToProperty(const plVariant& value, const plAbstractProperty* pProp)
      : m_value(value)
    {
    }

    operator const void*()
    {
      return const_cast<plVariant*>(&m_value);
    }

    const plVariant& m_value;
  };

  template <>
  struct plVariantToProperty<plTypedPointer>
  {
    plVariantToProperty(const plVariant& value, const plAbstractProperty* pProp)
    {
      m_ptr = value.Get<plTypedPointer>();
      PL_ASSERT_DEBUG(!m_ptr.m_pType || m_ptr.m_pType->IsDerivedFrom(pProp->GetSpecificType()),
        "Pointer of type '{0}' does not derive from '{}'", m_ptr.m_pType->GetTypeName(), pProp->GetSpecificType()->GetTypeName());
    }

    operator const void*()
    {
      return &m_ptr.m_pObject;
    }

    plTypedPointer m_ptr;
  };


  template <>
  struct plVariantToProperty<plTypedObject>
  {
    plVariantToProperty(const plVariant& value, const plAbstractProperty* pProp)
    {
      m_pPtr = value.GetData();
    }

    operator const void*()
    {
      return m_pPtr;
    }
    const void* m_pPtr = nullptr;
  };

  //////////////////////////////////////////////////////////////////////////

  struct GetValueFunc
  {
    template <typename T>
    PL_ALWAYS_INLINE void operator()(const plAbstractMemberProperty* pProp, const void* pObject, plVariant& value)
    {
      plVariantFromProperty<T> getter(value, pProp);
      pProp->GetValuePtr(pObject, getter);
    }
  };

  struct SetValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractMemberProperty* pProp, void* pObject, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->SetValuePtr(pObject, setter);
    }
  };

  struct GetArrayValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractArrayProperty* pProp, const void* pObject, plUInt32 uiIndex, plVariant& value)
    {
      plVariantFromProperty<T> getter(value, pProp);
      pProp->GetValue(pObject, uiIndex, getter);
    }
  };

  struct SetArrayValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->SetValue(pObject, uiIndex, setter);
    }
  };

  struct InsertArrayValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, uiIndex, setter);
    }
  };

  struct InsertSetValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, setter);
    }
  };

  struct RemoveSetValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->Remove(pObject, setter);
    }
  };

  struct GetMapValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractMapProperty* pProp, const void* pObject, const char* szKey, plVariant& value)
    {
      plVariantFromProperty<T> getter(value, pProp);
      getter.m_bSuccess = pProp->GetValue(pObject, szKey, getter);
    }
  };

  struct SetMapValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()(const plAbstractMapProperty* pProp, void* pObject, const char* szKey, const plVariant& value)
    {
      plVariantToProperty<T> setter(value, pProp);
      pProp->Insert(pObject, szKey, setter);
    }
  };

  static bool CompareProperties(const void* pObject, const void* pObject2, const plRTTI* pType)
  {
    if (pType->GetParentType())
    {
      if (!CompareProperties(pObject, pObject2, pType->GetParentType()))
        return false;
    }

    for (auto* pProp : pType->GetProperties())
    {
      if (!plReflectionUtils::IsEqual(pObject, pObject2, pProp))
        return false;
    }

    return true;
  }

  template <typename T>
  struct SetComponentValueImpl
  {
    PL_FORCE_INLINE static void impl(plVariant* pVector, plUInt32 uiComponent, double fValue) { PL_ASSERT_DEBUG(false, "plReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType()); }
  };

  template <typename T>
  struct SetComponentValueImpl<plVec2Template<T>>
  {
    PL_FORCE_INLINE static void impl(plVariant* pVector, plUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<plVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<plVec3Template<T>>
  {
    PL_FORCE_INLINE static void impl(plVariant* pVector, plUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<plVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  template <typename T>
  struct SetComponentValueImpl<plVec4Template<T>>
  {
    PL_FORCE_INLINE static void impl(plVariant* pVector, plUInt32 uiComponent, double fValue)
    {
      auto vec = pVector->Get<plVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          vec.x = static_cast<T>(fValue);
          break;
        case 1:
          vec.y = static_cast<T>(fValue);
          break;
        case 2:
          vec.z = static_cast<T>(fValue);
          break;
        case 3:
          vec.w = static_cast<T>(fValue);
          break;
      }
      *pVector = vec;
    }
  };

  struct SetComponentValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()()
    {
      SetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    plVariant* m_pVector;
    plUInt32 m_iComponent;
    double m_fValue;
  };

  template <typename T>
  struct GetComponentValueImpl
  {
    PL_FORCE_INLINE static void impl(const plVariant* pVector, plUInt32 uiComponent, double& ref_fValue) { PL_ASSERT_DEBUG(false, "plReflectionUtils::SetComponent was called with a non-vector variant '{0}'", pVector->GetType()); }
  };

  template <typename T>
  struct GetComponentValueImpl<plVec2Template<T>>
  {
    PL_FORCE_INLINE static void impl(const plVariant* pVector, plUInt32 uiComponent, double& ref_fValue)
    {
      const auto& vec = pVector->Get<plVec2Template<T>>();
      switch (uiComponent)
      {
        case 0:
          ref_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          ref_fValue = static_cast<double>(vec.y);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<plVec3Template<T>>
  {
    PL_FORCE_INLINE static void impl(const plVariant* pVector, plUInt32 uiComponent, double& ref_fValue)
    {
      const auto& vec = pVector->Get<plVec3Template<T>>();
      switch (uiComponent)
      {
        case 0:
          ref_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          ref_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          ref_fValue = static_cast<double>(vec.z);
          break;
      }
    }
  };

  template <typename T>
  struct GetComponentValueImpl<plVec4Template<T>>
  {
    PL_FORCE_INLINE static void impl(const plVariant* pVector, plUInt32 uiComponent, double& ref_fValue)
    {
      const auto& vec = pVector->Get<plVec4Template<T>>();
      switch (uiComponent)
      {
        case 0:
          ref_fValue = static_cast<double>(vec.x);
          break;
        case 1:
          ref_fValue = static_cast<double>(vec.y);
          break;
        case 2:
          ref_fValue = static_cast<double>(vec.z);
          break;
        case 3:
          ref_fValue = static_cast<double>(vec.w);
          break;
      }
    }
  };

  struct GetComponentValueFunc
  {
    template <typename T>
    PL_FORCE_INLINE void operator()()
    {
      GetComponentValueImpl<T>::impl(m_pVector, m_iComponent, m_fValue);
    }
    const plVariant* m_pVector;
    plUInt32 m_iComponent;
    double m_fValue;
  };
} // namespace

const plRTTI* plReflectionUtils::GetCommonBaseType(const plRTTI* pRtti1, const plRTTI* pRtti2)
{
  if (pRtti2 == nullptr)
    return nullptr;

  while (pRtti1 != nullptr)
  {
    const plRTTI* pRtti2Parent = pRtti2;

    while (pRtti2Parent != nullptr)
    {
      if (pRtti1 == pRtti2Parent)
        return pRtti2Parent;

      pRtti2Parent = pRtti2Parent->GetParentType();
    }

    pRtti1 = pRtti1->GetParentType();
  }

  return nullptr;
}

bool plReflectionUtils::IsBasicType(const plRTTI* pRtti)
{
  PL_ASSERT_DEBUG(pRtti != nullptr, "IsBasicType: missing data!");
  plVariant::Type::Enum type = pRtti->GetVariantType();
  return type >= plVariant::Type::FirstStandardType && type <= plVariant::Type::LastStandardType;
}

bool plReflectionUtils::IsValueType(const plAbstractProperty* pProp)
{
  return !pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && (pProp->GetFlags().IsSet(plPropertyFlags::StandardType) || plVariantTypeRegistry::GetSingleton()->FindVariantTypeInfo(pProp->GetSpecificType()));
}

const plRTTI* plReflectionUtils::GetTypeFromVariant(const plVariant& value)
{
  return value.GetReflectedType();
}

const plRTTI* plReflectionUtils::GetTypeFromVariant(plVariantType::Enum type)
{
  GetTypeFromVariantTypeFunc func;
  func.m_pType = nullptr;
  plVariant::DispatchTo(func, type);

  return func.m_pType;
}

plUInt32 plReflectionUtils::GetComponentCount(plVariantType::Enum type)
{
  switch (type)
  {
    case plVariant::Type::Vector2:
    case plVariant::Type::Vector2I:
    case plVariant::Type::Vector2U:
      return 2;
    case plVariant::Type::Vector3:
    case plVariant::Type::Vector3I:
    case plVariant::Type::Vector3U:
      return 3;
    case plVariant::Type::Vector4:
    case plVariant::Type::Vector4I:
    case plVariant::Type::Vector4U:
      return 4;
    default:
      PL_REPORT_FAILURE("Not a vector type: '{0}'", type);
      return 0;
  }
}

void plReflectionUtils::SetComponent(plVariant& ref_vector, plUInt32 uiComponent, double fValue)
{
  SetComponentValueFunc func;
  func.m_pVector = &ref_vector;
  func.m_iComponent = uiComponent;
  func.m_fValue = fValue;
  plVariant::DispatchTo(func, ref_vector.GetType());
}

double plReflectionUtils::GetComponent(const plVariant& vector, plUInt32 uiComponent)
{
  GetComponentValueFunc func;
  func.m_pVector = &vector;
  func.m_iComponent = uiComponent;
  plVariant::DispatchTo(func, vector.GetType());
  return func.m_fValue;
}

plVariant plReflectionUtils::GetMemberPropertyValue(const plAbstractMemberProperty* pProp, const void* pObject)
{
  plVariant res;
  PL_ASSERT_DEBUG(pProp != nullptr, "GetMemberPropertyValue: missing data!");

  GetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, res);

  return res;
}

void plReflectionUtils::SetMemberPropertyValue(const plAbstractMemberProperty* pProp, void* pObject, const plVariant& value)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMemberPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  if (pProp->GetFlags().IsAnySet(plPropertyFlags::Bitflags | plPropertyFlags::IsEnum))
  {
    auto pEnumerationProp = static_cast<const plAbstractEnumerationProperty*>(pProp);

    // Value can either be an integer or a string (human readable value)
    if (value.IsA<plString>())
    {
      plInt64 iValue;
      plReflectionUtils::StringToEnumeration(pProp->GetSpecificType(), value.Get<plString>(), iValue);
      pEnumerationProp->SetValue(pObject, iValue);
    }
    else
    {
      pEnumerationProp->SetValue(pObject, value.ConvertTo<plInt64>());
    }
  }
  else
  {
    SetValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, value);
  }
}

plVariant plReflectionUtils::GetArrayPropertyValue(const plAbstractArrayProperty* pProp, const void* pObject, plUInt32 uiIndex)
{
  plVariant res;
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    plLog::Error("GetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    GetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, res);
  }
  return res;
}

void plReflectionUtils::SetArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex, const plVariant& value)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    plLog::Error("SetArrayPropertyValue: Invalid index: {0}", uiIndex);
  }
  else
  {
    SetArrayValueFunc func;
    DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
  }
}

void plReflectionUtils::InsertSetPropertyValue(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  InsertSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

void plReflectionUtils::RemoveSetPropertyValue(const plAbstractSetProperty* pProp, void* pObject, const plVariant& value)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveSetPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  RemoveSetValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, value);
}

plVariant plReflectionUtils::GetMapPropertyValue(const plAbstractMapProperty* pProp, const void* pObject, const char* szKey)
{
  plVariant value;
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "GetMapPropertyValue: missing data!");

  GetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
  return value;
}

void plReflectionUtils::SetMapPropertyValue(const plAbstractMapProperty* pProp, void* pObject, const char* szKey, const plVariant& value)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "SetMapPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  SetMapValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, szKey, value);
}

void plReflectionUtils::InsertArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, const plVariant& value, plUInt32 uiIndex)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "InsertArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex > uiCount)
  {
    plLog::Error("InsertArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  InsertArrayValueFunc func;
  DispatchTo(func, pProp, pProp, pObject, uiIndex, value);
}

void plReflectionUtils::RemoveArrayPropertyValue(const plAbstractArrayProperty* pProp, void* pObject, plUInt32 uiIndex)
{
  PL_ASSERT_DEBUG(pProp != nullptr && pObject != nullptr, "RemoveArrayPropertyValue: missing data!");
  if (pProp->GetFlags().IsSet(plPropertyFlags::ReadOnly))
    return;

  auto uiCount = pProp->GetCount(pObject);
  if (uiIndex >= uiCount)
  {
    plLog::Error("RemoveArrayPropertyValue: Invalid index: {0}", uiIndex);
    return;
  }

  pProp->Remove(pObject, uiIndex);
}

const plAbstractMemberProperty* plReflectionUtils::GetMemberProperty(const plRTTI* pRtti, plUInt32 uiPropertyIndex)
{
  if (pRtti == nullptr)
    return nullptr;

  plHybridArray<const plAbstractProperty*, 32> props;
  pRtti->GetAllProperties(props);
  if (uiPropertyIndex < props.GetCount())
  {
    const plAbstractProperty* pProp = props[uiPropertyIndex];
    if (pProp->GetCategory() == plPropertyCategory::Member)
      return static_cast<const plAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

const plAbstractMemberProperty* plReflectionUtils::GetMemberProperty(const plRTTI* pRtti, const char* szPropertyName)
{
  if (pRtti == nullptr)
    return nullptr;

  if (const plAbstractProperty* pProp = pRtti->FindPropertyByName(szPropertyName))
  {
    if (pProp->GetCategory() == plPropertyCategory::Member)
      return static_cast<const plAbstractMemberProperty*>(pProp);
  }

  return nullptr;
}

void plReflectionUtils::GatherTypesDerivedFromClass(const plRTTI* pBaseRtti, plSet<const plRTTI*>& out_types)
{
  plRTTI::ForEachDerivedType(pBaseRtti,
    [&](const plRTTI* pRtti) {
      out_types.Insert(pRtti);
    });
}

void plReflectionUtils::GatherDependentTypes(const plRTTI* pRtti, plSet<const plRTTI*>& inout_typesAsSet, plDynamicArray<const plRTTI*>* out_pTypesAsStack /*= nullptr*/)
{
  auto AddType = [&](const plRTTI* pNewRtti) {
    if (pNewRtti != pRtti && pNewRtti->GetTypeFlags().IsSet(plTypeFlags::StandardType) == false && inout_typesAsSet.Contains(pNewRtti) == false)
    {
      inout_typesAsSet.Insert(pNewRtti);
      if (out_pTypesAsStack != nullptr)
      {
        out_pTypesAsStack->PushBack(pNewRtti);
      }

      GatherDependentTypes(pNewRtti, inout_typesAsSet, out_pTypesAsStack);
    }
  };

  if (const plRTTI* pParentRtti = pRtti->GetParentType())
  {
    AddType(pParentRtti);
  }

  for (const plAbstractProperty* prop : pRtti->GetProperties())
  {
    if (prop->GetCategory() == plPropertyCategory::Constant)
      continue;

    if (prop->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;

    AddType(prop->GetSpecificType());
  }

  for (const plAbstractFunctionProperty* func : pRtti->GetFunctions())
  {
    plUInt32 uiNumArgs = func->GetArgumentCount();
    for (plUInt32 i = 0; i < uiNumArgs; ++i)
    {
      AddType(func->GetArgumentType(i));
    }
  }

  for (const plPropertyAttribute* attr : pRtti->GetAttributes())
  {
    AddType(attr->GetDynamicRTTI());
  }
}

plResult plReflectionUtils::CreateDependencySortedTypeArray(const plSet<const plRTTI*>& types, plDynamicArray<const plRTTI*>& out_sortedTypes)
{
  out_sortedTypes.Clear();
  out_sortedTypes.Reserve(types.GetCount());

  plSet<const plRTTI*> accu;
  plDynamicArray<const plRTTI*> tmpStack;

  for (const plRTTI* pType : types)
  {
    if (accu.Contains(pType))
      continue;

    GatherDependentTypes(pType, accu, &tmpStack);

    while (tmpStack.IsEmpty() == false)
    {
      const plRTTI* pDependentType = tmpStack.PeekBack();
      PL_ASSERT_DEBUG(pDependentType != pType, "A type must not be reported as dependency of itself");
      tmpStack.PopBack();

      if (types.Contains(pDependentType) == false)
        return PL_FAILURE;

      out_sortedTypes.PushBack(pDependentType);
    }

    accu.Insert(pType);
    out_sortedTypes.PushBack(pType);
  }

  PL_ASSERT_DEV(types.GetCount() == out_sortedTypes.GetCount(), "Not all types have been sorted or the sorted list contains duplicates");
  return PL_SUCCESS;
}

bool plReflectionUtils::EnumerationToString(const plRTTI* pEnumerationRtti, plInt64 iValue, plStringBuilder& out_sOutput, plEnum<EnumConversionMode> conversionMode)
{
  out_sOutput.Clear();
  if (pEnumerationRtti->IsDerivedFrom<plEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        plVariant value = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant();
        if (value.ConvertTo<plInt64>() == iValue)
        {
          out_sOutput = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : plStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<plBitflagsBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        plVariant value = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant();
        if ((value.ConvertTo<plInt64>() & iValue) != 0)
        {
          out_sOutput.Append(conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : plStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2, "|");
        }
      }
    }
    out_sOutput.Shrink(0, 1);
    return true;
  }
  else
  {
    PL_ASSERT_DEV(false, "The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

void plReflectionUtils::GetEnumKeysAndValues(const plRTTI* pEnumerationRtti, plDynamicArray<EnumKeyValuePair>& ref_entries, plEnum<EnumConversionMode> conversionMode)
{
  /// \test This is new.

  ref_entries.Clear();

  if (pEnumerationRtti->IsDerivedFrom<plEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        plVariant value = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant();

        auto& e = ref_entries.ExpandAndGetRef();
        e.m_sKey = conversionMode == EnumConversionMode::FullyQualifiedName ? pProp->GetPropertyName() : plStringUtils::FindLastSubString(pProp->GetPropertyName(), "::") + 2;
        e.m_iValue = value.ConvertTo<plInt32>();
      }
    }
  }
}

bool plReflectionUtils::StringToEnumeration(const plRTTI* pEnumerationRtti, const char* szValue, plInt64& out_iValue)
{
  out_iValue = 0;
  if (pEnumerationRtti->IsDerivedFrom<plEnumBase>())
  {
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        // Testing fully qualified and short value name
        const char* valueNameOnly = plStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
        if (plStringUtils::IsEqual(pProp->GetPropertyName(), szValue) || (valueNameOnly != nullptr && plStringUtils::IsEqual(valueNameOnly + 2, szValue)))
        {
          plVariant value = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant();
          out_iValue = value.ConvertTo<plInt64>();
          return true;
        }
      }
    }
    return false;
  }
  else if (pEnumerationRtti->IsDerivedFrom<plBitflagsBase>())
  {
    plStringBuilder temp = szValue;
    plHybridArray<plStringView, 32> values;
    temp.Split(false, values, "|");
    for (auto sValue : values)
    {
      for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
      {
        if (pProp->GetCategory() == plPropertyCategory::Constant)
        {
          // Testing fully qualified and short value name
          const char* valueNameOnly = plStringUtils::FindLastSubString(pProp->GetPropertyName(), "::", nullptr);
          if (sValue.IsEqual(pProp->GetPropertyName()) || (valueNameOnly != nullptr && sValue.IsEqual(valueNameOnly + 2)))
          {
            plVariant value = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant();
            out_iValue |= value.ConvertTo<plInt64>();
          }
        }
      }
    }
    return true;
  }
  else
  {
    PL_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return false;
  }
}

plInt64 plReflectionUtils::DefaultEnumerationValue(const plRTTI* pEnumerationRtti)
{
  if (pEnumerationRtti->IsDerivedFrom<plEnumBase>() || pEnumerationRtti->IsDerivedFrom<plBitflagsBase>())
  {
    auto pProp = pEnumerationRtti->GetProperties()[0];
    PL_ASSERT_DEBUG(pProp->GetCategory() == plPropertyCategory::Constant && plStringUtils::EndsWith(pProp->GetPropertyName(), "::Default"), "First enumeration property must be the default value constant.");
    return static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<plInt64>();
  }
  else
  {
    PL_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

plInt64 plReflectionUtils::MakeEnumerationValid(const plRTTI* pEnumerationRtti, plInt64 iValue)
{
  if (pEnumerationRtti->IsDerivedFrom<plEnumBase>())
  {
    // Find current value
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        plInt64 iCurrentValue = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<plInt64>();
        if (iCurrentValue == iValue)
          return iValue;
      }
    }

    // Current value not found, return default value
    return plReflectionUtils::DefaultEnumerationValue(pEnumerationRtti);
  }
  else if (pEnumerationRtti->IsDerivedFrom<plBitflagsBase>())
  {
    plInt64 iNewValue = 0;
    // Filter valid bits
    for (auto pProp : pEnumerationRtti->GetProperties().GetSubArray(1))
    {
      if (pProp->GetCategory() == plPropertyCategory::Constant)
      {
        plInt64 iCurrentValue = static_cast<const plAbstractConstantProperty*>(pProp)->GetConstant().ConvertTo<plInt64>();
        if ((iCurrentValue & iValue) != 0)
        {
          iNewValue |= iCurrentValue;
        }
      }
    }
    return iNewValue;
  }
  else
  {
    PL_REPORT_FAILURE("The RTTI class '{0}' is not an enum or bitflags class", pEnumerationRtti->GetTypeName());
    return 0;
  }
}

bool plReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const plAbstractProperty* pProp)
{
  // #VAR TEST
  const plRTTI* pPropType = pProp->GetSpecificType();

  plVariant vTemp;
  plVariant vTemp2;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProp);

  switch (pProp->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      auto pSpecific = static_cast<const plAbstractMemberProperty*>(pProp);

      if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
        vTemp2 = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
        void* pRefrencedObject = vTemp.ConvertTo<void*>();
        void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
        if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
          return false;
        if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
          return true;

        if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
        {
          return IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
        }
        else
        {
          return pRefrencedObject == pRefrencedObject2;
        }
      }
      else
      {
        if (pProp->GetFlags().IsAnySet(plPropertyFlags::IsEnum | plPropertyFlags::Bitflags) || bIsValueType)
        {
          vTemp = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject);
          vTemp2 = plReflectionUtils::GetMemberPropertyValue(pSpecific, pObject2);
          return vTemp == vTemp2;
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class))
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pObject);
          void* pSubObject2 = pSpecific->GetPropertyPointer(pObject2);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return IsEqual(pSubObject, pSubObject2, pPropType);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            pSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSubObject2 = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pObject, pSubObject);
            pSpecific->GetValuePtr(pObject2, pSubObject2);
            bool bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            pPropType->GetAllocator()->Deallocate(pSubObject);
            pPropType->GetAllocator()->Deallocate(pSubObject2);
            return bEqual;
          }
          else
          {
            // TODO: return false if prop can't be compared?
            return true;
          }
        }
      }
    }
    break;
    case plPropertyCategory::Array:
    {
      auto pSpecific = static_cast<const plAbstractArrayProperty*>(pProp);

      const plUInt32 uiCount = pSpecific->GetCount(pObject);
      const plUInt32 uiCount2 = pSpecific->GetCount(pObject2);
      if (uiCount != uiCount2)
        return false;

      if (pSpecific->GetFlags().IsSet(plPropertyFlags::Pointer))
      {
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          vTemp = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
          vTemp2 = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
          void* pRefrencedObject = vTemp.ConvertTo<void*>();
          void* pRefrencedObject2 = vTemp2.ConvertTo<void*>();
          if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
            return false;
          if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
            continue;

          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            if (!IsEqual(pRefrencedObject, pRefrencedObject2, pPropType))
              return false;
          }
          else
          {
            if (pRefrencedObject != pRefrencedObject2)
              return false;
          }
        }
        return true;
      }
      else
      {
        if (bIsValueType)
        {
          for (plUInt32 i = 0; i < uiCount; ++i)
          {
            vTemp = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject, i);
            vTemp2 = plReflectionUtils::GetArrayPropertyValue(pSpecific, pObject2, i);
            if (vTemp != vTemp2)
              return false;
          }
          return true;
        }
        else if (pProp->GetFlags().IsSet(plPropertyFlags::Class) && pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          void* pSubObject2 = pPropType->GetAllocator()->Allocate<void>();

          bool bEqual = true;
          for (plUInt32 i = 0; i < uiCount; ++i)
          {
            pSpecific->GetValue(pObject, i, pSubObject);
            pSpecific->GetValue(pObject2, i, pSubObject2);
            bEqual = IsEqual(pSubObject, pSubObject2, pPropType);
            if (!bEqual)
              break;
          }

          pPropType->GetAllocator()->Deallocate(pSubObject);
          pPropType->GetAllocator()->Deallocate(pSubObject2);
          return bEqual;
        }
      }
    }
    break;
    case plPropertyCategory::Set:
    {
      auto pSpecific = static_cast<const plAbstractSetProperty*>(pProp);

      plHybridArray<plVariant, 16> values;
      pSpecific->GetValues(pObject, values);
      plHybridArray<plVariant, 16> values2;
      pSpecific->GetValues(pObject2, values2);

      const plUInt32 uiCount = values.GetCount();
      const plUInt32 uiCount2 = values2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = values2.Contains(values[i]);
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if (pProp->GetFlags().AreAllSet(plPropertyFlags::Pointer | plPropertyFlags::PointerOwner))
      {
        // TODO: pointer sets are never stable unless they use an array based pseudo set as storage.
        bool bEqual = true;
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          if (pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner))
          {
            void* pRefrencedObject = values[i].ConvertTo<void*>();
            void* pRefrencedObject2 = values2[i].ConvertTo<void*>();
            if ((pRefrencedObject == nullptr) != (pRefrencedObject2 == nullptr))
              return false;
            if ((pRefrencedObject == nullptr) && (pRefrencedObject2 == nullptr))
              continue;

            bEqual = IsEqual(pRefrencedObject, pRefrencedObject2, pPropType);
          }
          if (!bEqual)
            break;
        }

        return bEqual;
      }
    }
    break;
    case plPropertyCategory::Map:
    {
      auto pSpecific = static_cast<const plAbstractMapProperty*>(pProp);

      plHybridArray<plString, 16> keys;
      pSpecific->GetKeys(pObject, keys);
      plHybridArray<plString, 16> keys2;
      pSpecific->GetKeys(pObject2, keys2);

      const plUInt32 uiCount = keys.GetCount();
      const plUInt32 uiCount2 = keys2.GetCount();
      if (uiCount != uiCount2)
        return false;

      if (bIsValueType || (pProp->GetFlags().IsSet(plPropertyFlags::Pointer) && !pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)))
      {
        bool bEqual = true;
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;
          plVariant value1 = GetMapPropertyValue(pSpecific, pObject, keys[i]);
          plVariant value2 = GetMapPropertyValue(pSpecific, pObject2, keys[i]);
          bEqual = value1 == value2;
          if (!bEqual)
            break;
        }
        return bEqual;
      }
      else if ((!pProp->GetFlags().IsSet(plPropertyFlags::Pointer) || pProp->GetFlags().IsSet(plPropertyFlags::PointerOwner)) && pProp->GetFlags().IsSet(plPropertyFlags::Class))
      {
        bool bEqual = true;
        for (plUInt32 i = 0; i < uiCount; ++i)
        {
          bEqual = keys2.Contains(keys[i]);
          if (!bEqual)
            break;

          if (pProp->GetFlags().IsSet(plPropertyFlags::Pointer))
          {
            const void* value1 = nullptr;
            const void* value2 = nullptr;
            pSpecific->GetValue(pObject, keys[i], &value1);
            pSpecific->GetValue(pObject2, keys[i], &value2);
            if ((value1 == nullptr) != (value2 == nullptr))
              return false;
            if ((value1 == nullptr) && (value2 == nullptr))
              continue;
            bEqual = IsEqual(value1, value2, pPropType);
          }
          else
          {
            if (pPropType->GetAllocator()->CanAllocate())
            {
              void* value1 = pPropType->GetAllocator()->Allocate<void>();
              PL_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value1););
              void* value2 = pPropType->GetAllocator()->Allocate<void>();
              PL_SCOPE_EXIT(pPropType->GetAllocator()->Deallocate(value2););

              bool bRes1 = pSpecific->GetValue(pObject, keys[i], value1);
              bool bRes2 = pSpecific->GetValue(pObject2, keys[i], value2);
              if (bRes1 != bRes2)
                return false;
              if (!bRes1 && !bRes2)
                continue;
              bEqual = IsEqual(value1, value2, pPropType);
            }
            else
            {
              plLog::Error("The property '{0}' can not be compared as the type '{1}' cannot be allocated.", pProp->GetPropertyName(), pPropType->GetTypeName());
            }
          }
          if (!bEqual)
            break;
        }
        return bEqual;
      }
    }
    break;

    default:
      PL_ASSERT_NOT_IMPLEMENTED;
      break;
  }
  return true;
}

bool plReflectionUtils::IsEqual(const void* pObject, const void* pObject2, const plRTTI* pType)
{
  PL_ASSERT_DEV(pObject && pObject2 && pType, "invalid type.");
  if (pType->IsDerivedFrom<plReflectedClass>())
  {
    const plReflectedClass* pRefObject = static_cast<const plReflectedClass*>(pObject);
    const plReflectedClass* pRefObject2 = static_cast<const plReflectedClass*>(pObject2);
    pType = pRefObject->GetDynamicRTTI();
    if (pType != pRefObject2->GetDynamicRTTI())
      return false;
  }

  return CompareProperties(pObject, pObject2, pType);
}


void plReflectionUtils::DeleteObject(void* pObject, const plAbstractProperty* pOwnerProperty)
{
  if (!pObject)
    return;

  const plRTTI* pType = pOwnerProperty->GetSpecificType();
  if (pType->IsDerivedFrom<plReflectedClass>())
  {
    plReflectedClass* pRefObject = static_cast<plReflectedClass*>(pObject);
    pType = pRefObject->GetDynamicRTTI();
  }

  if (!pType->GetAllocator()->CanAllocate())
  {
    plLog::Error("Tried to deallocate object of type '{0}', but it has no allocator.", pType->GetTypeName());
    return;
  }
  pType->GetAllocator()->Deallocate(pObject);
}

plVariant plReflectionUtils::GetDefaultVariantFromType(plVariant::Type::Enum type)
{
  switch (type)
  {
    case plVariant::Type::Invalid:
      return plVariant();
    case plVariant::Type::Bool:
      return plVariant(false);
    case plVariant::Type::Int8:
      return plVariant((plInt8)0);
    case plVariant::Type::UInt8:
      return plVariant((plUInt8)0);
    case plVariant::Type::Int16:
      return plVariant((plInt16)0);
    case plVariant::Type::UInt16:
      return plVariant((plUInt16)0);
    case plVariant::Type::Int32:
      return plVariant((plInt32)0);
    case plVariant::Type::UInt32:
      return plVariant((plUInt32)0);
    case plVariant::Type::Int64:
      return plVariant((plInt64)0);
    case plVariant::Type::UInt64:
      return plVariant((plUInt64)0);
    case plVariant::Type::Float:
      return plVariant(0.0f);
    case plVariant::Type::Double:
      return plVariant(0.0);
    case plVariant::Type::Color:
      return plVariant(plColor(1.0f, 1.0f, 1.0f));
    case plVariant::Type::ColorGamma:
      return plVariant(plColorGammaUB(255, 255, 255));
    case plVariant::Type::Vector2:
      return plVariant(plVec2(0.0f, 0.0f));
    case plVariant::Type::Vector3:
      return plVariant(plVec3(0.0f, 0.0f, 0.0f));
    case plVariant::Type::Vector4:
      return plVariant(plVec4(0.0f, 0.0f, 0.0f, 0.0f));
    case plVariant::Type::Vector2I:
      return plVariant(plVec2I32(0, 0));
    case plVariant::Type::Vector3I:
      return plVariant(plVec3I32(0, 0, 0));
    case plVariant::Type::Vector4I:
      return plVariant(plVec4I32(0, 0, 0, 0));
    case plVariant::Type::Vector2U:
      return plVariant(plVec2U32(0, 0));
    case plVariant::Type::Vector3U:
      return plVariant(plVec3U32(0, 0, 0));
    case plVariant::Type::Vector4U:
      return plVariant(plVec4U32(0, 0, 0, 0));
    case plVariant::Type::Quaternion:
      return plVariant(plQuat(0.0f, 0.0f, 0.0f, 1.0f));
    case plVariant::Type::Matrix3:
      return plVariant(plMat3::MakeIdentity());
    case plVariant::Type::Matrix4:
      return plVariant(plMat4::MakeIdentity());
    case plVariant::Type::Transform:
      return plVariant(plTransform::MakeIdentity());
    case plVariant::Type::String:
      return plVariant(plString());
    case plVariant::Type::StringView:
      return plVariant(plStringView(), false);
    case plVariant::Type::DataBuffer:
      return plVariant(plDataBuffer());
    case plVariant::Type::Time:
      return plVariant(plTime());
    case plVariant::Type::Uuid:
      return plVariant(plUuid());
    case plVariant::Type::Angle:
      return plVariant(plAngle());
    case plVariant::Type::HashedString:
      return plVariant(plHashedString());
    case plVariant::Type::TempHashedString:
      return plVariant(plTempHashedString());
    case plVariant::Type::VariantArray:
      return plVariantArray();
    case plVariant::Type::VariantDictionary:
      return plVariantDictionary();
    case plVariant::Type::TypedPointer:
      return plVariant(static_cast<void*>(nullptr), nullptr);

    default:
      PL_REPORT_FAILURE("Invalid case statement");
      return plVariant();
  }
}

plVariant plReflectionUtils::GetDefaultValue(const plAbstractProperty* pProperty, plVariant index)
{
  const bool isValueType = plReflectionUtils::IsValueType(pProperty);
  const plVariantType::Enum type = pProperty->GetFlags().IsSet(plPropertyFlags::Pointer) || (pProperty->GetFlags().IsSet(plPropertyFlags::Class) && !isValueType) ? plVariantType::Uuid : pProperty->GetSpecificType()->GetVariantType();
  const plDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<plDefaultValueAttribute>();

  switch (pProperty->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pProperty->GetSpecificType() == plGetStaticRTTI<plVariant>())
            return pAttrib->GetValue();
          if (pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else if (pProperty->GetSpecificType()->GetTypeFlags().IsAnySet(plTypeFlags::IsEnum | plTypeFlags::Bitflags))
      {
        plInt64 iValue = plReflectionUtils::DefaultEnumerationValue(pProperty->GetSpecificType());
        if (pAttrib)
        {
          if (pAttrib->GetValue().CanConvertTo(plVariantType::Int64))
            iValue = pAttrib->GetValue().ConvertTo<plInt64>();
        }
        return plReflectionUtils::MakeEnumerationValid(pProperty->GetSpecificType(), iValue);
      }
      else // Class
      {
        return plUuid();
      }
    }
    break;
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<plVariantArray>())
          {
            if (!index.IsValid())
              return pAttrib->GetValue();

            plUInt32 iIndex = index.ConvertTo<plUInt32>();
            const auto& defaultArray = pAttrib->GetValue().Get<plVariantArray>();
            if (iIndex < defaultArray.GetCount())
            {
              return defaultArray[iIndex];
            }
            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return plVariantArray();

        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return plVariantArray();

        return plUuid();
      }
      break;
    case plPropertyCategory::Map:
      if (isValueType)
      {
        if (pAttrib)
        {
          if (pAttrib->GetValue().IsA<plVariantDictionary>())
          {
            if (!index.IsValid())
            {
              return pAttrib->GetValue();
            }
            plString sKey = index.ConvertTo<plString>();
            const auto& defaultDict = pAttrib->GetValue().Get<plVariantDictionary>();
            if (auto it = defaultDict.Find(sKey); it.IsValid())
              return it.Value();

            return GetDefaultVariantFromType(pProperty->GetSpecificType());
          }
          if (index.IsValid() && pAttrib->GetValue().CanConvertTo(type))
            return pAttrib->GetValue().ConvertTo(type);
        }

        if (!index.IsValid())
          return plVariantDictionary();
        return GetDefaultVariantFromType(pProperty->GetSpecificType());
      }
      else
      {
        if (!index.IsValid())
          return plVariantDictionary();

        return plUuid();
      }
      break;
    default:
      break;
  }

  PL_REPORT_FAILURE("Don't reach here");
  return plVariant();
}

plVariant plReflectionUtils::GetDefaultVariantFromType(const plRTTI* pRtti)
{
  plVariantType::Enum type = pRtti->GetVariantType();
  switch (type)
  {
    case plVariant::Type::TypedObject:
    {
      plVariant val;
      val.MoveTypedObject(pRtti->GetAllocator()->Allocate<void>(), pRtti);
      return val;
    }
    break;

    default:
      return GetDefaultVariantFromType(type);
  }
}

void plReflectionUtils::SetAllMemberPropertiesToDefault(const plRTTI* pRtti, void* pObject)
{
  plHybridArray<const plAbstractProperty*, 32> properties;
  pRtti->GetAllProperties(properties);

  for (auto pProp : properties)
  {
    if (pProp->GetCategory() == plPropertyCategory::Member)
    {
      const plVariant defValue = plReflectionUtils::GetDefaultValue(pProp);

      plReflectionUtils::SetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pObject, defValue);
    }
  }
}

namespace
{
  template <class C>
  struct plClampCategoryType
  {
    enum
    {
      value = (((plVariant::TypeDeduction<C>::value >= plVariantType::Int8 && plVariant::TypeDeduction<C>::value <= plVariantType::Double) || (plVariant::TypeDeduction<C>::value == plVariantType::Time) || (plVariant::TypeDeduction<C>::value == plVariantType::Angle))) + ((plVariant::TypeDeduction<C>::value >= plVariantType::Vector2 && plVariant::TypeDeduction<C>::value <= plVariantType::Vector4U) * 2)
    };
  };

  template <typename T, int V = plClampCategoryType<T>::value>
  struct ClampVariantFuncImpl
  {
    static PL_ALWAYS_INLINE plResult Func(plVariant& value, const plClampValueAttribute* pAttrib)
    {
      return PL_FAILURE;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 1> // scalar types
  {
    static PL_ALWAYS_INLINE plResult Func(plVariant& value, const plClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = plMath::Max(value.ConvertTo<T>(), pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = plMath::Min(value.ConvertTo<T>(), pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return PL_SUCCESS;
    }
  };

  template <typename T>
  struct ClampVariantFuncImpl<T, 2> // vector types
  {
    static PL_ALWAYS_INLINE plResult Func(plVariant& value, const plClampValueAttribute* pAttrib)
    {
      if (pAttrib->GetMinValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMax(pAttrib->GetMinValue().ConvertTo<T>());
      }
      if (pAttrib->GetMaxValue().CanConvertTo<T>())
      {
        value = value.ConvertTo<T>().CompMin(pAttrib->GetMaxValue().ConvertTo<T>());
      }
      return PL_SUCCESS;
    }
  };

  struct ClampVariantFunc
  {
    template <typename T>
    PL_ALWAYS_INLINE plResult operator()(plVariant& value, const plClampValueAttribute* pAttrib)
    {
      return ClampVariantFuncImpl<T>::Func(value, pAttrib);
    }
  };
} // namespace

plResult plReflectionUtils::ClampValue(plVariant& value, const plClampValueAttribute* pAttrib)
{
  plVariantType::Enum type = value.GetType();
  if (type == plVariantType::Invalid || pAttrib == nullptr)
    return PL_SUCCESS; // If there is nothing to clamp or no clamp attribute we call it a success.

  ClampVariantFunc func;
  return plVariant::DispatchTo(func, type, value, pAttrib);
}


