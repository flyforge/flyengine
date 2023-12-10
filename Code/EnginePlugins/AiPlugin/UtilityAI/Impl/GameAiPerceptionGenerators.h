#pragma once

#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiPerceptionGenPOI : public plAiPerceptionGenerator
{
public:
  plAiPerceptionGenPOI();
  ~plAiPerceptionGenPOI();

  virtual plStringView GetPerceptionType() override { return "plAiPerceptionPOI"_plsv; }
  virtual void UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(plAiSensorManager& ref_SensorManager) override;

private:
  plDynamicArray<plAiPerceptionPOI> m_Perceptions;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiPerceptionGenWander : public plAiPerceptionGenerator
{
public:
  plAiPerceptionGenWander();
  ~plAiPerceptionGenWander();

  virtual plStringView GetPerceptionType() override { return "plAiPerceptionWander"_plsv; }
  virtual void UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(plAiSensorManager& ref_SensorManager) override;

private:
  plDynamicArray<plAiPerceptionWander> m_Perceptions;
};
