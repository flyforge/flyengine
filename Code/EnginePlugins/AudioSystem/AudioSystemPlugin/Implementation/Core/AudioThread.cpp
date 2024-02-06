#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioSystem.h>
#include <AudioSystemPlugin/Core/AudioThread.h>

plAudioThread::plAudioThread()
  : plThread("AudioThread")
{
}

plUInt32 plAudioThread::Run()
{
  PL_ASSERT_DEBUG(m_pAudioSystem, "AudioThread has no AudioSystem!");

  while (m_bKeepRunning)
  {
    m_pAudioSystem->UpdateInternal();
  }

  return 0;
}

PL_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_AudioThread);
