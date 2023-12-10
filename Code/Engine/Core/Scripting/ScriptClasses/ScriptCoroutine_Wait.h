#pragma once

#include <Core/Scripting/ScriptCoroutine.h>

class PLASMA_CORE_DLL plScriptCoroutine_Wait : public plTypedScriptCoroutine<plScriptCoroutine_Wait, plTime>
{
public:
  void Start(plTime timeout);
  virtual Result Update(plTime deltaTimeSinceLastUpdate) override;

private:
  plTime m_TimeRemaing;
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plScriptCoroutine_Wait);
