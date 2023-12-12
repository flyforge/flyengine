#pragma once

#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

class plWorld;

class PLASMA_GAMEENGINE_DLL plVolumeSampler
{
public:
  plVolumeSampler();
  ~plVolumeSampler();

  void RegisterValue(plHashedString sName, plVariant defaultValue, plTime interpolationDuration = plTime::Zero());
  void DeregisterValue(plHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(plWorld& world, const plVec3& vGlobalPosition, plTime deltaTime);

  plVariant GetValue(plTempHashedString sName) const
  {
    if (const Value* pValue = m_Values.GetValue(sName))
    {
      return pValue->m_CurrentValue;
    }

    return plVariant();
  }

  static plUInt32 ComputeSortingKey(float fSortOrder, float fMaxScale);

private:
  struct Value
  {
    plVariant m_DefaultValue;
    plVariant m_CurrentValue;
    double m_fInterpolationFactor = -1.0;
  };

  plHashTable<plHashedString, Value> m_Values;
  plHashTable<plHashedString, plVariant> m_TargetValues;
};
