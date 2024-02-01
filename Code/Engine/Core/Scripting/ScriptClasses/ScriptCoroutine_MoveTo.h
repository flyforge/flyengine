#pragma once

#include <Core/Scripting/ScriptCoroutine.h>
#include <Core/World/Declarations.h>
#include <Foundation/Math/CurveFunctions.h>

class PL_CORE_DLL plScriptCoroutine_MoveTo : public plTypedScriptCoroutine<plScriptCoroutine_MoveTo, plGameObjectHandle, plVec3, plTime, plEnum<plCurveFunction>>
{
public:
  void Start(plGameObjectHandle hObject, const plVec3& vTargetPos, plTime duration, plEnum<plCurveFunction> easing);
  virtual Result Update(plTime deltaTimeSinceLastUpdate) override;

private:
  plGameObjectHandle m_hObject;
  plVec3 m_vSourcePos;
  plVec3 m_vTargetPos;
  plEnum<plCurveFunction> m_Easing;

  plTime m_Duration;
  plTime m_TimePassed;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plScriptCoroutine_MoveTo);
