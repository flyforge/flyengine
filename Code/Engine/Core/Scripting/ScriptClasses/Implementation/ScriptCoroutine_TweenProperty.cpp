#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_TweenProperty.h>
#include <Core/World/World.h>
#include <Foundation/Reflection/ReflectionUtils.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutine_TweenProperty, plScriptCoroutine, 1, plRTTIDefaultAllocator<plScriptCoroutine_TweenProperty>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Start, In, "Component", In, "PropertyName", In, "TargetValue", In, "Duration", In, "Easing"),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Coroutine::TweenProperty {PropertyName}"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plScriptCoroutine_TweenProperty::Start(plComponentHandle hComponent, plStringView sPropertyName, plVariant targetValue, plTime duration, plEnum<plCurveFunction> easing)
{
  plComponent* pComponent = nullptr;
  if (plWorld::GetWorld(hComponent)->TryGetComponent(hComponent, pComponent) == false)
  {
    plLog::Error("TweenProperty: The given component was not found.");
    return;
  }

  auto pType = pComponent->GetDynamicRTTI();
  auto pProp = pType->FindPropertyByName(sPropertyName);
  if (pProp == nullptr || pProp->GetCategory() != plPropertyCategory::Member)
  {
    plLog::Error("TweenProperty: The given component of type '{}' does not have a member property named '{}'.", pType->GetTypeName(), sPropertyName);
    return;
  }

  plVariantType::Enum variantType = pProp->GetSpecificType()->GetVariantType();
  if (variantType == plVariantType::Invalid)
  {
    plLog::Error("TweenProperty: Can't tween property '{}' of type '{}'.", sPropertyName, pProp->GetSpecificType()->GetTypeName());
    return;
  }

  plResult conversionStatus = PL_SUCCESS;
  m_TargetValue = targetValue.ConvertTo(variantType, &conversionStatus);
  if (conversionStatus.Failed())
  {
    plLog::Error("TweenProperty: Can't convert given target value to '{}'.", pProp->GetSpecificType()->GetTypeName());
    return;
  }

  m_pProperty = static_cast<const plAbstractMemberProperty*>(pProp);
  m_hComponent = hComponent;
  m_SourceValue = plReflectionUtils::GetMemberPropertyValue(m_pProperty, pComponent);
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = plTime::MakeZero();
}

plScriptCoroutine::Result plScriptCoroutine_TweenProperty::Update(plTime deltaTimeSinceLastUpdate)
{
  if (m_pProperty == nullptr)
  {
    return Result::Failed();
  }

  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    plComponent* pComponent = nullptr;
    if (plWorld::GetWorld(m_hComponent)->TryGetComponent(m_hComponent, pComponent) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = plMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = plCurveFunction::GetValue(m_Easing, fCurrentX);
    plVariant currentValue = plMath::Lerp(m_SourceValue, m_TargetValue, fCurrentX);

    plReflectionUtils::SetMemberPropertyValue(m_pProperty, pComponent, currentValue);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}


PL_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptCoroutine_TweenProperty);

