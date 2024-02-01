#include <Foundation/FoundationPCH.h>

#include <Foundation/Reflection/PropertyPath.h>
#include <Foundation/Reflection/ReflectionUtils.h>
#include <Foundation/Types/UniquePtr.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plPropertyPathStep, plNoBase, 1, plRTTIDefaultAllocator<plPropertyPathStep>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Property", m_sProperty),
    PL_MEMBER_PROPERTY("Index", m_Index),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

plPropertyPath::plPropertyPath() = default;
plPropertyPath::~plPropertyPath() = default;

bool plPropertyPath::IsValid() const
{
  return m_bIsValid;
}

plResult plPropertyPath::InitializeFromPath(const plRTTI& rootObjectRtti, const char* szPath)
{
  m_bIsValid = false;

  const plStringBuilder sPathParts = szPath;
  plStringBuilder sIndex;
  plStringBuilder sFieldName;

  plHybridArray<plStringView, 4> parts;
  sPathParts.Split(false, parts, "/");

  // an empty path is valid as well

  m_PathSteps.Clear();
  m_PathSteps.Reserve(parts.GetCount());

  const plRTTI* pCurRtti = &rootObjectRtti;

  for (const plStringView& part : parts)
  {
    if (part.EndsWith("]"))
    {
      const char* szBracket = part.FindSubString("[");

      sIndex.SetSubString_FromTo(szBracket + 1, part.GetEndPointer() - 1);

      sFieldName.SetSubString_FromTo(part.GetStartPointer(), szBracket);
    }
    else
    {
      sFieldName = part;
      sIndex.Clear();
    }

    const plAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(sFieldName);

    if (pAbsProp == nullptr)
      return PL_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;

    if (pAbsProp->GetCategory() == plPropertyCategory::Array)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = plVariant();
      }
      else
      {
        plInt32 iIndex;
        PL_SUCCEED_OR_RETURN(plConversionUtils::StringToInt(sIndex, iIndex));
        step.m_Index = iIndex;
      }
    }
    else if (pAbsProp->GetCategory() == plPropertyCategory::Set)
    {
      if (sIndex.IsEmpty())
      {
        step.m_Index = plVariant();
      }
      else
      {
        return PL_FAILURE;
      }
    }
    else if (pAbsProp->GetCategory() == plPropertyCategory::Map)
    {
      step.m_Index = sIndex.IsEmpty() ? plVariant() : plVariant(sIndex.GetData());
    }

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return PL_SUCCESS;
}

plResult plPropertyPath::InitializeFromPath(const plRTTI* pRootObjectRtti, const plArrayPtr<const plPropertyPathStep> path)
{
  m_bIsValid = false;

  m_PathSteps.Clear();
  m_PathSteps.Reserve(path.GetCount());

  const plRTTI* pCurRtti = pRootObjectRtti;
  for (const plPropertyPathStep& pathStep : path)
  {
    const plAbstractProperty* pAbsProp = pCurRtti->FindPropertyByName(pathStep.m_sProperty);
    if (pAbsProp == nullptr)
      return PL_FAILURE;

    auto& step = m_PathSteps.ExpandAndGetRef();
    step.m_pProperty = pAbsProp;
    step.m_Index = pathStep.m_Index;

    pCurRtti = pAbsProp->GetSpecificType();
  }

  m_bIsValid = true;
  return PL_SUCCESS;
}

plResult plPropertyPath::WriteToLeafObject(void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeaf, const plRTTI& pType)> func) const
{
  PL_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(plTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), true, func);
}

plResult plPropertyPath::ReadFromLeafObject(void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeaf, const plRTTI& pType)> func) const
{
  PL_ASSERT_DEBUG(
    m_PathSteps.IsEmpty() || m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetTypeFlags().IsSet(plTypeFlags::Class),
    "To resolve the leaf object the path needs to be empty or end in a class.");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr(), false, func);
}

plResult plPropertyPath::WriteProperty(
  void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeafObject, const plRTTI& pLeafType, const plAbstractProperty* pProp, const plVariant& index)> func) const
{
  PL_ASSERT_DEBUG(!m_PathSteps.IsEmpty(), "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), true,
    [this, &func](void* pLeafObject, const plRTTI& leafType) {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

plResult plPropertyPath::ReadProperty(
  void* pRootObject, const plRTTI& type, plDelegate<void(void* pLeafObject, const plRTTI& pLeafType, const plAbstractProperty* pProp, const plVariant& index)> func) const
{
  PL_ASSERT_DEBUG(m_bIsValid, "Call InitializeFromPath before WriteToObject");
  return ResolvePath(pRootObject, &type, m_PathSteps.GetArrayPtr().GetSubArray(0, m_PathSteps.GetCount() - 1), false,
    [this, &func](void* pLeafObject, const plRTTI& leafType) {
      auto& lastStep = m_PathSteps[m_PathSteps.GetCount() - 1];
      func(pLeafObject, leafType, lastStep.m_pProperty, lastStep.m_Index);
    });
}

void plPropertyPath::SetValue(void* pRootObject, const plRTTI& type, const plVariant& value) const
{
  // PL_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    value.CanConvertTo(m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType()),
  //                "The given value does not match the type at the given path.");

  WriteProperty(pRootObject, type, [&value](void* pLeaf, const plRTTI& type, const plAbstractProperty* pProp, const plVariant& index) {
    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
        plReflectionUtils::SetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pLeaf, value);
        break;
      case plPropertyCategory::Array:
        plReflectionUtils::SetArrayPropertyValue(static_cast<const plAbstractArrayProperty*>(pProp), pLeaf, index.Get<plInt32>(), value);
        break;
      case plPropertyCategory::Map:
        plReflectionUtils::SetMapPropertyValue(static_cast<const plAbstractMapProperty*>(pProp), pLeaf, index.Get<plString>(), value);
        break;
      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }).IgnoreResult();
}

void plPropertyPath::GetValue(void* pRootObject, const plRTTI& type, plVariant& out_value) const
{
  // PL_ASSERT_DEBUG(!m_PathSteps.IsEmpty() &&
  //                    m_PathSteps[m_PathSteps.GetCount() - 1].m_pProperty->GetSpecificType()->GetVariantType() != plVariantType::Invalid,
  //                "The property path of value {} cannot be stored in an plVariant.", m_PathSteps[m_PathSteps.GetCount() -
  //                1].m_pProperty->GetSpecificType()->GetTypeName());

  ReadProperty(pRootObject, type, [&out_value](void* pLeaf, const plRTTI& type, const plAbstractProperty* pProp, const plVariant& index) {
    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
        out_value = plReflectionUtils::GetMemberPropertyValue(static_cast<const plAbstractMemberProperty*>(pProp), pLeaf);
        break;
      case plPropertyCategory::Array:
        out_value = plReflectionUtils::GetArrayPropertyValue(static_cast<const plAbstractArrayProperty*>(pProp), pLeaf, index.Get<plInt32>());
        break;
      case plPropertyCategory::Map:
        out_value = plReflectionUtils::GetMapPropertyValue(static_cast<const plAbstractMapProperty*>(pProp), pLeaf, index.Get<plString>());
        break;
      default:
        PL_ASSERT_NOT_IMPLEMENTED;
        break;
    }
  }).IgnoreResult();
}

plResult plPropertyPath::ResolvePath(void* pCurrentObject, const plRTTI* pType, const plArrayPtr<const ResolvedStep> path, bool bWriteToObject,
  const plDelegate<void(void* pLeaf, const plRTTI& pType)>& func)
{
  if (path.IsEmpty())
  {
    func(pCurrentObject, *pType);
    return PL_SUCCESS;
  }
  else // Recurse
  {
    const plAbstractProperty* pProp = path[0].m_pProperty;
    const plRTTI* pPropType = pProp->GetSpecificType();

    switch (pProp->GetCategory())
    {
      case plPropertyCategory::Member:
      {
        auto pSpecific = static_cast<const plAbstractMemberProperty*>(pProp);
        if (pPropType->GetProperties().GetCount() > 0)
        {
          void* pSubObject = pSpecific->GetPropertyPointer(pCurrentObject);
          // Do we have direct access to the property?
          if (pSubObject != nullptr)
          {
            return ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);
          }
          // If the property is behind an accessor, we need to retrieve it first.
          else if (pPropType->GetAllocator()->CanAllocate())
          {
            void* pRetrievedSubObject = pPropType->GetAllocator()->Allocate<void>();
            pSpecific->GetValuePtr(pCurrentObject, pRetrievedSubObject);

            plResult res = ResolvePath(pRetrievedSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

            if (bWriteToObject)
              pSpecific->SetValuePtr(pCurrentObject, pRetrievedSubObject);

            pPropType->GetAllocator()->Deallocate(pRetrievedSubObject);
            return res;
          }
          else
          {
            PL_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
          }
        }
      }
      break;
      case plPropertyCategory::Array:
      {
        auto pSpecific = static_cast<const plAbstractArrayProperty*>(pProp);

        if (pPropType->GetAllocator()->CanAllocate())
        {
          const plUInt32 uiIndex = path[0].m_Index.ConvertTo<plUInt32>();
          if (uiIndex >= pSpecific->GetCount(pCurrentObject))
            return PL_FAILURE;

          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();
          pSpecific->GetValue(pCurrentObject, uiIndex, pSubObject);

          plResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->SetValue(pCurrentObject, uiIndex, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          PL_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case plPropertyCategory::Map:
      {
        auto pSpecific = static_cast<const plAbstractMapProperty*>(pProp);
        const plString& sKey = path[0].m_Index.Get<plString>();
        if (!pSpecific->Contains(pCurrentObject, sKey))
          return PL_FAILURE;

        if (pPropType->GetAllocator()->CanAllocate())
        {
          void* pSubObject = pPropType->GetAllocator()->Allocate<void>();

          pSpecific->GetValue(pCurrentObject, sKey, pSubObject);

          plResult res = ResolvePath(pSubObject, pProp->GetSpecificType(), path.GetSubArray(1), bWriteToObject, func);

          if (bWriteToObject)
            pSpecific->Insert(pCurrentObject, sKey, pSubObject);

          pPropType->GetAllocator()->Deallocate(pSubObject);
          return res;
        }
        else
        {
          PL_REPORT_FAILURE("Non-allocatable property should not be part of an object chain!");
        }
      }
      break;
      case plPropertyCategory::Set:
      default:
      {
        PL_REPORT_FAILURE("Property of type Set should not be part of an object chain!");
      }
      break;
    }
    return PL_FAILURE;
  }
}



PL_STATICLINK_FILE(Foundation, Foundation_Reflection_Implementation_PropertyPath);
