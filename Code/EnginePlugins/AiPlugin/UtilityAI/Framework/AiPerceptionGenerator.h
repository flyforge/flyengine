#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>

class plGameObject;
class plAiSensorManager;

class PLASMA_AIPLUGIN_DLL plAiPerceptionGenerator
{
public:
  plAiPerceptionGenerator() = default;
  virtual ~plAiPerceptionGenerator() = default;

  virtual plStringView GetPerceptionType() = 0;
  virtual void UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager) = 0;
  virtual bool HasPerceptions() const = 0;
  virtual void GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const = 0;
  virtual void FlagNeededSensors(plAiSensorManager& ref_SensorManager) = 0;
};
