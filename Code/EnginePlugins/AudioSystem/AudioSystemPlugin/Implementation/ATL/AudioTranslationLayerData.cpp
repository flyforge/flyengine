#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/ATL/AudioTranslationLayerData.h>
#include <AudioSystemPlugin/Core/AudioMiddleware.h>

plATLTrigger::~plATLTrigger()
{
  if (m_mEvents.IsEmpty() == false)
  {
    auto* pAudioMiddleware = plSingletonRegistry::GetSingletonInstance<plAudioMiddleware>();

    for (auto it = m_mEvents.GetIterator(); it.IsValid(); ++it)
    {
      plATLEvent* event = it.Value();

      if (pAudioMiddleware != nullptr)
      {
        pAudioMiddleware->DestroyEventData(event->m_pEventData).IgnoreResult();
      }

      PL_AUDIOSYSTEM_DELETE(event);
    }

    m_mEvents.Clear();
  }
}

void plATLTrigger::AttachEvent(const plAudioSystemDataID uiEventId, plAudioSystemEventData* const pEventData)
{
  m_mEvents[uiEventId] = PL_AUDIOSYSTEM_NEW(plATLEvent, uiEventId, pEventData);
}

void plATLTrigger::DetachEvent(const plAudioSystemDataID uiEventId)
{
  PL_AUDIOSYSTEM_DELETE(m_mEvents[uiEventId]);
  m_mEvents.Remove(uiEventId);
}

plResult plATLTrigger::GetEvent(const plAudioSystemDataID uiEventId, plAudioSystemEventData*& out_pEventData) const
{
  if (!m_mEvents.Contains(uiEventId))
    return PL_FAILURE;

  out_pEventData = (*m_mEvents.GetValue(uiEventId))->m_pEventData;
  return PL_SUCCESS;
}
