#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

class PlasmaEditorEngineConnection;
class plDocument;
class plDocumentObject;
struct plDocumentObjectPropertyEvent;
struct plDocumentObjectStructureEvent;
class plQtEngineDocumentWindow;
class plAssetDocument;

class PLASMA_EDITORFRAMEWORK_DLL PlasmaEditorEngineProcessConnection
{
  PLASMA_DECLARE_SINGLETON(PlasmaEditorEngineProcessConnection);

public:
  PlasmaEditorEngineProcessConnection();
  ~PlasmaEditorEngineProcessConnection();

  /// \brief The given file system configuration will be used by the engine process to setup the runtime data directories.
  ///        This only takes effect if the editor process is restarted.
  void SetFileSystemConfig(const plApplicationFileSystemConfig& cfg) { m_FileSystemConfig = cfg; }

  /// \brief The given plugin configuration will be used by the engine process to load runtime plugins.
  ///        This only takes effect if the editor process is restarted.
  void SetPluginConfig(const plApplicationPluginConfig& cfg) { m_PluginConfig = cfg; }

  void Update();
  plResult RestartProcess();
  void ShutdownProcess();
  bool IsProcessCrashed() const { return m_bProcessCrashed; }

  PlasmaEditorEngineConnection* CreateEngineConnection(plAssetDocument* pDocument);
  void DestroyEngineConnection(plAssetDocument* pDocument);

  void SendMessage(plProcessMessage* pMessage);

  /// /brief Waits for a message of type pMessageType. If tTimeout is zero, the function will not timeout. If the timeout is valid
  ///        and is it, PLASMA_FAILURE is returned. If the message type matches and pCallback is valid, the function will be called
  ///        and the return values decides whether the message is to be accepted and the waiting has ended.
  plResult WaitForMessage(const plRTTI* pMessageType, plTime tTimeout, plProcessCommunicationChannel ::WaitForMessageCallback* pCallback = nullptr);
  /// /brief Same as WaitForMessage but the message must be to a specific document. Therefore,
  ///        pMessageType must be derived from PlasmaEditorEngineDocumentMsg and the function will only return if the received
  ///        message matches both type, document and is accepted by pCallback.
  plResult WaitForDocumentMessage(
    const plUuid& assetGuid, const plRTTI* pMessageType, plTime tTimeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback = nullptr);

  void SetWaitForDebugger(bool bWait) { m_bProcessShouldWaitForDebugger = bWait; }
  bool GetWaitForDebugger() const { return m_bProcessShouldWaitForDebugger; }

  void SetRenderer(const char* szRenderer) { m_sRenderer = szRenderer; }
  const char* GetRenderer() const { return m_sRenderer; }

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

  /// /brief Sends a message that the document has been opened or closed. Resends all document data.
  ///
  /// Calling this will always clear the existing document on the engine side and reset the state to the editor state.
  void SendDocumentOpenMessage(const plAssetDocument* pDocument, bool bOpen);

  void ActivateRemoteProcess(const plAssetDocument* pDocument, plUInt32 uiViewID);

  plProcessCommunicationChannel& GetCommunicationChannel() { return m_IPC; }

  struct Event
  {
    enum class Type
    {
      Invalid,
      ProcessStarted,
      ProcessCrashed,
      ProcessShutdown,
      ProcessMessage,
      ProcessRestarted,
    };

    Event()
    {
      m_Type = Type::Invalid;
      m_pMsg = nullptr;
    }

    Type m_Type;
    const plProcessMessage* m_pMsg;
  };

  static plEvent<const Event&> s_Events;

private:
  void Initialize(const plRTTI* pFirstAllowedMessageType);
  void HandleIPCEvent(const plProcessCommunicationChannel::Event& e);
  void UIServicesTickEventHandler(const plQtUiServices::TickEvent& e);
  bool ConnectToRemoteProcess();
  void ShutdownRemoteProcess();

  bool m_bProcessShouldWaitForDebugger;
  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bClientIsConfigured;
  plEventSubscriptionID m_TickEventSubscriptionID = 0;
  plUInt32 m_uiRedrawCountSent = 0;
  plUInt32 m_uiRedrawCountReceived = 0;

  plString m_sRenderer;
  PlasmaEditorProcessCommunicationChannel m_IPC;
  plUniquePtr<PlasmaEditorProcessRemoteCommunicationChannel> m_pRemoteProcess;
  plApplicationFileSystemConfig m_FileSystemConfig;
  plApplicationPluginConfig m_PluginConfig;
  plHashTable<plUuid, plAssetDocument*> m_DocumentByGuid;
};

class PLASMA_EDITORFRAMEWORK_DLL PlasmaEditorEngineConnection
{
public:
  void SendMessage(PlasmaEditorEngineDocumentMsg* pMessage);
  void SendHighlightObjectMessage(plViewHighlightMsgToEngine* pMessage);

  plDocument* GetDocument() const { return m_pDocument; }

private:
  friend class PlasmaEditorEngineProcessConnection;
  PlasmaEditorEngineConnection(plDocument* pDocument) { m_pDocument = pDocument; }
  ~PlasmaEditorEngineConnection() {}

  plDocument* m_pDocument;
};
