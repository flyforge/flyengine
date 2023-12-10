#include <AiPlugin/AiPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <AiPlugin/UtilityAI/Framework/AiPerceptionManager.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>

plAiPerceptionManager::plAiPerceptionManager() = default;
plAiPerceptionManager::~plAiPerceptionManager() = default;

void plAiPerceptionManager::FlagPerceptionTypeAsNeeded(plStringView sPerceptionType)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType)
    {
      p.m_uiNeededInUpdate = m_uiUpdateCount;
    }
  }
}

void plAiPerceptionManager::AddGenerator(plUniquePtr<plAiPerceptionGenerator>&& pGenerator)
{
  auto& p = m_PerceptionGenerators.ExpandAndGetRef();
  p.m_pPerceptionGenerator = std::move(pGenerator);
}

void plAiPerceptionManager::FlagNeededSensors(plAiSensorManager& ref_SensorManager)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      p.m_pPerceptionGenerator->FlagNeededSensors(ref_SensorManager);
    }
  }
}

void plAiPerceptionManager::UpdateNeededPerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager)
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      p.m_pPerceptionGenerator->UpdatePerceptions(owner, ref_SensorManager);
    }
  }

  ++m_uiUpdateCount;
}

bool plAiPerceptionManager::HasPerceptionsOfType(plStringView sPerceptionType) const
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType && p.m_pPerceptionGenerator->HasPerceptions())
    {
      return true;
    }
  }

  return false;
}

void plAiPerceptionManager::GetPerceptionsOfType(plStringView sPerceptionType, plDynamicArray<const plAiPerception*>& out_Perceptions) const
{
  for (auto& p : m_PerceptionGenerators)
  {
    if (p.m_pPerceptionGenerator->GetPerceptionType() == sPerceptionType)
    {
      p.m_pPerceptionGenerator->GetPerceptions(out_Perceptions);
    }
  }
}
