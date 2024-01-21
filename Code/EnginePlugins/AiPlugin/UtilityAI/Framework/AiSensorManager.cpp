#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

plAiSensorManager::plAiSensorManager() = default;
plAiSensorManager::~plAiSensorManager() = default;

void plAiSensorManager::AddSensor(plStringView sName, plUniquePtr<plAiSensor>&& pSensor)
{
  auto& s = m_Sensors.ExpandAndGetRef();
  s.m_sName = sName;
  s.m_pSensor = std::move(pSensor);
}

void plAiSensorManager::FlagAsNeeded(plStringView sName)
{
  for (auto& s : m_Sensors)
  {
    if (s.m_sName == sName)
    {
      s.m_uiNeededInUpdate = m_uiUpdateCount;
      return;
    }
  }
}

void plAiSensorManager::UpdateNeededSensors(plGameObject& owner)
{
  for (auto& s : m_Sensors)
  {
    if (s.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      s.m_pSensor->UpdateSensor(owner);
    }
  }

  ++m_uiUpdateCount;
}

const plAiSensor* plAiSensorManager::GetSensor(plStringView sName) const
{
  for (auto& s : m_Sensors)
  {
    if (s.m_sName == sName)
    {
      return s.m_pSensor.Borrow();
    }
  }

  return nullptr;
}
