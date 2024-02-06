#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <Foundation/Threading/Thread.h>

/// \brief The audio thread. Responsible to process asynchronous audio requests.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioThread : public plThread
{
public:
  plAudioThread();

private:
  friend class plAudioSystem;

  plUInt32 Run() override;

  class plAudioSystem* m_pAudioSystem = nullptr;
  volatile bool m_bKeepRunning = true;
};
