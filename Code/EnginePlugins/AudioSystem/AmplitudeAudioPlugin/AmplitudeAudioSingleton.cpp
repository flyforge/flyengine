#include <AmplitudeAudioPlugin/AmplitudeAudioPluginPCH.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioSingleton.h>
#include <AmplitudeAudioPlugin/Core/AmplitudeAudioData.h>
#include <AmplitudeAudioPlugin/Core/Common.h>

#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Core/ResourceManager/ResourceManager.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlUtils.h>
#include <Foundation/IO/OpenDdlWriter.h>

#include <GameEngine/GameApplication/GameApplication.h>

#include <SparkyStudios/Audio/Amplitude/Amplitude.h>
#include <SparkyStudios/Audio/Amplitude/Core/Common/Constants.h>
#include <SparkyStudios/Audio/Amplitude/Math/HandmadeMath.h>

using namespace SparkyStudios::Audio;

namespace Log
{
  static void Write(const char* format, va_list args)
  {
    if (format && format[0] != '\0')
    {
      constexpr size_t bufferLen = 1024;
      char buffer[bufferLen] = "[Amplitude] ";

      vsnprintf(buffer + 12, bufferLen - 12, format, args);

      buffer[bufferLen - 1] = '\0';

      plLog::Debug("{0}", buffer);
    }
  }

  static void DeviceNotification(Amplitude::DeviceNotification notification, const Amplitude::DeviceDescription& device, Amplitude::Driver* driver)
  {
    switch (notification)
    {
      case Amplitude::DeviceNotification::Opened:
        plLog::Info("Device opened: {0}", device.mDeviceName.c_str());
        break;
      case Amplitude::DeviceNotification::Started:
        plLog::Info("Device started: {0}", device.mDeviceName.c_str());
        break;
      case Amplitude::DeviceNotification::Stopped:
        plLog::Info("Device stopped: {0}", device.mDeviceName.c_str());
        break;
      case Amplitude::DeviceNotification::Rerouted:
        plLog::Info("Device rerouted: {0}", device.mDeviceName.c_str());
        break;
      case Amplitude::DeviceNotification::Closed:
        plLog::Info("Device closed: {0}", device.mDeviceName.c_str());
        break;
    }
  }
} // namespace Log

namespace Memory
{
  static Amplitude::AmVoidPtr Malloc([[maybe_unused]] Amplitude::MemoryPoolKind pool, Amplitude::AmSize size)
  {
    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->Allocate(size, PL_AUDIOSYSTEM_MEMORY_ALIGNMENT);
  }

  static Amplitude::AmVoidPtr Malign([[maybe_unused]] Amplitude::MemoryPoolKind pool, Amplitude::AmSize size, Amplitude::AmUInt32 alignment)
  {
    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->Allocate(size, alignment);
  }

  static Amplitude::AmVoidPtr Realloc(Amplitude::MemoryPoolKind pool, Amplitude::AmVoidPtr address, Amplitude::AmSize size)
  {
    if (address == nullptr)
      return Malloc(pool, size);

    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->Reallocate(address, plAudioMiddlewareAllocatorWrapper::GetAllocator()->AllocatedSize(address), size, PL_AUDIOSYSTEM_MEMORY_ALIGNMENT);
  }

  static Amplitude::AmVoidPtr Realign(Amplitude::MemoryPoolKind pool, Amplitude::AmVoidPtr address, Amplitude::AmSize size, Amplitude::AmUInt32 alignment)
  {
    if (address == nullptr)
      return Malign(pool, size, alignment);

    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->Reallocate(address, plAudioMiddlewareAllocatorWrapper::GetAllocator()->AllocatedSize(address), size, alignment);
  }

  static void Free([[maybe_unused]] Amplitude::MemoryPoolKind pool, Amplitude::AmVoidPtr address)
  {
    plAudioMiddlewareAllocatorWrapper::GetAllocator()->Deallocate(address);
  }

  static Amplitude::AmSize TotalMemorySize()
  {
    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->GetStats().m_uiAllocationSize;
  }

  static Amplitude::AmSize SizeOfMemory([[maybe_unused]] Amplitude::MemoryPoolKind pool, Amplitude::AmVoidPtr address)
  {
    return plAudioMiddlewareAllocatorWrapper::GetAllocator()->AllocatedSize(address);
  }
} // namespace Memory

namespace Utils
{
  static AmVec3 plVec3ToAmVec3(const plVec3& vec)
  {
    return AM_V3(vec.x, vec.y, vec.z);
  }
} // namespace Utils

PL_IMPLEMENT_SINGLETON(plAmplitude);

void plAmplitudeConfiguration::Save(plOpenDdlWriter& ddl) const
{
  plOpenDdlUtils::StoreString(ddl, m_sInitSoundBank, s_szAmplitudeConfigKeyInitBank);
  plOpenDdlUtils::StoreString(ddl, m_sEngineConfigFileName, s_szAmplitudeConfigKeyEngineConfigFileName);
}

void plAmplitudeConfiguration::Load(const plOpenDdlReaderElement& ddl)
{
  if (const plOpenDdlReaderElement* pElement = ddl.FindChildOfType(plOpenDdlPrimitiveType::String, s_szAmplitudeConfigKeyInitBank))
  {
    m_sInitSoundBank = pElement->GetPrimitivesString()[0];
  }

  if (const plOpenDdlReaderElement* pElement = ddl.FindChildOfType(plOpenDdlPrimitiveType::String, s_szAmplitudeConfigKeyEngineConfigFileName))
  {
    m_sEngineConfigFileName = pElement->GetPrimitivesString()[0];
  }
}

bool plAmplitudeConfiguration::operator==(const plAmplitudeConfiguration& rhs) const
{
  if (m_sInitSoundBank != rhs.m_sInitSoundBank)
    return false;

  if (m_sEngineConfigFileName != rhs.m_sEngineConfigFileName)
    return false;

  return true;
}

plResult plAmplitudeAssetProfiles::Save(plOpenDdlWriter& writer) const
{
  if (m_AssetProfiles.IsEmpty())
    return PL_FAILURE;

  for (auto it = m_AssetProfiles.GetIterator(); it.IsValid(); ++it)
  {
    if (!it.Key().IsEmpty())
    {
      writer.BeginObject("Platform", it.Key());
      {
        it.Value().Save(writer);
      }
      writer.EndObject();
    }
  }

  return PL_SUCCESS;
}

plResult plAmplitudeAssetProfiles::Load(const plOpenDdlReaderElement& reader)
{
  m_AssetProfiles.Clear();

  const plOpenDdlReaderElement* pChild = reader.GetFirstChild();

  while (pChild)
  {
    if (pChild->IsCustomType("Platform") && pChild->HasName())
    {
      auto& cfg = m_AssetProfiles[pChild->GetName()];

      cfg.Load(*pChild);
    }

    pChild = pChild->GetSibling();
  }

  return PL_SUCCESS;
}


plAmplitude::plAmplitude()
  : m_SingletonRegistrar(this)
  , m_pEngine(nullptr)
  , m_dCurrentTime(0.0)
  , m_bInitialized(false)
{
  m_pData = PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), Data);
}

plAmplitude::~plAmplitude()
{
  // Shutdown().IgnoreResult();
}

plResult plAmplitude::LoadConfiguration(const plOpenDdlReaderElement& reader)
{
  return m_pData->m_Configs.Load(reader);
}

plResult plAmplitude::Startup()
{
  if (m_bInitialized)
    return PL_SUCCESS;

  DetectPlatform();

  if (!m_pData->m_Configs.m_AssetProfiles.Contains(m_pData->m_sPlatform))
  {
    plLog::Error("Amplitude configuration for platform '{0}' not available. Amplitude will be deactivated.", m_pData->m_sPlatform);
    return PL_FAILURE;
  }

  const auto& config = m_pData->m_Configs.m_AssetProfiles[m_pData->m_sPlatform];

  // Initialize the engine
  Amplitude::RegisterLogFunc(Log::Write);
  Amplitude::RegisterDeviceNotificationCallback(Log::DeviceNotification);

  plStringBuilder assetsPath;

  if (plFileSystem::ResolvePath(":project/Sounds/Amplitude/amplitude_assets", &assetsPath, nullptr).Failed())
  {
    plLog::Error("No Amplitude assets directory available in the project. Amplitude will be deactivated.", m_pData->m_sPlatform);
    return PL_FAILURE;
  }

  m_pEngine = Amplitude::Engine::GetInstance();
  PL_ASSERT_DEBUG(m_pEngine != nullptr, "Amplitude engine not available.");

  m_Loader.SetBasePath(AM_STRING_TO_OS_STRING(assetsPath.GetData()));

  Amplitude::MemoryManagerConfig memConfig;
  memConfig.alignedMalloc = Memory::Malign;
  memConfig.alignedRealloc = Memory::Realign;
  memConfig.free = Memory::Free;
  memConfig.malloc = Memory::Malloc;
  memConfig.realloc = Memory::Realloc;
  memConfig.sizeOf = Memory::SizeOfMemory;
  memConfig.totalReservedMemorySize = Memory::TotalMemorySize;

  Amplitude::MemoryManager::Initialize(memConfig);
  PL_ASSERT_DEBUG(Amplitude::MemoryManager::IsInitialized(), "Amplitude memory manager not initialized.");

  m_pEngine->SetFileLoader(m_Loader);

  m_pEngine->Initialize(AM_STRING_TO_OS_STRING(config.m_sEngineConfigFileName.GetData())), "Amplitude engine initialization failed.";

  Amplitude::AmBankID uiInitBankId = Amplitude::kAmInvalidObjectId;

  plLog::Warning("Amplitude Soundbank: {0}", config.m_sInitSoundBank.GetData());

  if (!m_pEngine->LoadSoundBank(AM_STRING_TO_OS_STRING(config.m_sInitSoundBank.GetData()), uiInitBankId))
  {
    plLog::Error("Amplitude engine initialization failed. Could not load initial sound bank '{0}'.", config.m_sInitSoundBank);
    return PL_FAILURE;
  }

  m_bInitialized = true;

  plLog::Success("Amplitude Audio initialized.");
  return PL_SUCCESS;
}

plResult plAmplitude::Shutdown()
{
  if (m_bInitialized)
  {
    m_bInitialized = false;

    if (m_pEngine != nullptr)
    {
      m_pEngine->Deinitialize();
      m_pEngine = nullptr;
    }

    // Terminate the Memory Manager
    if (Amplitude::MemoryManager::IsInitialized())
    {
      Amplitude::MemoryManager::Deinitialize();
    }

    // Finally delete all data
    m_pData.Clear();

    plLog::Success("Amplitude Audio deinitialized.");
  }

  return PL_SUCCESS;
}

plResult plAmplitude::Release()
{
  return PL_SUCCESS;
}

plResult plAmplitude::StopAllSounds()
{
  if (m_bInitialized)
  {
    m_pEngine->StopAll();
  }

  return PL_SUCCESS;
}

plResult plAmplitude::AddEntity(plAudioSystemEntityData* pEntityData, const char* szEntityName)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const Amplitude::Entity& entity = m_pEngine->AddEntity(pAmplitudeEntity->m_uiAmId);

  return entity.Valid() ? PL_SUCCESS : PL_FAILURE;
}

plResult plAmplitude::ResetEntity(plAudioSystemEntityData* pEntityData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  // Nothing to reset for now...

  return PL_SUCCESS;
}

plResult plAmplitude::UpdateEntity(plAudioSystemEntityData* pEntityData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  // Nothing to update for now... The real entity update is scheduled in the Amplitude's Update function.

  return PL_SUCCESS;
}

plResult plAmplitude::RemoveEntity(plAudioSystemEntityData* pEntityData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  m_pEngine->RemoveEntity(pAmplitudeEntity->m_uiAmId);
  const Amplitude::Entity& entity = m_pEngine->GetEntity(pAmplitudeEntity->m_uiAmId);

  return entity.Valid() ? PL_FAILURE : PL_SUCCESS;
}

plResult plAmplitude::SetEntityTransform(plAudioSystemEntityData* pEntityData, const plAudioSystemTransform& Transform)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  if (const Amplitude::Entity& entity = m_pEngine->GetEntity(pAmplitudeEntity->m_uiAmId); entity.Valid())
  {
    entity.SetLocation(Utils::plVec3ToAmVec3(Transform.m_vPosition));
    entity.SetOrientation(Utils::plVec3ToAmVec3(-Transform.m_vForward), Utils::plVec3ToAmVec3(Transform.m_vUp));
  }

  return PL_SUCCESS;
}

plResult plAmplitude::LoadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData)
{
  // Amplitude doesn't support loading triggers

  return PL_SUCCESS;
}

plResult plAmplitude::ActivateTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData, plAudioSystemEventData* pEventData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeTrigger = plDynamicCast<const plAmplitudeAudioTriggerData*>(pTriggerData);
  if (pAmplitudeTrigger == nullptr)
    return PL_FAILURE;

  auto* pAmplitudeEvent = plDynamicCast<plAmplitudeAudioEventData*>(pEventData);
  if (pAmplitudeEvent == nullptr)
    return PL_FAILURE;

  plResult result = PL_FAILURE;

  Amplitude::AmEntityID entityId = 0;

  // if (amplitudeEntity->m_bHasPosition)
  // {
  // Positioned entities
  entityId = pAmplitudeEntity->m_uiAmId;
  // }

  const Amplitude::Entity& entity = m_pEngine->GetEntity(entityId);

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  if (entityId != Amplitude::kAmInvalidObjectId && !entity.Valid())
  {
    plLog::Error("Unable to find entity with ID: {0}", entityId);
  }
#endif

  if (Amplitude::EventHandle const event = m_pEngine->GetEventHandle(pAmplitudeTrigger->m_uiAmId))
  {
    if (const Amplitude::EventCanceler& canceler = m_pEngine->Trigger(event, entity); canceler.Valid())
    {
      pAmplitudeEvent->m_eState = plAudioSystemEventState::Playing;
      pAmplitudeEvent->m_EventCanceler = canceler;
      result = PL_SUCCESS;
    }
  }
  else
  {
    plLog::Error("[Amplitude] Unable to activate a trigger, the associated Amplitude event with ID {0} has not been found in loaded banks.", pAmplitudeTrigger->m_uiAmId);
  }

  return result;
}

plResult plAmplitude::UnloadTrigger(plAudioSystemEntityData* pEntityData, const plAudioSystemTriggerData* pTriggerData)
{
  // Amplitude doesn't support unloading triggers

  return PL_SUCCESS;
}

plResult plAmplitude::StopEvent(plAudioSystemEntityData* pEntityData, const plAudioSystemEventData* pEventData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeEvent = plDynamicCast<const plAmplitudeAudioEventData*>(pEventData);
  if (pAmplitudeEvent == nullptr)
    return PL_FAILURE;

  plResult result = PL_FAILURE;

  switch (pAmplitudeEvent->m_eState)
  {
    case plAudioSystemEventState::Playing:
    {
      if (pAmplitudeEvent->m_EventCanceler.Valid())
      {
        pAmplitudeEvent->m_EventCanceler.Cancel();
        result = PL_SUCCESS;
        plLog::Success("[Amplitude] Successfully stopped the event.");
      }
      else
      {
        plLog::Error("[Amplitude] Encountered a running event without a valid canceler.");
      }
      break;
    }
    default:
    {
      plLog::Warning("[Amplitude] Stopping an event of this type is not supported yet");
      break;
    }
  }

  return result;
}

plResult plAmplitude::StopAllEvents(plAudioSystemEntityData* pEntityData)
{
  // TODO: Stop all events
  return PL_SUCCESS;
}

plResult plAmplitude::SetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData, float fValue)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeRtpc = plDynamicCast<const plAmplitudeAudioRtpcData*>(pRtpcData);
  if (pAmplitudeRtpc == nullptr)
    return PL_FAILURE;

  Amplitude::RtpcHandle const rtpc = m_pEngine->GetRtpcHandle(pAmplitudeRtpc->m_uiAmId);

  if (rtpc == nullptr)
    return PL_FAILURE;

  rtpc->SetValue(fValue);

  return PL_SUCCESS;
}

plResult plAmplitude::ResetRtpc(plAudioSystemEntityData* pEntityData, const plAudioSystemRtpcData* pRtpcData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeRtpc = plDynamicCast<const plAmplitudeAudioRtpcData*>(pRtpcData);
  if (pAmplitudeRtpc == nullptr)
    return PL_FAILURE;

  Amplitude::RtpcHandle const rtpc = m_pEngine->GetRtpcHandle(pAmplitudeRtpc->m_uiAmId);

  if (rtpc == nullptr)
    return PL_FAILURE;

  rtpc->Reset();

  return PL_SUCCESS;
}

plResult plAmplitude::SetSwitchState(plAudioSystemEntityData* pEntityData, const plAudioSystemSwitchStateData* pSwitchStateData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeSwitch = plDynamicCast<const plAmplitudeAudioSwitchStateData*>(pSwitchStateData);
  if (pAmplitudeSwitch == nullptr)
    return PL_FAILURE;

  Amplitude::SwitchHandle const _switch = m_pEngine->GetSwitchHandle(pAmplitudeSwitch->m_uiSwitchId);

  if (_switch == nullptr)
    return PL_FAILURE;

  _switch->SetState(pAmplitudeSwitch->m_uiSwitchStateId);

  return PL_SUCCESS;
}

plResult plAmplitude::SetObstructionAndOcclusion(plAudioSystemEntityData* pEntityData, float fObstruction, float fOcclusion)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const Amplitude::Entity& entity = m_pEngine->GetEntity(pAmplitudeEntity->m_uiAmId);

  if (!entity.Valid())
    return PL_FAILURE;

  entity.SetObstruction(fObstruction);
  entity.SetOcclusion(fOcclusion);

  return PL_SUCCESS;
}

plResult plAmplitude::SetEnvironmentAmount(plAudioSystemEntityData* pEntityData, const plAudioSystemEnvironmentData* pEnvironmentData, float fAmount)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeEntity = plDynamicCast<plAmplitudeAudioEntityData*>(pEntityData);
  if (pAmplitudeEntity == nullptr)
    return PL_FAILURE;

  const auto* const pAmplitudeEnvironment = plDynamicCast<const plAmplitudeAudioEnvironmentData*>(pEnvironmentData);
  if (pAmplitudeEnvironment == nullptr)
    return PL_FAILURE;

  const Amplitude::Environment& environment = m_pEngine->AddEnvironment(pAmplitudeEnvironment->m_uiAmId);
  environment.SetEffect(pAmplitudeEnvironment->m_uiEffectId);

  if (!environment.Valid())
    return PL_FAILURE;

  const Amplitude::Entity& entity = m_pEngine->GetEntity(pAmplitudeEntity->m_uiAmId);

  if (!entity.Valid())
    return PL_FAILURE;

  entity.SetEnvironmentFactor(environment.GetId(), fAmount);

  return PL_SUCCESS;
}

plResult plAmplitude::AddListener(plAudioSystemListenerData* pListenerData, const char* szListenerName)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeListener = plDynamicCast<plAmplitudeAudioListenerData*>(pListenerData);
  if (pAmplitudeListener == nullptr)
    return PL_FAILURE;

  const Amplitude::Listener& listener = m_pEngine->AddListener(pAmplitudeListener->m_uiAmId);

  return listener.Valid() ? PL_SUCCESS : PL_FAILURE;
}

plResult plAmplitude::ResetListener(plAudioSystemListenerData* pListenerData)
{
  // Nothing to reset for now
  return PL_SUCCESS;
}

plResult plAmplitude::RemoveListener(plAudioSystemListenerData* pListenerData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeListener = plDynamicCast<plAmplitudeAudioListenerData*>(pListenerData);
  if (pAmplitudeListener == nullptr)
    return PL_FAILURE;

  m_pEngine->RemoveListener(pAmplitudeListener->m_uiAmId);
  const Amplitude::Listener& listener = m_pEngine->GetListener(pAmplitudeListener->m_uiAmId);

  return listener.Valid() ? PL_FAILURE : PL_SUCCESS;
}

plResult plAmplitude::SetListenerTransform(plAudioSystemListenerData* pListenerData, const plAudioSystemTransform& Transform)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeListener = plDynamicCast<plAmplitudeAudioListenerData*>(pListenerData);
  if (pAmplitudeListener == nullptr)
    return PL_FAILURE;

  if (const Amplitude::Listener& listener = m_pEngine->GetListener(pAmplitudeListener->m_uiAmId); listener.Valid())
  {
    listener.SetLocation(Utils::plVec3ToAmVec3(Transform.m_vPosition));
    listener.SetOrientation(Utils::plVec3ToAmVec3(-Transform.m_vForward), Utils::plVec3ToAmVec3(Transform.m_vUp));
  }

  return PL_SUCCESS;
}

plResult plAmplitude::LoadBank(plAudioSystemBankData* pBankData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeBank = plDynamicCast<plAmplitudeAudioSoundBankData*>(pBankData);
  if (pAmplitudeBank == nullptr)
    return PL_FAILURE;

  if (!m_pEngine->LoadSoundBank(AM_STRING_TO_OS_STRING(pAmplitudeBank->m_sFileName.GetData())))
  {
    plLog::Error("[Amplitude] Could not load sound bank '{0}'.", pAmplitudeBank->m_sFileName);
    return PL_FAILURE;
  }

  return PL_SUCCESS;
}

plResult plAmplitude::UnloadBank(plAudioSystemBankData* pBankData)
{
  if (!m_bInitialized)
    return PL_FAILURE;

  const auto* const pAmplitudeBank = plDynamicCast<plAmplitudeAudioSoundBankData*>(pBankData);
  if (pAmplitudeBank == nullptr)
    return PL_FAILURE;

  if (pAmplitudeBank->m_uiAmId == Amplitude::kAmInvalidObjectId)
    return PL_FAILURE;

  m_pEngine->UnloadSoundBank(pAmplitudeBank->m_uiAmId);

  return PL_SUCCESS;
}

plAudioSystemEntityData* plAmplitude::CreateWorldEntity(plAudioSystemDataID uiEntityId)
{
  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioEntityData, uiEntityId, false);
}

plAudioSystemEntityData* plAmplitude::CreateEntityData(plAudioSystemDataID uiEntityId)
{
  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioEntityData, uiEntityId);
}

plResult plAmplitude::DestroyEntityData(plAudioSystemEntityData* pEntityData)
{
  if (pEntityData == nullptr || !pEntityData->IsInstanceOf<plAmplitudeAudioEntityData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pEntityData);
  return PL_SUCCESS;
}

plAudioSystemListenerData* plAmplitude::CreateListenerData(plAudioSystemDataID uiListenerId)
{
  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioListenerData, uiListenerId);
}

plResult plAmplitude::DestroyListenerData(plAudioSystemListenerData* pListenerData)
{
  if (pListenerData == nullptr || !pListenerData->IsInstanceOf<plAmplitudeAudioListenerData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pListenerData);
  return PL_SUCCESS;
}

plAudioSystemEventData* plAmplitude::CreateEventData(plAudioSystemDataID uiEventId)
{
  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioEventData, uiEventId);
}

plResult plAmplitude::ResetEventData(plAudioSystemEventData* pEventData)
{
  if (pEventData == nullptr || !pEventData->IsInstanceOf<plAmplitudeAudioEventData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pEventData);
  return PL_SUCCESS;
}

plResult plAmplitude::DestroyEventData(plAudioSystemEventData* pEventData)
{
  if (pEventData == nullptr || !pEventData->IsInstanceOf<plAmplitudeAudioEventData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pEventData);
  return PL_SUCCESS;
}

plAudioSystemBankData* plAmplitude::DeserializeBankEntry(plStreamReader* pBankEntry)
{
  if (pBankEntry == nullptr)
    return nullptr;

  Amplitude::AmBankID uiAmId;
  *pBankEntry >> uiAmId;

  plString sFileName;
  *pBankEntry >> sFileName;

  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioSoundBankData, uiAmId, sFileName);
}

plResult plAmplitude::DestroyBank(plAudioSystemBankData* pBankData)
{
  if (pBankData == nullptr || !pBankData->IsInstanceOf<plAmplitudeAudioSoundBankData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pBankData);
  return PL_SUCCESS;
}

plAudioSystemTriggerData* plAmplitude::DeserializeTriggerEntry(plStreamReader* pTriggerEntry) const
{
  if (pTriggerEntry == nullptr)
    return nullptr;

  Amplitude::AmEventID uiAmId;
  *pTriggerEntry >> uiAmId;

  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioTriggerData, uiAmId);
}

plResult plAmplitude::DestroyTriggerData(plAudioSystemTriggerData* pTriggerData)
{
  if (pTriggerData == nullptr || !pTriggerData->IsInstanceOf<plAmplitudeAudioTriggerData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pTriggerData);
  return PL_SUCCESS;
}

plAudioSystemRtpcData* plAmplitude::DeserializeRtpcEntry(plStreamReader* pRtpcEntry) const
{
  if (pRtpcEntry == nullptr)
    return nullptr;

  Amplitude::AmEventID uiAmId;
  *pRtpcEntry >> uiAmId;

  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioRtpcData, uiAmId);
}

plResult plAmplitude::DestroyRtpcData(plAudioSystemRtpcData* pRtpcData)
{
  if (pRtpcData == nullptr || !pRtpcData->IsInstanceOf<plAmplitudeAudioRtpcData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pRtpcData);
  return PL_SUCCESS;
}

plAudioSystemSwitchStateData* plAmplitude::DeserializeSwitchStateEntry(plStreamReader* pSwitchStateEntry) const
{
  if (pSwitchStateEntry == nullptr)
    return nullptr;

  Amplitude::AmSwitchID uiSwitchId;
  *pSwitchStateEntry >> uiSwitchId;

  Amplitude::AmObjectID uiSwitchStateId;
  *pSwitchStateEntry >> uiSwitchStateId;

  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioSwitchStateData, uiSwitchId, uiSwitchStateId);
}

plResult plAmplitude::DestroySwitchStateData(plAudioSystemSwitchStateData* pSwitchStateData)
{
  if (pSwitchStateData == nullptr || !pSwitchStateData->IsInstanceOf<plAmplitudeAudioSwitchStateData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pSwitchStateData);
  return PL_SUCCESS;
}

plAudioSystemEnvironmentData* plAmplitude::DeserializeEnvironmentEntry(plStreamReader* pEnvironmentEntry) const
{
  if (pEnvironmentEntry == nullptr)
    return nullptr;

  Amplitude::AmEnvironmentID uiEnvironmentId;
  *pEnvironmentEntry >> uiEnvironmentId;

  Amplitude::AmEffectID uiEffectId;
  *pEnvironmentEntry >> uiEffectId;

  return PL_NEW(plAudioMiddlewareAllocatorWrapper::GetAllocator(), plAmplitudeAudioEnvironmentData, uiEnvironmentId, uiEffectId);
}

plResult plAmplitude::DestroyEnvironmentData(plAudioSystemEnvironmentData* pEnvironmentData)
{
  if (pEnvironmentData == nullptr || !pEnvironmentData->IsInstanceOf<plAmplitudeAudioEnvironmentData>())
    return PL_FAILURE;

  PL_DELETE(plAudioMiddlewareAllocatorWrapper::GetAllocator(), pEnvironmentData);
  return PL_SUCCESS;
}

plResult plAmplitude::SetLanguage(const char* szLanguage)
{
  return PL_SUCCESS;
}

const char* plAmplitude::GetMiddlewareName() const
{
  return s_szAmplitudeMiddlewareName;
}

float plAmplitude::GetMasterGain() const
{
  return m_pEngine->GetMasterGain();
}

bool plAmplitude::GetMute() const
{
  return m_pEngine->GetMute();
}

void plAmplitude::OnMasterGainChange(float fGain)
{
  // Master Volume
  m_pEngine->SetMasterGain(fGain);
}

void plAmplitude::OnMuteChange(bool bMute)
{
  // Mute
  m_pEngine->SetMute(bMute);
}

void plAmplitude::OnLoseFocus()
{
}

void plAmplitude::OnGainFocus()
{
}

void plAmplitude::Update(plTime delta)
{
  if (m_pEngine == nullptr)
    return;

  PL_ASSERT_DEV(m_pData != nullptr, "UpdateSound() should not be called at this time.");

  m_pEngine->AdvanceFrame(delta.AsFloatInSeconds());
}

void plAmplitude::DetectPlatform() const
{
  if (!m_pData->m_sPlatform.IsEmpty())
    return;

#if PL_ENABLED(PL_PLATFORM_WINDOWS_DESKTOP)
  m_pData->m_sPlatform = "Desktop";

#elif PL_ENABLED(PL_PLATFORM_WINDOWS_UWP)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode

#elif PL_ENABLED(PL_PLATFORM_LINUX)
  m_pData->m_sPlatform = "Desktop"; /// \todo Need to detect mobile device mode (Android)

#elif PL_ENABLED(PL_PLATFORM_OSX)
  m_pData->m_sPlatform = "Desktop";

#elif PL_ENABLED(PL_PLATFORM_IOS)
  m_pData->m_sPlatform = "Mobile";

#elif PL_ENABLED(PL_PLATFORM_ANDROID)
  m_pData->m_sPlatform = "Mobile";

#elif
#  error "Unknown Platform"

#endif
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_AmplitudeAudioSingleton);
