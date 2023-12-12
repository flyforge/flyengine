#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>

class PlasmaEditorEngineProcessApp;
class plDocumentOpenMsgToEngine;
class PlasmaEngineProcessDocumentContext;
class plResourceUpdateMsgToEngine;
class plRestoreResourceMsgToEngine;

class PlasmaEngineProcessGameApplication : public plGameApplication
{
public:
  typedef plGameApplication SUPER;

  PlasmaEngineProcessGameApplication();
  ~PlasmaEngineProcessGameApplication();

  virtual plResult BeforeCoreSystemsStartup() override;
  virtual void AfterCoreSystemsStartup() override;

  virtual void BeforeCoreSystemsShutdown() override;

  virtual plApplication::Execution Run() override;

  void LogWriter(const plLoggingEventData& e);

protected:
  virtual void BaseInit_ConfigureLogging() override;
  virtual void Deinit_ShutdownLogging() override;
  virtual void Init_FileSystem_ConfigureDataDirs() override;
  virtual bool Run_ProcessApplicationInput() override;
  virtual plUniquePtr<PlasmaEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  { /* do nothing */
  }

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const PlasmaEngineProcessCommunicationChannel::Event& e);
  void EventHandlerCVar(const plCVarEvent& e);
  void EventHandlerCVarPlugin(const plPluginEvent& e);
  void TransmitCVar(const plCVar* pCVar);

  void HandleResourceUpdateMsg(const plResourceUpdateMsgToEngine& msg);
  void HandleResourceRestoreMsg(const plRestoreResourceMsgToEngine& msg);

  PlasmaEngineProcessDocumentContext* CreateDocumentContext(const plDocumentOpenMsgToEngine* pMsg);

  virtual void Init_LoadProjectPlugins() override;

  virtual plString FindProjectDirectory() const override;

  plString m_sProjectDirectory;
  plApplicationFileSystemConfig m_CustomFileSystemConfig;
  plApplicationPluginConfig m_CustomPluginConfig;
  PlasmaEngineProcessCommunicationChannel m_IPC;
  plUniquePtr<PlasmaEditorEngineProcessApp> m_pApp;
  plLongOpWorkerManager m_LongOpWorkerManager;

  plUInt32 m_uiRedrawCountReceived = 0;
  plUInt32 m_uiRedrawCountExecuted = 0;
};
