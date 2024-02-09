#pragma once

#include <AiPlugin/UtilityAI/Framework/AiPerceptionGenerator.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiPerceptionGenPOI : public plAiPerceptionGenerator
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

class PL_AIPLUGIN_DLL plAiPerceptionGenWander : public plAiPerceptionGenerator
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

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiPerceptionGenCheckpoint : public plAiPerceptionGenerator
{
public:
  plAiPerceptionGenCheckpoint();
  ~plAiPerceptionGenCheckpoint();

  virtual plStringView GetPerceptionType() override { return "plAiPerceptionCheckpoint"_plsv; }
  virtual void UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager) override;
  virtual bool HasPerceptions() const override;
  virtual void GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const override;
  virtual void FlagNeededSensors(plAiSensorManager& ref_SensorManager) override;

private:
  plDynamicArray<plAiPerceptionCheckpoint> m_Perceptions;
  plSpatialData::Category m_SpatialCategory = plInvalidSpatialDataCategory;
};
