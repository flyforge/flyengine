#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessCommunicationChannel.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <Foundation/Application/Config/FileSystemConfig.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Types/UniquePtr.h>
#include <GameEngine/GameApplication/GameApplication.h>
#include <Foundation/Logging/HTMLWriter.h>

class plEditorEngineProcessApp;
class plDocumentOpenMsgToEngine;
class plEngineProcessDocumentContext;
class plResourceUpdateMsgToEngine;
class plRestoreResourceMsgToEngine;

class plEngineProcessGameApplication : public plGameApplication
{
public:
  using SUPER = plGameApplication;

  plEngineProcessGameApplication();
  ~plEngineProcessGameApplication();

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
  virtual plUniquePtr<plEditorEngineProcessApp> CreateEngineProcessApp();

  virtual void ActivateGameStateAtStartup() override
  { /* do nothing */
  }

private:
  void ConnectToHost();
  void DisableErrorReport();
  void WaitForDebugger();
  static bool EditorAssertHandler(const char* szSourceFile, plUInt32 uiLine, const char* szFunction, const char* szExpression, const char* szAssertMsg);
  void AddEditorAssertHandler();
  void RemoveEditorAssertHandler();

  bool ProcessIPCMessages(bool bPendingOpInProgress);
  void SendProjectReadyMessage();
  void SendReflectionInformation();
  void EventHandlerIPC(const plEngineProcessCommunicationChannel::Event& e);
  void EventHandlerCVar(const plCVarEvent& e);
  void EventHandlerCVarPlugin(const plPluginEvent& e);
  void TransmitCVar(const plCVar* pCVar);

  void HandleResourceUpdateMsg(const plResourceUpdateMsgToEngine& msg);
  void HandleResourceRestoreMsg(const plRestoreResourceMsgToEngine& msg);

  plEngineProcessDocumentContext* CreateDocumentContext(const plDocumentOpenMsgToEngine* pMsg);

  virtual void Init_LoadProjectPlugins() override;

  virtual plString FindProjectDirectory() const override;

  plString m_sProjectDirectory;
  plApplicationFileSystemConfig m_CustomFileSystemConfig;
  plApplicationPluginConfig m_CustomPluginConfig;
  plEngineProcessCommunicationChannel m_IPC;
  plUniquePtr<plEditorEngineProcessApp> m_pApp;
  plLongOpWorkerManager m_LongOpWorkerManager;
  plLogWriter::HTML m_LogHTML;

  plUInt32 m_uiRedrawCountReceived = 0;
  plUInt32 m_uiRedrawCountExecuted = 0;
};
