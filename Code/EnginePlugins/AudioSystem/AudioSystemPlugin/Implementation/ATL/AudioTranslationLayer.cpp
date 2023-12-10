#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayer.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
#  include <RendererCore/Debug/DebugRenderer.h>
#  include <RendererCore/Pipeline/View.h>
#  include <RendererCore/RenderWorld/RenderWorld.h>

plCVarBool cvar_AudioSystemDebug("Audio.Debugging.Enable", false, plCVarFlags::None, "Defines if Audio System debug information are displayed.");
#endif

plCVarInt cvar_AudioSystemMemoryEntitiesPoolSize("Audio.Memory.EntitiesPoolSize", 1024, plCVarFlags::Save, "Specify the pre-allocated number of entities in the pool.");
plCVarFloat cvar_AudioSystemGain("Audio.MasterGain", 1.0f, plCVarFlags::Save, "The main volume of the audio system.");
plCVarBool cvar_AudioSystemMute("Audio.Mute", false, plCVarFlags::Default, "Whether sound output is muted.");

plAudioTranslationLayer::plAudioTranslationLayer() = default;
plAudioTranslationLayer::~plAudioTranslationLayer() = default;

plResult plAudioTranslationLayer::Startup()
{
  m_pAudioMiddleware = plSingletonRegistry::GetSingletonInstance<plAudioMiddleware>();

  if (m_pAudioMiddleware == nullptr)
  {
    plLog::Error("Unable to load the ATL, there is no audio middleware implementation found. Make sure you have enabled an audio middleware plugin.");
    return PLASMA_FAILURE;
  }

  // Load configuration
  plAudioSystem::GetSingleton()->LoadConfiguration(">project/Sounds/AudioSystemConfig.ddl");

  // Start the audio middleware
  PLASMA_SUCCEED_OR_RETURN_CUSTOM_LOG(m_pAudioMiddleware->Startup(), "Unable to load the ATL. An error occurred while loading the audio middleware.");

  // Register CVar update events
  cvar_AudioSystemGain.m_CVarEvents.AddEventHandler(plMakeDelegate(&plAudioTranslationLayer::OnMasterGainChange, this));
  cvar_AudioSystemMute.m_CVarEvents.AddEventHandler(plMakeDelegate(&plAudioTranslationLayer::OnMuteChange, this));

  plLog::Success("ATL loaded successfully. Using {0} as the audio middleware.", m_pAudioMiddleware->GetMiddlewareName());
  return PLASMA_SUCCESS;
}

void plAudioTranslationLayer::Shutdown()
{
  if (m_pAudioMiddleware != nullptr)
  {
    // Unregister CVar update events
    cvar_AudioSystemGain.m_CVarEvents.RemoveEventHandler(plMakeDelegate(&plAudioTranslationLayer::OnMasterGainChange, this));
    cvar_AudioSystemMute.m_CVarEvents.RemoveEventHandler(plMakeDelegate(&plAudioTranslationLayer::OnMuteChange, this));

    m_pAudioMiddleware->Shutdown().IgnoreResult();
  }

  m_pAudioMiddleware = nullptr;

  m_mEntities.Clear();
  m_mListeners.Clear();
  m_mTriggers.Clear();
  m_mRtpcs.Clear();
  m_mSwitchStates.Clear();
}

void plAudioTranslationLayer::Update()
{
  PLASMA_PROFILE_SCOPE("AudioSystem");

  const plTime currentUpdateTime = plTime::Now();
  m_LastFrameTime = currentUpdateTime - m_LastUpdateTime;
  m_LastUpdateTime = currentUpdateTime;

  auto* pAudioMiddleware = plSingletonRegistry::GetSingletonInstance<plAudioMiddleware>();

  if (pAudioMiddleware == nullptr)
    return;

  pAudioMiddleware->Update(m_LastFrameTime);
}

plAudioSystemDataID plAudioTranslationLayer::GetTriggerId(plStringView sTriggerName) const
{
  const auto uiTriggerId = plHashHelper<plStringView>::Hash(sTriggerName);

  if (const auto it = m_mTriggers.Find(uiTriggerId); it.IsValid())
  {
    return uiTriggerId;
  }

  return 0;
}

plAudioSystemDataID plAudioTranslationLayer::GetRtpcId(plStringView sRtpcName) const
{
  const auto uiRtpcId = plHashHelper<plStringView>::Hash(sRtpcName);

  if (const auto it = m_mRtpcs.Find(uiRtpcId); it.IsValid())
  {
    return uiRtpcId;
  }

  return 0;
}

plAudioSystemDataID plAudioTranslationLayer::GetSwitchStateId(plStringView sSwitchStateName) const
{
  const auto uiSwitchStateId = plHashHelper<plStringView>::Hash(sSwitchStateName);

  if (const auto it = m_mSwitchStates.Find(uiSwitchStateId); it.IsValid())
  {
    return uiSwitchStateId;
  }

  return 0;
}

plAudioSystemDataID plAudioTranslationLayer::GetEnvironmentId(plStringView sEnvironmentName) const
{
  const auto uiEnvironmentId = plHashHelper<plStringView>::Hash(sEnvironmentName);

  if (const auto it = m_mEnvironments.Find(uiEnvironmentId); it.IsValid())
  {
    return uiEnvironmentId;
  }

  return 0;
}

bool plAudioTranslationLayer::ProcessRequest(plVariant&& request, bool bSync)
{
  if (m_pAudioMiddleware == nullptr)
    return false;

  bool needCallback = false;

  if (request.IsA<plAudioSystemRequestRegisterEntity>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestRegisterEntity>();
    plAudioSystemEntityData* entity = m_pAudioMiddleware->CreateEntityData(audioRequest.m_uiEntityId);

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (entity == nullptr)
    {
      plLog::Error("Failed to create entity data for entity {0}", audioRequest.m_uiEntityId);
      return needCallback;
    }

    m_mEntities[audioRequest.m_uiEntityId] = PLASMA_AUDIOSYSTEM_NEW(plATLEntity, audioRequest.m_uiEntityId, entity);
    audioRequest.m_eStatus = m_pAudioMiddleware->AddEntity(entity, audioRequest.m_sName);
  }

  else if (request.IsA<plAudioSystemRequestSetEntityTransform>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetEntityTransform>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to set entity transform {0}. Make sure it was registered before.", audioRequest.m_uiEntityId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    audioRequest.m_eStatus = m_pAudioMiddleware->SetEntityTransform(entity->m_pEntityData, audioRequest.m_Transform);
  }

  else if (request.IsA<plAudioSystemRequestUnregisterEntity>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestUnregisterEntity>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to unregister entity {0}. Make sure it was registered before.", audioRequest.m_uiEntityId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    m_pAudioMiddleware->RemoveEntity(entity->m_pEntityData).IgnoreResult();
    audioRequest.m_eStatus = m_pAudioMiddleware->DestroyEntityData(entity->m_pEntityData);

    if (audioRequest.m_eStatus.Succeeded())
    {
      PLASMA_AUDIOSYSTEM_DELETE(m_mEntities[audioRequest.m_uiEntityId]);
      m_mEntities.Remove(audioRequest.m_uiEntityId);
    }
  }

  else if (request.IsA<plAudioSystemRequestRegisterListener>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestRegisterListener>();
    plAudioSystemListenerData* pListenerData = m_pAudioMiddleware->CreateListenerData(audioRequest.m_uiListenerId);

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (pListenerData == nullptr)
    {
      plLog::Error("Failed to create listener data for listener {0}", audioRequest.m_uiListenerId);
      return needCallback;
    }

    m_mListeners[audioRequest.m_uiListenerId] = PLASMA_AUDIOSYSTEM_NEW(plATLListener, audioRequest.m_uiListenerId, pListenerData);
    audioRequest.m_eStatus = m_pAudioMiddleware->AddListener(pListenerData, audioRequest.m_sName);
  }

  else if (request.IsA<plAudioSystemRequestSetListenerTransform>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetListenerTransform>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mListeners.Contains(audioRequest.m_uiListenerId))
    {
      plLog::Error("Failed to set listener transform {0}. Make sure it was registered before.", audioRequest.m_uiListenerId);
      return needCallback;
    }

    const auto& listener = m_mListeners[audioRequest.m_uiListenerId];
    audioRequest.m_eStatus = m_pAudioMiddleware->SetListenerTransform(listener->m_pListenerData, audioRequest.m_Transform);
  }

  else if (request.IsA<plAudioSystemRequestUnregisterListener>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestUnregisterListener>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mListeners.Contains(audioRequest.m_uiListenerId))
    {
      plLog::Error("Failed to unregister listener {0}. Make sure it was registered before.", audioRequest.m_uiListenerId);
      return needCallback;
    }

    const auto& listener = m_mListeners[audioRequest.m_uiListenerId];
    m_pAudioMiddleware->RemoveListener(listener->m_pListenerData).IgnoreResult();
    audioRequest.m_eStatus = m_pAudioMiddleware->DestroyListenerData(listener->m_pListenerData);

    if (audioRequest.m_eStatus.Succeeded())
    {
      PLASMA_AUDIOSYSTEM_DELETE(m_mListeners[audioRequest.m_uiListenerId]);
      m_mListeners.Remove(audioRequest.m_uiListenerId);
    }
  }

  else if (request.IsA<plAudioSystemRequestLoadTrigger>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestLoadTrigger>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to load trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to load trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiObjectId];

    plAudioSystemEventData* pEventData = nullptr;
    if (trigger->GetEvent(audioRequest.m_uiEventId, pEventData).Failed())
    {
      pEventData = m_pAudioMiddleware->CreateEventData(audioRequest.m_uiEventId);

      if (pEventData == nullptr)
      {
        plLog::Error("Failed to load trigger {0}. Unable to allocate memory for the linked event with ID {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEventId);
        return needCallback;
      }

      trigger->AttachEvent(audioRequest.m_uiEventId, pEventData);
    }

    audioRequest.m_eStatus = m_pAudioMiddleware->LoadTrigger(entity->m_pEntityData, trigger->m_pTriggerData, pEventData);
  }

  else if (request.IsA<plAudioSystemRequestActivateTrigger>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestActivateTrigger>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to activate trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to activate trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiObjectId];

    plAudioSystemEventData* pEventData = nullptr;
    if (trigger->GetEvent(audioRequest.m_uiEventId, pEventData).Failed())
    {
      plLog::Error("Failed to activate trigger {0}. Make sure to load the trigger before to activate it.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    audioRequest.m_eStatus = m_pAudioMiddleware->ActivateTrigger(entity->m_pEntityData, trigger->m_pTriggerData, pEventData);
  }

  else if (request.IsA<plAudioSystemRequestStopEvent>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestStopEvent>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to stop trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiTriggerId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiTriggerId))
    {
      plLog::Error("Failed to stop trigger {0}. Make sure it was registered before.", audioRequest.m_uiTriggerId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiTriggerId];

    plAudioSystemEventData* pEventData = nullptr;
    if (trigger->GetEvent(audioRequest.m_uiObjectId, pEventData).Failed())
    {
      plLog::Error("Failed to stop trigger {0}. Make sure to load the trigger before to stop it.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    audioRequest.m_eStatus = m_pAudioMiddleware->StopEvent(entity->m_pEntityData, pEventData);
  }

  else if (request.IsA<plAudioSystemRequestUnloadTrigger>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestUnloadTrigger>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to unload the trigger {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mTriggers.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to stop trigger {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& trigger = m_mTriggers[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->UnloadTrigger(entity->m_pEntityData, trigger->m_pTriggerData);
  }

  else if (request.IsA<plAudioSystemRequestSetRtpcValue>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetRtpcValue>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to set the rtpc {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mRtpcs.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to set rtpc {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& rtpc = m_mRtpcs[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->SetRtpc(entity->m_pEntityData, rtpc->m_pRtpcData, audioRequest.m_fValue);
  }

  else if (request.IsA<plAudioSystemRequestSetSwitchState>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetSwitchState>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to set the switch state {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mSwitchStates.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to set switch state {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& switchState = m_mSwitchStates[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->SetSwitchState(entity->m_pEntityData, switchState->m_pSwitchStateData);
  }

  else if (request.IsA<plAudioSystemRequestSetEnvironmentAmount>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetEnvironmentAmount>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to set amount for environment {0}. It references an unregistered entity {1}.", audioRequest.m_uiObjectId, audioRequest.m_uiEntityId);
      return needCallback;
    }

    if (!m_mEnvironments.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to set amount for environment {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];
    const auto& environment = m_mEnvironments[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->SetEnvironmentAmount(entity->m_pEntityData, environment->m_pEnvironmentData, audioRequest.m_fAmount);
  }

  else if (request.IsA<plAudioSystemRequestSetObstructionOcclusion>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestSetObstructionOcclusion>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mEntities.Contains(audioRequest.m_uiEntityId))
    {
      plLog::Error("Failed to set obstruction and occlusion values. It references an unregistered entity {0}.", audioRequest.m_uiEntityId);
      return needCallback;
    }

    const auto& entity = m_mEntities[audioRequest.m_uiEntityId];

    audioRequest.m_eStatus = m_pAudioMiddleware->SetObstructionAndOcclusion(entity->m_pEntityData, audioRequest.m_fObstruction, audioRequest.m_fOcclusion);
  }

  else if (request.IsA<plAudioSystemRequestLoadBank>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestLoadBank>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mSoundBanks.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to load sound bank {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& bank = m_mSoundBanks[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->LoadBank(bank->m_pSoundBankData);
  }

  else if (request.IsA<plAudioSystemRequestUnloadBank>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestUnloadBank>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    if (!m_mSoundBanks.Contains(audioRequest.m_uiObjectId))
    {
      plLog::Error("Failed to unload sound bank {0}. Make sure it was registered before.", audioRequest.m_uiObjectId);
      return needCallback;
    }

    const auto& bank = m_mSoundBanks[audioRequest.m_uiObjectId];

    audioRequest.m_eStatus = m_pAudioMiddleware->UnloadBank(bank->m_pSoundBankData);
  }

  else if (request.IsA<plAudioSystemRequestShutdown>())
  {
    auto& audioRequest = request.GetWritable<plAudioSystemRequestShutdown>();

    audioRequest.m_eStatus = {PLASMA_FAILURE};
    needCallback = audioRequest.m_Callback.IsValid();

    // Destroy sound banks
    for (auto&& bank : m_mSoundBanks)
    {
      m_pAudioMiddleware->DestroyBank(bank.Value()->m_pSoundBankData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(bank.Value());
    }

    // Destroy environments
    for (auto&& environment : m_mEnvironments)
    {
      m_pAudioMiddleware->DestroyEnvironmentData(environment.Value()->m_pEnvironmentData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(environment.Value());
    }

    // Destroy switch states
    for (auto&& switchState : m_mSwitchStates)
    {
      m_pAudioMiddleware->DestroySwitchStateData(switchState.Value()->m_pSwitchStateData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(switchState.Value());
    }

    // Destroy rtpcs
    for (auto&& rtpc : m_mRtpcs)
    {
      m_pAudioMiddleware->DestroyRtpcData(rtpc.Value()->m_pRtpcData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(rtpc.Value());
    }

    // Destroy triggers
    for (auto&& trigger : m_mTriggers)
    {
      m_pAudioMiddleware->DestroyTriggerData(trigger.Value()->m_pTriggerData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(trigger.Value());
    }

    // Destroy listeners
    for (auto&& listener : m_mListeners)
    {
      m_pAudioMiddleware->DestroyListenerData(listener.Value()->m_pListenerData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(listener.Value());
    }

    // Destroy entities
    for (auto&& entity : m_mEntities)
    {
      m_pAudioMiddleware->DestroyEntityData(entity.Value()->m_pEntityData).IgnoreResult();
      PLASMA_AUDIOSYSTEM_DELETE(entity.Value());
    }

    audioRequest.m_eStatus = {PLASMA_SUCCESS};
  }

  if (needCallback)
  {
    plAudioSystem::GetSingleton()->QueueRequestCallback(std::move(request), bSync);
  }

  return needCallback;
}

void plAudioTranslationLayer::OnMasterGainChange(const plCVarEvent& e) const
{
  if (e.m_EventType == plCVarEvent::Type::ValueChanged && m_pAudioMiddleware != nullptr)
  {
    m_pAudioMiddleware->OnMasterGainChange(static_cast<plCVarFloat*>(e.m_pCVar)->GetValue());
  }
}

void plAudioTranslationLayer::OnMuteChange(const plCVarEvent& e) const
{
  if (e.m_EventType == plCVarEvent::Type::ValueChanged && m_pAudioMiddleware != nullptr)
  {
    m_pAudioMiddleware->OnMuteChange(static_cast<plCVarBool*>(e.m_pCVar)->GetValue());
  }
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
void plAudioTranslationLayer::DebugRender() const
{
  static plTime tAccumTime;
  static plTime tDisplayedFrameTime = m_LastFrameTime;
  static plUInt32 uiFrames = 0;
  static plUInt32 uiFPS = 0;

  ++uiFrames;
  tAccumTime += m_LastFrameTime;

  if (tAccumTime >= plTime::Seconds(1.0))
  {
    tAccumTime -= plTime::Seconds(1.0);
    tDisplayedFrameTime = m_LastFrameTime;

    uiFPS = uiFrames;
    uiFrames = 0;
  }

  if (cvar_AudioSystemDebug)
  {
    const auto* pAudioMiddleware = plSingletonRegistry::GetSingletonInstance<plAudioMiddleware>();

    if (pAudioMiddleware == nullptr)
      return;

    if (const plView* pView = plRenderWorld::GetViewByUsageHint(plCameraUsageHint::MainView, plCameraUsageHint::EditorView))
    {
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::BottomRight, "AudioSystem", plFmt("ATL ({0}) - {1} fps, {2} ms", pAudioMiddleware->GetMiddlewareName(), uiFPS, plArgF(tDisplayedFrameTime.GetMilliseconds(), 1, false, 4)));
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::BottomRight, "AudioSystem", plFmt("Entities Count: {0}", m_mEntities.GetCount()));
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::BottomRight, "AudioSystem", plFmt("Listeners Count: {0}", m_mListeners.GetCount()));
      plDebugRenderer::DrawInfoText(pView->GetHandle(), plDebugTextPlacement::BottomRight, "AudioSystem", plFmt("Total Allocated Memory: {0}Mb", (plAudioSystemAllocator::GetSingleton()->GetStats().m_uiAllocationSize + plAudioMiddlewareAllocator::GetSingleton()->GetStats().m_uiAllocationSize) / 1048576.0f));
    }
  }
}
#endif

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_ATL_AudioTranslationLayer);
