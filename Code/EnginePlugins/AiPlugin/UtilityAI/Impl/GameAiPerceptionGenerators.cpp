#include <AiPlugin/AiPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <AiPlugin/UtilityAI/Framework/AiSensorManager.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptionGenerators.h>
#include <AiPlugin/UtilityAI/Impl/GameAiPerceptions.h>
#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>

plAiPerceptionGenPOI::plAiPerceptionGenPOI() = default;
plAiPerceptionGenPOI::~plAiPerceptionGenPOI() = default;

void plAiPerceptionGenPOI::UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  const plAiSensorSpatial* pSensorSee = static_cast<const plAiSensorSpatial*>(ref_SensorManager.GetSensor("Sensor_See"));
  if (pSensorSee == nullptr)
    return;

  plWorld* pWorld = owner.GetWorld();

  plHybridArray<plGameObjectHandle, 32> detectedObjects;
  pSensorSee->RetrieveSensations(owner, detectedObjects);

  for (plGameObjectHandle hObj : detectedObjects)
  {
    plGameObject* pObj = nullptr;
    if (pWorld->TryGetObject(hObj, pObj))
    {
      auto& g = m_Perceptions.ExpandAndGetRef();
      g.m_vGlobalPosition = pObj->GetGlobalPosition();
    }
  }
}

bool plAiPerceptionGenPOI::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void plAiPerceptionGenPOI::GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void plAiPerceptionGenPOI::FlagNeededSensors(plAiSensorManager& ref_SensorManager)
{
  ref_SensorManager.FlagAsNeeded("Sensor_See");
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plAiPerceptionGenWander::plAiPerceptionGenWander() = default;
plAiPerceptionGenWander::~plAiPerceptionGenWander() = default;

void plAiPerceptionGenWander::UpdatePerceptions(plGameObject& owner, const plAiSensorManager& ref_SensorManager)
{
  m_Perceptions.Clear();

  plWorld* pWorld = owner.GetWorld();
  const plVec3 c = owner.GetGlobalPosition();
  const plVec3 dir = 3.0f * owner.GetGlobalDirForwards();
  const plVec3 right = 5.0f * owner.GetGlobalDirRight();

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c + dir - right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir + right;
  }

  {
    auto& g = m_Perceptions.ExpandAndGetRef();
    g.m_vGlobalPosition = c - dir - right;
  }
}

bool plAiPerceptionGenWander::HasPerceptions() const
{
  return !m_Perceptions.IsEmpty();
}

void plAiPerceptionGenWander::GetPerceptions(plDynamicArray<const plAiPerception*>& out_Perceptions) const
{
  out_Perceptions.Reserve(out_Perceptions.GetCount() + m_Perceptions.GetCount());

  for (const auto& perception : m_Perceptions)
  {
    out_Perceptions.PushBack(&perception);
  }
}

void plAiPerceptionGenWander::FlagNeededSensors(plAiSensorManager& ref_SensorManager)
{
  ref_SensorManager.FlagAsNeeded("Sensor_See");
}
