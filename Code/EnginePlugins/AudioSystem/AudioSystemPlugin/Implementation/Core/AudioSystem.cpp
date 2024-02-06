#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioMiddleware.h>
#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Foundation/Configuration/CVar.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Time/Clock.h>

constexpr plAudioSystemDataID kEditorListenerId = 1; // PLASMA will send -1 as ID in override mode, but we use 1 internally
constexpr plAudioSystemDataID kEditorSoundEventId = 1;

plThreadID gMainThreadId = static_cast<plThreadID>(0);

plCVarFloat cvar_AudioSystemFPS("Audio.FPS", 60, plCVarFlags::Save, "The maximum number of frames to process within one second in the audio system.");

// clang-format off
PL_IMPLEMENT_SINGLETON(plAudioSystem);
// clang-format on

void plAudioSystem::LoadConfiguration(plStringView sFile)
{
  plFileReader file;
  if (file.Open(sFile).Failed())
    return;

  plOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return;

  const plOpenDdlReaderElement* pRoot = ddl.GetRootElement();
  const plOpenDdlReaderElement* pChild = pRoot->GetFirstChild();

  while (pChild)
  {
    if (pChild->IsCustomType("Middleware") && pChild->HasName() && pChild->GetName().Compare(m_AudioTranslationLayer.m_pAudioMiddleware->GetMiddlewareName()) == 0)
    {
      plLog::Debug("Loading audio middleware configuration for {}...", pChild->GetName());

      if (m_AudioTranslationLayer.m_pAudioMiddleware->LoadConfiguration(*pChild).Failed())
        plLog::Error("Failed to load configuration for audio middleware: {0}.", pChild->GetName());

      plLog::Success("Audio middleware configuration for {} successfully loaded.", pChild->GetName());
      break;
    }

    pChild = pChild->GetSibling();
  }
}

void plAudioSystem::SetOverridePlatform(plStringView sPlatform)
{
}

void plAudioSystem::UpdateSound()
{
  // TODO: Seems that this function is not called from the thread that has started the audio system. It is a bit obvious, but have to check later if this assert is relevant...
  // PL_ASSERT_ALWAYS(gMainThreadId == plThreadUtils::GetCurrentThreadID(), "AudioSystem::UpdateSound not called from main thread.");

  if (!m_bInitialized)
    return;

  // Process a single synchronous request callback, if any
  bool handleBlockingRequest = false;
  plVariant blockingRequest;

  {
    PL_LOCK(m_BlockingRequestCallbacksMutex);
    handleBlockingRequest = !m_BlockingRequestCallbacksQueue.IsEmpty();
    if (handleBlockingRequest)
    {
      blockingRequest = std::move(m_BlockingRequestCallbacksQueue.PeekFront());
      m_BlockingRequestCallbacksQueue.PopFront();
    }
  }

  if (handleBlockingRequest)
  {
    CallRequestCallbackFunc func(blockingRequest);
    plVariant::DispatchTo(func, blockingRequest.GetType());

    m_ProcessingEvent.ReturnToken();
  }

  if (!handleBlockingRequest)
  {
    // Process asynchronous callbacks
    plAudioSystemRequestsQueue callbacks{};
    {
      PL_LOCK(m_PendingRequestCallbacksMutex);
      callbacks.Swap(m_PendingRequestCallbacksQueue);
    }

    while (!callbacks.IsEmpty())
    {
      plVariant callback(callbacks.PeekFront());

      CallRequestCallbackFunc func(callback);

      plVariant::DispatchTo(func, callback.GetType());

      callbacks.PopFront();
    }
  }

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  m_AudioTranslationLayer.DebugRender();
#endif
}

void plAudioSystem::SetMasterChannelVolume(float fVolume)
{
  m_AudioTranslationLayer.m_pAudioMiddleware->OnMasterGainChange(fVolume);
}

float plAudioSystem::GetMasterChannelVolume() const
{
  return m_AudioTranslationLayer.m_pAudioMiddleware->GetMasterGain();
}

void plAudioSystem::SetMasterChannelMute(bool bMute)
{
  m_AudioTranslationLayer.m_pAudioMiddleware->OnMuteChange(bMute);
}

bool plAudioSystem::GetMasterChannelMute() const
{
  return m_AudioTranslationLayer.m_pAudioMiddleware->GetMute();
}

void plAudioSystem::SetMasterChannelPaused(bool bPaused)
{
}

bool plAudioSystem::GetMasterChannelPaused() const
{
  return false;
}

void plAudioSystem::SetSoundGroupVolume(plStringView sVcaGroupGuid, float fVolume)
{
}

float plAudioSystem::GetSoundGroupVolume(plStringView sVcaGroupGuid) const
{
  return 0.0f;
}

plUInt8 plAudioSystem::GetNumListeners()
{
  return 0;
}

void plAudioSystem::SetListenerOverrideMode(bool bEnabled)
{
  m_bListenerOverrideMode = bEnabled;
}

void plAudioSystem::SetListener(plInt32 iIndex, const plVec3& vPosition, const plVec3& vForward, const plVec3& vUp, const plVec3& vVelocity)
{
  plAudioSystemRequestSetListenerTransform request;

  // Index is -1 when inside the editor, listener is overriden when simulating. Both of them seems to mean the listener is the editor camera. Need to be sure about that...
  request.m_uiListenerId = m_bListenerOverrideMode || iIndex == -1 ? kEditorListenerId : static_cast<plAudioSystemDataID>(iIndex);
  request.m_Transform.m_vPosition = vPosition;
  request.m_Transform.m_vForward = vForward;
  request.m_Transform.m_vUp = vUp;
  request.m_Transform.m_vVelocity = vVelocity;

  if (m_bListenerOverrideMode)
  {
    // Editor mode
    SendRequestSync(request);
  }
  else
  {
    SendRequest(request);
  }
}

plResult plAudioSystem::OneShotSound(plStringView sResourceID, const plTransform& globalPosition, float fPitch, float fVolume, bool bBlockIfNotLoaded)
{
  plAudioSystemRequestActivateTrigger request;

  request.m_uiEntityId = 1; // Send the sound to the global entity
  request.m_uiObjectId = GetTriggerId(sResourceID);
  request.m_uiEventId = kEditorSoundEventId;

  plResult res = PL_FAILURE;
  request.m_Callback = [&res](const plAudioSystemRequestActivateTrigger& e)
  {
    res = e.m_eStatus.m_Result;
  };

  SendRequestSync(request);
  return res;
}

plAudioSystem::plAudioSystem()
  : m_SingletonRegistrar(this)
  , m_bInitialized(false)
  , m_bListenerOverrideMode(false)
{
  gMainThreadId = plThreadUtils::GetCurrentThreadID();
}

plAudioSystem::~plAudioSystem()
{
  PL_ASSERT_DEV(!m_bInitialized, "You should shutdown the AudioSystem before to call the dtor.");
}

bool plAudioSystem::Startup()
{
  PL_ASSERT_ALWAYS(gMainThreadId == plThreadUtils::GetCurrentThreadID(), "AudioSystem::Startup not called from main thread.");

  if (m_bInitialized)
    return true;

  const auto* pAudioMiddleware = plSingletonRegistry::GetSingletonInstance<plAudioMiddleware>();
  if (pAudioMiddleware == nullptr)
  {
    plLog::Error("Could not find an active audio middleware. The AudioSystem will not start.");
    return false;
  }

  StopAudioThread();

  if (m_MainEvent.Create().Succeeded() && m_ProcessingEvent.Create().Succeeded() && m_AudioTranslationLayer.Startup().Succeeded())
  {
    // Start audio thread
    StartAudioThread();
    m_bInitialized = true;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    // Register the default (editor) listener
    plAudioSystemRequestRegisterListener request;

    request.m_uiListenerId = kEditorListenerId;
    request.m_sName = "Editor Listener";

    SendRequestSync(request);
#endif
  }

  return m_bInitialized;
}

void plAudioSystem::Shutdown()
{
  PL_ASSERT_ALWAYS(gMainThreadId == plThreadUtils::GetCurrentThreadID(), "AudioSystem::Shutdown not called from main thread.");

  if (!m_bInitialized)
    return;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  // Unregister the default (editor) listener
  plAudioSystemRequestUnregisterListener request;

  request.m_uiListenerId = kEditorListenerId;

  SendRequestSync(request);
#endif

  plAudioSystemRequestShutdown shutdownRequest;
  SendRequestSync(shutdownRequest);

  StopAudioThread();
  m_AudioTranslationLayer.Shutdown();
  m_bInitialized = false;
}

bool plAudioSystem::IsInitialized() const
{
  return m_bInitialized;
}

void plAudioSystem::SendRequest(plVariant&& request)
{
  if (!m_bInitialized)
    return;

  PL_LOCK(m_PendingRequestsMutex);
  m_PendingRequestsQueue.PushBack(std::move(request));
}

void plAudioSystem::SendRequests(plAudioSystemRequestsQueue& requests)
{
  if (!m_bInitialized)
    return;

  PL_LOCK(m_PendingRequestsMutex);
  for (auto& request : requests)
  {
    m_PendingRequestsQueue.PushBack(std::move(request));
  }
}

void plAudioSystem::SendRequestSync(plVariant&& request)
{
  if (!m_bInitialized)
    return;

  {
    PL_LOCK(m_BlockingRequestsMutex);
    m_BlockingRequestsQueue.PushBack(std::move(request));
  }

  m_ProcessingEvent.ReturnToken();
  m_MainEvent.AcquireToken();
}

void plAudioSystem::QueueRequestCallback(plVariant&& request, bool bSync)
{
  if (!m_bInitialized)
    return;

  if (bSync)
  {
    PL_LOCK(m_BlockingRequestCallbacksMutex);
    m_BlockingRequestCallbacksQueue.PushBack(std::move(request));
  }
  else
  {
    PL_LOCK(m_PendingRequestCallbacksMutex);
    m_PendingRequestCallbacksQueue.PushBack(std::move(request));
  }
}

plAudioSystemDataID plAudioSystem::GetTriggerId(plStringView sTriggerName) const
{
  return m_AudioTranslationLayer.GetTriggerId(sTriggerName);
}

plAudioSystemDataID plAudioSystem::GetRtpcId(plStringView sRtpcName) const
{
  return m_AudioTranslationLayer.GetRtpcId(sRtpcName);
}

plAudioSystemDataID plAudioSystem::GetSwitchStateId(plStringView sSwitchStateName) const
{
  return m_AudioTranslationLayer.GetSwitchStateId(sSwitchStateName);
}

plAudioSystemDataID plAudioSystem::GetEnvironmentId(plStringView sEnvironmentName) const
{
  return m_AudioTranslationLayer.GetEnvironmentId(sEnvironmentName);
}

plAudioSystemDataID plAudioSystem::GetBankId(plStringView sBankName) const
{
  return 0;
}

void plAudioSystem::RegisterTrigger(plAudioSystemDataID uiId, plAudioSystemTriggerData* pTriggerData)
{
  if (m_AudioTranslationLayer.m_mTriggers.Contains(uiId))
  {
    plLog::Warning("ATL: Trigger with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  m_AudioTranslationLayer.m_mTriggers[uiId] = PL_AUDIOSYSTEM_NEW(plATLTrigger, uiId, pTriggerData);
}

void plAudioSystem::RegisterRtpc(plAudioSystemDataID uiId, plAudioSystemRtpcData* pRtpcData)
{
  if (m_AudioTranslationLayer.m_mRtpcs.Contains(uiId))
  {
    plLog::Warning("ATL: Rtpc with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  m_AudioTranslationLayer.m_mRtpcs[uiId] = PL_AUDIOSYSTEM_NEW(plATLRtpc, uiId, pRtpcData);
}

void plAudioSystem::RegisterSwitchState(plAudioSystemDataID uiId, plAudioSystemSwitchStateData* pSwitchStateData)
{
  if (m_AudioTranslationLayer.m_mSwitchStates.Contains(uiId))
  {
    plLog::Warning("ATL: Switch state with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  m_AudioTranslationLayer.m_mSwitchStates[uiId] = PL_AUDIOSYSTEM_NEW(plATLSwitchState, uiId, pSwitchStateData);
}

void plAudioSystem::RegisterEnvironment(plAudioSystemDataID uiId, plAudioSystemEnvironmentData* pEnvironmentData)
{
  if (m_AudioTranslationLayer.m_mEnvironments.Contains(uiId))
  {
    plLog::Warning("ATL: Environment with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  m_AudioTranslationLayer.m_mEnvironments[uiId] = PL_AUDIOSYSTEM_NEW(plATLEnvironment, uiId, pEnvironmentData);
}

void plAudioSystem::RegisterSoundBank(plAudioSystemDataID uiId, plAudioSystemBankData* pSoundBankData)
{
  if (m_AudioTranslationLayer.m_mSoundBanks.Contains(uiId))
  {
    plLog::Warning("ATL: Sound bank with id {0} already exists. Skipping new registration.", uiId);
    return;
  }

  m_AudioTranslationLayer.m_mSoundBanks[uiId] = PL_AUDIOSYSTEM_NEW(plATLSoundBank, uiId, pSoundBankData);
}

void plAudioSystem::UnregisterEntity(const plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mEntities.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mEntities[uiId]);
  m_AudioTranslationLayer.m_mEntities.Remove(uiId);
}

void plAudioSystem::UnregisterListener(const plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mListeners.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mListeners[uiId]);
  m_AudioTranslationLayer.m_mListeners.Remove(uiId);
}

void plAudioSystem::UnregisterTrigger(const plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mTriggers.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mTriggers[uiId]);
  m_AudioTranslationLayer.m_mTriggers.Remove(uiId);
}

void plAudioSystem::UnregisterRtpc(const plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mRtpcs.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mRtpcs[uiId]);
  m_AudioTranslationLayer.m_mRtpcs.Remove(uiId);
}

void plAudioSystem::UnregisterSwitchState(plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mSwitchStates.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mSwitchStates[uiId]);
  m_AudioTranslationLayer.m_mSwitchStates.Remove(uiId);
}

void plAudioSystem::UnregisterEnvironment(plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mEnvironments.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mEnvironments[uiId]);
  m_AudioTranslationLayer.m_mEnvironments.Remove(uiId);
}

void plAudioSystem::UnregisterSoundBank(plAudioSystemDataID uiId)
{
  if (!m_AudioTranslationLayer.m_mSoundBanks.Contains(uiId))
    return;

  PL_AUDIOSYSTEM_DELETE(m_AudioTranslationLayer.m_mSoundBanks[uiId]);
  m_AudioTranslationLayer.m_mSoundBanks.Remove(uiId);
}

void plAudioSystem::UpdateInternal()
{
  PL_ASSERT_ALWAYS(m_pAudioThread->GetThreadID() == plThreadUtils::GetCurrentThreadID(), "AudioSystem::UpdateInternal not called from audio thread.");

  PL_PROFILE_SCOPE("AudioSystem");

  if (!m_bInitialized)
    return;

  const plTime startTime = plTime::Now();

  // Process a single synchronous request, if any
  bool handleBlockingRequest = false;
  plVariant blockingRequest;

  {
    PL_LOCK(m_BlockingRequestsMutex);
    handleBlockingRequest = !m_BlockingRequestsQueue.IsEmpty();
    if (handleBlockingRequest)
    {
      blockingRequest = std::move(m_BlockingRequestsQueue.PeekFront());
      m_BlockingRequestsQueue.PopFront();
    }
  }

  if (handleBlockingRequest)
  {
    const bool needCallback = m_AudioTranslationLayer.ProcessRequest(std::move(blockingRequest), true);
    m_MainEvent.ReturnToken();

    // If a callback is found, wait for it to be executed
    if (needCallback)
      m_ProcessingEvent.AcquireToken();
  }

  if (!handleBlockingRequest)
  {
    // Normal request processing: lock and swap the pending requests queue
    // so that the queue can be opened for new requests while the current set
    // of requests can be processed.
    plAudioSystemRequestsQueue requestsToProcess{};
    {
      PL_LOCK(m_PendingRequestsMutex);
      requestsToProcess.Swap(m_PendingRequestsQueue);
    }

    while (!requestsToProcess.IsEmpty())
    {
      // Normal request...
      plVariant& request(requestsToProcess.PeekFront());
      m_AudioTranslationLayer.ProcessRequest(std::move(request), false);
      requestsToProcess.PopFront();
    }
  }

  m_AudioTranslationLayer.Update();

  if (!handleBlockingRequest)
  {
    const plTime endTime = plTime::Now(); // stamp the end time
    const plTime elapsedTime = endTime - startTime;

    if (const plTime frameTime = plTime::Seconds(1.0f / cvar_AudioSystemFPS); frameTime > elapsedTime)
    {
      m_ProcessingEvent.TryAcquireToken().IgnoreResult();
    }
  }
}

void plAudioSystem::StartAudioThread()
{
  StopAudioThread();

  if (m_pAudioThread == nullptr)
  {
    m_pAudioThread = PL_AUDIOSYSTEM_NEW(plAudioThread);
    m_pAudioThread->m_pAudioSystem = this;
    m_pAudioThread->Start();

    plLog::Success("Audio thread started.");
  }
}

void plAudioSystem::StopAudioThread()
{
  if (m_pAudioThread != nullptr)
  {
    m_pAudioThread->m_bKeepRunning = false;
    m_pAudioThread->Join();

    PL_AUDIOSYSTEM_DELETE(m_pAudioThread);

    plLog::Success("Audio thread stopped.");
  }
}

void plAudioSystem::GameApplicationEventHandler(const plGameApplicationExecutionEvent& e)
{
  plAudioSystem* pAudioSystem = GetSingleton();

  if (pAudioSystem == nullptr || !pAudioSystem->m_bInitialized)
    return;

  if (e.m_Type == plGameApplicationExecutionEvent::Type::AfterWorldUpdates)
  {
    pAudioSystem->UpdateSound();
  }
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioSystem);
