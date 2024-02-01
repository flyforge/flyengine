#pragma once

#include <EditorEngineProcessFramework/IPC/ProcessCommunicationChannel.h>

class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plEngineProcessCommunicationChannel : public plProcessCommunicationChannel
{
public:
  plResult ConnectToHostProcess();

  bool IsHostAlive() const;

private:
  plInt64 m_iHostPID = 0;
};
