#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

class PL_CORE_DLL plScriptCoroutine_Wait : public plTypedScriptCoroutine<plScriptCoroutine_Wait, plTime>
{
public:
  void Start(plTime timeout);
  virtual Result Update(plTime deltaTimeSinceLastUpdate) override;

private:
  plTime m_TimeRemaing;
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plScriptCoroutine_Wait);
