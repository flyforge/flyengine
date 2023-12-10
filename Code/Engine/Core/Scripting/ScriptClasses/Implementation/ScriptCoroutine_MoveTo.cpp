#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptClasses/ScriptCoroutine_MoveTo.h>
#include <Core/World/World.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptCoroutine_MoveTo, plScriptCoroutine, 1, plRTTIDefaultAllocator<plScriptCoroutine_MoveTo>)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Start, In, "Object", In, "TargetPos", In, "Duration", In, "Easing"),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTitleAttribute("Coroutine::MoveTo {TargetPos}"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

void plScriptCoroutine_MoveTo::Start(plGameObjectHandle hObject, const plVec3& vTargetPos, plTime duration, plEnum<plCurveFunction> easing)
{
  plGameObject* pObject = nullptr;
  if (plWorld::GetWorld(hObject)->TryGetObject(hObject, pObject) == false)
  {
    plLog::Error("MoveTo: The given game object was not found.");
    return;
  }

  m_hObject = hObject;
  m_vSourcePos = pObject->GetLocalPosition();
  m_vTargetPos = vTargetPos;
  m_Easing = easing;

  m_Duration = duration;
  m_TimePassed = plTime::MakeZero();
}

plScriptCoroutine::Result plScriptCoroutine_MoveTo::Update(plTime deltaTimeSinceLastUpdate)
{
  if (deltaTimeSinceLastUpdate.IsPositive())
  {
    plGameObject* pObject = nullptr;
    if (plWorld::GetWorld(m_hObject)->TryGetObject(m_hObject, pObject) == false)
    {
      return Result::Failed();
    }

    m_TimePassed += deltaTimeSinceLastUpdate;

    const double fDuration = m_Duration.GetSeconds();
    double fCurrentX = plMath::Min(fDuration > 0 ? m_TimePassed.GetSeconds() / fDuration : 1.0, 1.0);
    fCurrentX = plCurveFunction::GetValue(m_Easing, fCurrentX);

    plVec3 vCurrentPos = plMath::Lerp(m_vSourcePos, m_vTargetPos, static_cast<float>(fCurrentX));
    pObject->SetLocalPosition(vCurrentPos);
  }

  if (m_TimePassed < m_Duration)
  {
    return Result::Running();
  }

  return Result::Completed();
}
