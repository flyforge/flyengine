#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Types/Variant.h>
#include <GameEngine/GameEngineDLL.h>

/// \brief A volume sampler is used to sample the registered values from volumes at a given position. It also takes care of interpolation over time of those values.
class PL_GAMEENGINE_DLL plVolumeSampler
{
public:
  plVolumeSampler();
  ~plVolumeSampler();

  void RegisterValue(plHashedString sName, plVariant defaultValue, plTime interpolationDuration = plTime::MakeZero());
  void DeregisterValue(plHashedString sName);
  void DeregisterAllValues();

  void SampleAtPosition(const plWorld& world, plSpatialData::Category spatialCategory, const plVec3& vGlobalPosition, plTime deltaTime);

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
    plVariant m_TargetValue;
    plVariant m_CurrentValue;
    double m_fInterpolationFactor = -1.0;
  };

  plHashTable<plHashedString, Value> m_Values;
};
