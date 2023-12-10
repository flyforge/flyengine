#pragma once

#include <FileservePlugin/Fileserver/Fileserver.h>
#include <Foundation/Application/Application.h>
#include <Foundation/Communication/RemoteInterface.h>
#include <Foundation/Types/UniquePtr.h>

/// \brief A stand-alone application for the plFileServer.
///
/// If PLASMA_USE_QT is defined, the GUI from the EditorPluginFileserve is used. Otherwise the server runs as a console application.
///
/// If the command line option "-fs_wait_timeout seconds" is specified, the server will wait for a limited time for any client to
/// connect and close automatically, if no connection is established. Once a client connects, the timeout becomes irrelevant.
/// If the command line option "-fs_close_timeout seconds" is specified, the application will automatically shut down when no
/// client is connected anymore and a certain timeout is reached. Once a client connects, the timeout is reset.
/// This timeout has no effect as long as no client has connected.
class plFileserverApp : public plApplication
{
public:
  typedef plApplication SUPER;

  plFileserverApp()
    : plApplication("Fileserve")
  {
  }

  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;
  virtual void BeforeCoreSystemsShutdown() override;

  virtual plApplication::Execution Run() override;
  void FileserverEventHandlerConsole(const plFileserverEvent& e);
  void FileserverEventHandler(const plFileserverEvent& e);

  plUInt32 m_uiSleepCounter = 0;
  plUInt32 m_uiConnections = 0;
  plTime m_CloseAppTimeout;
  plTime m_TimeTillClosing;
};
