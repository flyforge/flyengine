#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Communication/IpcChannel.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

class plRemoteInterface;
class plRemoteMessage;

class PLASMA_FOUNDATION_DLL plIpcChannelEnet : public plIpcChannel
{
public:
  plIpcChannelEnet(plStringView sAddress, Mode::Enum mode);
  ~plIpcChannelEnet();

protected:
  virtual void InternalConnect() override;
  virtual void InternalDisconnect() override;
  virtual void InternalSend() override;
  virtual bool NeedWakeup() const override;
  virtual bool RequiresRegularTick() override { return true; }
  virtual void Tick() override;
  void NetworkMessageHandler(plRemoteMessage& msg);
  void EnetEventHandler(const plRemoteEvent& e);

  plString m_sAddress;
  plString m_sLastAddress;
  plTime m_LastConnectAttempt;
  plUniquePtr<plRemoteInterface> m_pNetwork;
};

#endif
