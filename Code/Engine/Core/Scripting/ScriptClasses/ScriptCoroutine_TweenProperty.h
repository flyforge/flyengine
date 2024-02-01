#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

class PL_CORE_DLL plScriptCoroutine_TweenProperty : public plTypedScriptCoroutine<plScriptCoroutine_TweenProperty, plComponentHandle, plStringView, plVariant, plTime, plEnum<plCurveFunction>>
{
public:
  void Start(plComponentHandle hComponent, plStringView sPropertyName, plVariant targetValue, plTime duration, plEnum<plCurveFunction> easing);
  virtual Result Update(plTime deltaTimeSinceLastUpdate) override;

private:
  const plAbstractMemberProperty* m_pProperty = nullptr;
  plComponentHandle m_hComponent;
  plVariant m_SourceValue;
  plVariant m_TargetValue;
  plEnum<plCurveFunction> m_Easing;

  plTime m_Duration;
  plTime m_TimePassed;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plScriptCoroutine_TweenProperty);
