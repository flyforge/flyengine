#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Impl/GameAiSensors.h>
#include <GameEngine/AI/SensorComponent.h>

plAiSensorSpatial::plAiSensorSpatial(plTempHashedString sObjectName)
{
  m_sObjectName = sObjectName;
}

plAiSensorSpatial::~plAiSensorSpatial() = default;

void plAiSensorSpatial::UpdateSensor(plGameObject& owner)
{
  if (m_hSensorObject.IsInvalidated())
  {
    plGameObject* pSensors = owner.FindChildByName(m_sObjectName);

    if (pSensors == nullptr)
      return;

    m_hSensorObject = pSensors->GetHandle();
  }

  plWorld* pWorld = owner.GetWorld();
  plGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(m_hSensorObject, pSensors))
    return;

  plDynamicArray<plSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;


  plPhysicsWorldModuleInterface* pPhysicsWorldModule = pWorld->GetModule<plPhysicsWorldModuleInterface>();

  plHybridArray<plGameObject*, 32> objectsInSensorVolume;
  plHybridArray<plGameObjectHandle, 32> detectedObjects;

  for (auto pSensor : sensors)
  {
    pSensor->RunSensorCheck(pPhysicsWorldModule, objectsInSensorVolume, detectedObjects, false);
  }
}

void plAiSensorSpatial::RetrieveSensations(plGameObject& owner, plDynamicArray<plGameObjectHandle>& out_Sensations) const
{
  plWorld* pWorld = owner.GetWorld();
  plGameObject* pSensors = nullptr;
  if (!pWorld->TryGetObject(m_hSensorObject, pSensors))
    return;

  plDynamicArray<plSensorComponent*> sensors;
  pSensors->TryGetComponentsOfBaseType(sensors);

  if (sensors.IsEmpty())
    return;

  plPhysicsWorldModuleInterface* pPhysicsWorldModule = pWorld->GetModule<plPhysicsWorldModuleInterface>();

  plHybridArray<plGameObject*, 32> objectsInSensorVolume;
  plHybridArray<plGameObjectHandle, 32> detectedObjects;

  for (auto pSensor : sensors)
  {
    plArrayPtr<const plGameObjectHandle> detections = pSensor->GetLastDetectedObjects();
    out_Sensations.PushBackRange(detections);
  }

  if (out_Sensations.GetCount() > 16)
  {
    plLog::Warning("Much sensor input: {} items", out_Sensations.GetCount());
  }
}
