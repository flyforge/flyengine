#pragma once

#include <Foundation/Configuration/CVar.h>

template <typename Type, plCVarType::Enum CVarType>
plTypedCVar<Type, CVarType>::plTypedCVar(plStringView sName, const Type& value, plBitflags<plCVarFlags> flags, plStringView sDescription)
  : plCVar(sName, flags, sDescription)
{
  PLASMA_ASSERT_DEBUG(sName.FindSubString(" ") == nullptr, "CVar names must not contain whitespace");

  for (plUInt32 i = 0; i < plCVarValue::ENUM_COUNT; ++i)
    m_Values[i] = value;
}

template <typename Type, plCVarType::Enum CVarType>
plTypedCVar<Type, CVarType>::operator const Type&() const
{
  return (m_Values[plCVarValue::Current]);
}

template <typename Type, plCVarType::Enum CVarType>
plCVarType::Enum plTypedCVar<Type, CVarType>::GetType() const
{
  return CVarType;
}

template <typename Type, plCVarType::Enum CVarType>
void plTypedCVar<Type, CVarType>::SetToRestartValue()
{
  if (m_Values[plCVarValue::Current] == m_Values[plCVarValue::Restart])
    return;

  // this will NOT trigger a 'restart value changed' event
  m_Values[plCVarValue::Current] = m_Values[plCVarValue::Restart];

  plCVarEvent e(this);
  e.m_EventType = plCVarEvent::ValueChanged;
  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}

template <typename Type, plCVarType::Enum CVarType>
const Type& plTypedCVar<Type, CVarType>::GetValue(plCVarValue::Enum val) const
{
  return (m_Values[val]);
}

template <typename Type, plCVarType::Enum CVarType>
void plTypedCVar<Type, CVarType>::operator=(const Type& value)
{
  plCVarEvent e(this);

  if (GetFlags().IsAnySet(plCVarFlags::RequiresRestart))
  {
    if (value == m_Values[plCVarValue::Restart]) // no change
      return;

    e.m_EventType = plCVarEvent::RestartValueChanged;
  }
  else
  {
    if (m_Values[plCVarValue::Current] == value) // no change
      return;

    m_Values[plCVarValue::Current] = value;
    e.m_EventType = plCVarEvent::ValueChanged;
  }

  m_Values[plCVarValue::Restart] = value;

  m_CVarEvents.Broadcast(e);

  // broadcast the same to the 'all cvars' event handlers
  s_AllCVarEvents.Broadcast(e);
}
