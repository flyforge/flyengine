#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/Configuration/Startup.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Types/VariantTypeRegistry.h>
#include <ToolsFoundation/Object/DocumentObjectBase.h>
#include <ToolsFoundation/Reflection/IReflectedTypeAccessor.h>
#include <ToolsFoundation/Reflection/PhantomRttiManager.h>
#include <ToolsFoundation/Reflection/ReflectedType.h>
#include <ToolsFoundation/Reflection/ToolsReflectionUtils.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>

namespace
{
  struct GetDoubleFunc
  {
    GetDoubleFunc(const plVariant& value)
      : m_Value(value)
    {
    }
    template <typename T>
    void operator()()
    {
      if (m_Value.CanConvertTo<double>())
      {
        m_fValue = m_Value.ConvertTo<double>();
        m_bValid = true;
      }
    }

    const plVariant& m_Value;
    double m_fValue = 0;
    bool m_bValid = false;
  };

  template <>
  void GetDoubleFunc::operator()<plAngle>()
  {
    m_fValue = m_Value.Get<plAngle>().GetDegree();
    m_bValid = true;
  }

  template <>
  void GetDoubleFunc::operator()<plTime>()
  {
    m_fValue = m_Value.Get<plTime>().GetSeconds();
    m_bValid = true;
  }

  struct GetVariantFunc
  {
    GetVariantFunc(double fValue, plVariantType::Enum type, plVariant& out_value)
      : m_fValue(fValue)
      , m_Type(type)
      , m_Value(out_value)
    {
    }
    template <typename T>
    void operator()()
    {
      m_Value = m_fValue;
      if (m_Value.CanConvertTo(m_Type))
      {
        m_Value = m_Value.ConvertTo(m_Type);
        m_bValid = true;
      }
      else
      {
        m_Value = plVariant();
      }
    }

    double m_fValue;
    plVariantType::Enum m_Type;
    plVariant& m_Value;
    bool m_bValid = false;
  };

  template <>
  void GetVariantFunc::operator()<plAngle>()
  {
    m_Value = plAngle::MakeFromDegree((float)m_fValue);
    m_bValid = true;
  }

  template <>
  void GetVariantFunc::operator()<plTime>()
  {
    m_Value = plTime::MakeFromSeconds(m_fValue);
    m_bValid = true;
  }
} // namespace
////////////////////////////////////////////////////////////////////////
// plToolsReflectionUtils public functions
////////////////////////////////////////////////////////////////////////

plVariant plToolsReflectionUtils::GetStorageDefault(const plAbstractProperty* pProperty)
{
  const plDefaultValueAttribute* pAttrib = pProperty->GetAttributeByType<plDefaultValueAttribute>();
  auto type = pProperty->GetFlags().IsSet(plPropertyFlags::StandardType) ? pProperty->GetSpecificType()->GetVariantType() : plVariantType::Uuid;

  const bool bIsValueType = plReflectionUtils::IsValueType(pProperty);

  switch (pProperty->GetCategory())
  {
    case plPropertyCategory::Member:
    {
      return plReflectionUtils::GetDefaultValue(pProperty);
    }
    break;
    case plPropertyCategory::Array:
    case plPropertyCategory::Set:
    {
      if (bIsValueType && pAttrib && pAttrib->GetValue().IsA<plVariantArray>())
      {
        const plVariantArray& value = pAttrib->GetValue().Get<plVariantArray>();
        plVariantArray ret;
        ret.SetCount(value.GetCount());
        for (plUInt32 i = 0; i < value.GetCount(); i++)
        {
          ret[i] = value[i].ConvertTo(type);
        }
        return ret;
      }
      return plVariantArray();
    }
    break;
    case plPropertyCategory::Map:
    {
      return plVariantDictionary();
    }
    break;
    case plPropertyCategory::Constant:
    case plPropertyCategory::Function:
      break; // no defaults
  }
  return plVariant();
}

bool plToolsReflectionUtils::GetFloatFromVariant(const plVariant& val, double& out_fValue)
{
  if (val.IsValid())
  {
    GetDoubleFunc func(val);
    plVariant::DispatchTo(func, val.GetType());
    out_fValue = func.m_fValue;
    return func.m_bValid;
  }
  return false;
}


bool plToolsReflectionUtils::GetVariantFromFloat(double fValue, plVariantType::Enum type, plVariant& out_val)
{
  GetVariantFunc func(fValue, type, out_val);
  plVariant::DispatchTo(func, type);

  return func.m_bValid;
}

void plToolsReflectionUtils::GetReflectedTypeDescriptorFromRtti(const plRTTI* pRtti, plReflectedTypeDescriptor& out_desc)
{
  GetMinimalReflectedTypeDescriptorFromRtti(pRtti, out_desc);
  out_desc.m_Flags.Remove(plTypeFlags::Minimal);

  auto rttiProps = pRtti->GetProperties();
  const plUInt32 uiCount = rttiProps.GetCount();
  out_desc.m_Properties.Reserve(uiCount);
  for (plUInt32 i = 0; i < uiCount; ++i)
  {
    const plAbstractProperty* prop = rttiProps[i];

    switch (prop->GetCategory())
    {
      case plPropertyCategory::Constant:
      {
        auto constantProp = static_cast<const plAbstractConstantProperty*>(prop);
        const plRTTI* pPropRtti = constantProp->GetSpecificType();
        if (plReflectionUtils::IsBasicType(pPropRtti))
        {
          plVariant value = constantProp->GetConstant();
          PL_ASSERT_DEV(pPropRtti->GetVariantType() == value.GetType(), "Variant value type and property type should always match!");
          out_desc.m_Properties.PushBack(plReflectedPropertyDescriptor(constantProp->GetPropertyName(), value, prop->GetAttributes()));
        }
        else
        {
          PL_ASSERT_DEV(false, "Non-pod constants are not supported yet!");
        }
      }
      break;

      case plPropertyCategory::Member:
      case plPropertyCategory::Array:
      case plPropertyCategory::Set:
      case plPropertyCategory::Map:
      {
        const plRTTI* pPropRtti = prop->GetSpecificType();
        out_desc.m_Properties.PushBack(plReflectedPropertyDescriptor(prop->GetCategory(), prop->GetPropertyName(), pPropRtti->GetTypeName(), prop->GetFlags(), prop->GetAttributes()));
      }
      break;

      case plPropertyCategory::Function:
        break;

      default:
        break;
    }
  }

  auto rttiFunc = pRtti->GetFunctions();
  const plUInt32 uiFuncCount = rttiFunc.GetCount();
  out_desc.m_Functions.Reserve(uiFuncCount);

  for (plUInt32 i = 0; i < uiFuncCount; ++i)
  {
    const plAbstractFunctionProperty* prop = rttiFunc[i];
    out_desc.m_Functions.PushBack(plReflectedFunctionDescriptor(prop->GetPropertyName(), prop->GetFlags(), prop->GetFunctionType(), prop->GetAttributes()));
    plReflectedFunctionDescriptor& desc = out_desc.m_Functions.PeekBack();
    desc.m_ReturnValue = plFunctionArgumentDescriptor(prop->GetReturnType() ? prop->GetReturnType()->GetTypeName() : "", prop->GetReturnFlags());
    const plUInt32 uiArguments = prop->GetArgumentCount();
    desc.m_Arguments.Reserve(uiArguments);
    for (plUInt32 a = 0; a < uiArguments; ++a)
    {
      desc.m_Arguments.PushBack(plFunctionArgumentDescriptor(prop->GetArgumentType(a)->GetTypeName(), prop->GetArgumentFlags(a)));
    }
  }

  out_desc.m_ReferenceAttributes = pRtti->GetAttributes();
}


void plToolsReflectionUtils::GetMinimalReflectedTypeDescriptorFromRtti(const plRTTI* pRtti, plReflectedTypeDescriptor& out_desc)
{
  PL_ASSERT_DEV(pRtti != nullptr, "Type to process must not be null!");
  out_desc.m_sTypeName = pRtti->GetTypeName();
  out_desc.m_sPluginName = pRtti->GetPluginName();
  out_desc.m_Flags = pRtti->GetTypeFlags() | plTypeFlags::Minimal;
  out_desc.m_uiTypeVersion = pRtti->GetTypeVersion();
  const plRTTI* pParentRtti = pRtti->GetParentType();
  out_desc.m_sParentTypeName = pParentRtti ? pParentRtti->GetTypeName() : nullptr;

  out_desc.m_Properties.Clear();
  out_desc.m_Functions.Clear();
  out_desc.m_Attributes.Clear();
  out_desc.m_ReferenceAttributes = plArrayPtr<plPropertyAttribute* const>();
}

static void GatherObjectTypesInternal(const plDocumentObject* pObject, plSet<const plRTTI*>& inout_types)
{
  inout_types.Insert(pObject->GetTypeAccessor().GetType());
  plReflectionUtils::GatherDependentTypes(pObject->GetTypeAccessor().GetType(), inout_types);

  for (const plDocumentObject* pChild : pObject->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<plTemporaryAttribute>() != nullptr)
      continue;

    GatherObjectTypesInternal(pChild, inout_types);
  }
}

void plToolsReflectionUtils::GatherObjectTypes(const plDocumentObject* pObject, plSet<const plRTTI*>& inout_types)
{
  GatherObjectTypesInternal(pObject, inout_types);
}

bool plToolsReflectionUtils::DependencySortTypeDescriptorArray(plDynamicArray<plReflectedTypeDescriptor*>& ref_descriptors)
{
  plMap<plReflectedTypeDescriptor*, plSet<plString>> dependencies;

  plSet<plString> typesInArray;
  // Gather all types in array
  for (plReflectedTypeDescriptor* desc : ref_descriptors)
  {
    typesInArray.Insert(desc->m_sTypeName);
  }

  // Find all direct dependencies to types in the array for each type.
  for (plReflectedTypeDescriptor* desc : ref_descriptors)
  {
    auto it = dependencies.Insert(desc, plSet<plString>());

    if (typesInArray.Contains(desc->m_sParentTypeName))
    {
      it.Value().Insert(desc->m_sParentTypeName);
    }
    for (plReflectedPropertyDescriptor& propDesc : desc->m_Properties)
    {
      if (typesInArray.Contains(propDesc.m_sType))
      {
        it.Value().Insert(propDesc.m_sType);
      }
    }
  }

  plSet<plString> accu;
  plDynamicArray<plReflectedTypeDescriptor*> sorted;
  sorted.Reserve(ref_descriptors.GetCount());
  // Build new sorted types array.
  while (!ref_descriptors.IsEmpty())
  {
    bool bDeadEnd = true;
    for (plReflectedTypeDescriptor* desc : ref_descriptors)
    {
      // Are the types dependencies met?
      if (accu.ContainsSet(dependencies[desc]))
      {
        sorted.PushBack(desc);
        bDeadEnd = false;
        ref_descriptors.RemoveAndCopy(desc);
        accu.Insert(desc->m_sTypeName);
        break;
      }
    }

    if (bDeadEnd)
    {
      return false;
    }
  }

  ref_descriptors = sorted;
  return true;
}
