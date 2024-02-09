#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>


template <typename T>
using plATLMapLookup = plMap<plAudioSystemDataID, T*, plCompareHelper<plAudioSystemDataID>, plAudioSystemAllocatorWrapper>;

using plATLEntityLookup = plATLMapLookup<class plATLEntity>;
using plATLListenerLookup = plATLMapLookup<class plATLListener>;
using plATLTriggerLookup = plATLMapLookup<class plATLTrigger>;
using plATLEventLookup = plATLMapLookup<class plATLEvent>;
using plATLRtpcLookup = plATLMapLookup<class plATLRtpc>;
using plATLSwitchStateLookup = plATLMapLookup<class plATLSwitchState>;
using plATLEnvironmentLookup = plATLMapLookup<class plATLEnvironment>;
using plATLSoundBankLookup = plATLMapLookup<class plATLSoundBank>;

class plATLControl
{
public:
  explicit plATLControl(const plAudioSystemDataID uiId)
    : m_uiId(uiId)
  {
  }

  virtual ~plATLControl() = default;

  PL_NODISCARD virtual plAudioSystemDataID GetId() const { return m_uiId; }

private:
  const plAudioSystemDataID m_uiId;
};

class plATLListener
  : public plATLControl
{
public:
  explicit plATLListener(const plAudioSystemDataID uiId, plAudioSystemListenerData* const pListenerData = nullptr)
    : plATLControl(uiId)
    , m_pListenerData(pListenerData)
  {
  }

  ~plATLListener() override = default;

  plAudioSystemListenerData* const m_pListenerData;
};

class plATLEntity final
  : public plATLControl
{
public:
  explicit plATLEntity(const plAudioSystemDataID uiId, plAudioSystemEntityData* const pEntityData = nullptr)
    : plATLControl(uiId)
    , m_pEntityData(pEntityData)
  {
  }

  ~plATLEntity() override = default;

  plAudioSystemEntityData* const m_pEntityData;
};

class plATLTrigger final
  : public plATLControl
{
public:
  explicit plATLTrigger(const plAudioSystemDataID uiId, plAudioSystemTriggerData* const pTriggerData = nullptr)
    : plATLControl(uiId)
    , m_pTriggerData(pTriggerData)
  {
  }

  ~plATLTrigger() override;

  /// \brief Attach an event to this trigger. This means the attached event has been triggered by this trigger.
  /// This will ensure that every event will be destroyed when the trigger is destroyed.
  void AttachEvent(plAudioSystemDataID uiEventId, plAudioSystemEventData* pEventData);

  /// \brief Detach an event to this trigger. This will also destroy the event.
  void DetachEvent(plAudioSystemDataID uiEventId);

  /// \brief Get an attached event. This will fail and return nullptr if an event with the given ID is not
  /// attached to this trigger.
  PL_NODISCARD plResult GetEvent(plAudioSystemDataID uiEventId, plAudioSystemEventData*& out_pEventData) const;

  plAudioSystemTriggerData* const m_pTriggerData;

private:
  plATLEventLookup m_mEvents;
};

class plATLEvent final
  : public plATLControl
{
public:
  explicit plATLEvent(const plAudioSystemDataID uiId, plAudioSystemEventData* const pEventData = nullptr)
    : plATLControl(uiId)
    , m_pEventData(pEventData)
  {
  }

  ~plATLEvent() override = default;

  plAudioSystemEventData* const m_pEventData;
};

class plATLRtpc final
  : public plATLControl
{
public:
  explicit plATLRtpc(const plAudioSystemDataID uiId, plAudioSystemRtpcData* const pRtpcData = nullptr)
    : plATLControl(uiId)
    , m_pRtpcData(pRtpcData)
  {
  }

  ~plATLRtpc() override = default;

  plAudioSystemRtpcData* const m_pRtpcData;
};

class plATLSwitchState final
  : public plATLControl
{
public:
  explicit plATLSwitchState(const plAudioSystemDataID uiId, plAudioSystemSwitchStateData* const pSwitchStateData = nullptr)
    : plATLControl(uiId)
    , m_pSwitchStateData(pSwitchStateData)
  {
  }

  ~plATLSwitchState() override = default;

  plAudioSystemSwitchStateData* const m_pSwitchStateData;
};

class plATLEnvironment final
  : public plATLControl
{
public:
  explicit plATLEnvironment(const plAudioSystemDataID uiId, plAudioSystemEnvironmentData* const pEnvironmentData = nullptr)
    : plATLControl(uiId)
    , m_pEnvironmentData(pEnvironmentData)
  {
  }

  ~plATLEnvironment() override = default;

  plAudioSystemEnvironmentData* const m_pEnvironmentData;
};

class plATLSoundBank final
  : public plATLControl
{
public:
  explicit plATLSoundBank(const plAudioSystemDataID uiId, plAudioSystemBankData* const pSoundBankData = nullptr)
    : plATLControl(uiId)
    , m_pSoundBankData(pSoundBankData)
  {
  }

  ~plATLSoundBank() override = default;

  plAudioSystemBankData* const m_pSoundBankData;
};
