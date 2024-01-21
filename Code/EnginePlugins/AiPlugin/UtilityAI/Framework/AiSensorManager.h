#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiSensor.h>

class PLASMA_AIPLUGIN_DLL plAiSensorManager
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAiSensorManager);

public:
  plAiSensorManager();
  ~plAiSensorManager();

  void AddSensor(plStringView sName, plUniquePtr<plAiSensor>&& pSensor);

  void FlagAsNeeded(plStringView sName);

  void UpdateNeededSensors(plGameObject& owner);

  const plAiSensor* GetSensor(plStringView sName) const;

private:
  struct SensorInfo
  {
    plString m_sName;
    plUniquePtr<plAiSensor> m_pSensor;
    bool m_bActive = true;
    plUInt32 m_uiNeededInUpdate = 0;
  };

  plUInt32 m_uiUpdateCount = 1;
  plHybridArray<SensorInfo, 2> m_Sensors;
};
