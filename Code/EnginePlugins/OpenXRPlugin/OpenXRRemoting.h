#pragma once

#ifdef BUILDSYSTEM_ENABLE_OPENXR_REMOTING_SUPPORT
#  include <OpenXRPlugin/Basics.h>
#  include <OpenXRPlugin/OpenXRIncludes.h>

#  include <Foundation/Configuration/Singleton.h>
#  include <GameEngine/XR/XRRemotingInterface.h>


class plOpenXR;

class plOpenXRRemoting : public plXRRemotingInterface
{
  PLASMA_DECLARE_SINGLETON_OF_INTERFACE(plOpenXRRemoting, plXRRemotingInterface);

public:
  plOpenXRRemoting(plOpenXR* pOpenXR);
  ~plOpenXRRemoting();

  virtual plResult Initialize() override;
  virtual plResult Deinitialize() override;
  virtual bool IsInitialized() const override;

  virtual plResult Connect(const char* remoteHostName, uint16_t remotePort, bool enableAudio, int maxBitrateKbps) override;
  virtual plResult Disconnect() override;
  virtual plEnum<plXRRemotingConnectionState> GetConnectionState() const override;
  virtual plXRRemotingConnectionEvent& GetConnectionEvent() override;

private:
  friend class plOpenXR;

  void HandleEvent(const XrEventDataBuffer& event);

private:
  bool m_bInitialized = false;
  plString m_sPreviousRuntime;
  plOpenXR* m_pOpenXR = nullptr;
  plXRRemotingConnectionEvent m_event;
};
#endif
