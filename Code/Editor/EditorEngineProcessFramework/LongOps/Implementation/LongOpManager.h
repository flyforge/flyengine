#pragma once

#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkDLL.h>

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <Foundation/Types/Uuid.h>

/// \brief Base class with shared functionality for plLongOpControllerManager and plLongOpWorkerManager
class PLASMA_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpManager
{
public:
  /// \brief Needs to be called early to initialize the IPC channel to use.
  void Startup(plProcessCommunicationChannel* pCommunicationChannel);

  /// \brief Call this to shut down the IPC communication.
  void Shutdown();

  /// \brief Publicly exposed mutex for some special cases.
  mutable plMutex m_Mutex;

protected:
  virtual void ProcessCommunicationChannelEventHandler(const plProcessCommunicationChannel::Event& e) = 0;

  plProcessCommunicationChannel* m_pCommunicationChannel = nullptr;
  plEvent<const plProcessCommunicationChannel::Event&>::Unsubscriber m_Unsubscriber;
};
