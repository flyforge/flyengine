#pragma once

#include <Foundation/Communication/RemoteInterface.h>

#ifdef BUILDSYSTEM_ENABLE_ENET_SUPPORT

/// \brief An implementation for plRemoteInterface built on top of Enet
class PL_FOUNDATION_DLL plRemoteInterfaceEnet : public plRemoteInterface
{
public:
  ~plRemoteInterfaceEnet();

  /// \brief Allocates a new instance with the given allocator
  static plInternal::NewInstance<plRemoteInterfaceEnet> Make(plAllocator* pAllocator = plFoundation::GetDefaultAllocator());

  /// \brief The port through which the connection was started
  plUInt16 GetPort() const { return m_uiPort; }

private:
  plRemoteInterfaceEnet();
  friend class plRemoteInterfaceEnetImpl;

protected:
  plUInt16 m_uiPort = 0;
};

#endif
