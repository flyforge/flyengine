#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

void plLongOpManager::Startup(plProcessCommunicationChannel* pCommunicationChannel)
{
  m_pCommunicationChannel = pCommunicationChannel;
  m_pCommunicationChannel->m_Events.AddEventHandler(plMakeDelegate(&plLongOpManager::ProcessCommunicationChannelEventHandler, this), m_Unsubscriber);
}

void plLongOpManager::Shutdown()
{
  PL_LOCK(m_Mutex);

  m_Unsubscriber.Unsubscribe();
  m_pCommunicationChannel = nullptr;
}
