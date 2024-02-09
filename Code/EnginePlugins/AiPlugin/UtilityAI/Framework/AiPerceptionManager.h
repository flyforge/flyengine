#pragma once

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/UniquePtr.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>

class plGameObject;
class plAiSensorManager;

class PL_AIPLUGIN_DLL plAiPerceptionManager
{
  PL_DISALLOW_COPY_AND_ASSIGN(plAiPerceptionManager);

public:
  plAiPerceptionManager();
  ~plAiPerceptionManager();

  void FlagPerceptionTypeAsNeeded(plStringView sPerceptionType);

  void AddGenerator(plUniquePtr<plAiPerceptionGenerator>&& pGenerator);

  void FlagNeededSensors(plAiSensorManager& ref_SensorManager);

  void UpdateNeededPerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager);

  bool HasPerceptionsOfType(plStringView sPerceptionType) const;
  void GetPerceptionsOfType(plStringView sPerceptionType, plDynamicArray<const plAiPerception*>& out_Perceptions) const;

private:
  struct PerceptionInfo
  {
    plUniquePtr<plAiPerceptionGenerator> m_pPerceptionGenerator;
    plUInt32 m_uiNeededInUpdate = 0;
  };

  plUInt32 m_uiUpdateCount = 1;
  plHybridArray<PerceptionInfo, 12> m_PerceptionGenerators;
};
