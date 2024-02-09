#pragma once

#include <Core/World/Declarations.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/HashedString.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>

class PL_AIPLUGIN_DLL plAiSensorSpatial : public plAiSensor
{
public:
  plAiSensorSpatial(plTempHashedString sObjectName);
  ~plAiSensorSpatial();

  virtual void UpdateSensor(plGameObject& owner) override;
  void RetrieveSensations(plGameObject& owner, plDynamicArray<plGameObjectHandle>& out_Sensations) const;

  plTempHashedString m_sObjectName;

private:
  plGameObjectHandle m_hSensorObject;
};
