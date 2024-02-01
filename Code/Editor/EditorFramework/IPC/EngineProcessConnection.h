#pragma once

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/EditorFrameworkDLL.h>
#include <EditorFramework/IPC/EditorProcessCommunicationChannel.h>
#include <Foundation/Application/Config/PluginConfig.h>
#include <Foundation/Communication/Event.h>
#include <Foundation/Configuration/Singleton.h>
#include <Foundation/Types/UniquePtr.h>
#include <GuiFoundation/UIServices/UIServices.moc.h>

class plEditorEngineConnection;
class plDocument;
class plDocumentObject;
struct plDocumentObjectPropertyEvent;
struct plDocumentObjectStructureEvent;
class plQtEngineDocumentWindow;
class plAssetDocument;

class PL_EDITORFRAMEWORK_DLL plEditorEngineProcessConnection
{
  PL_DECLARE_SINGLETON(plEditorEngineProcessConnection);

public:
  plEditorEngineProcessConnection();
  ~plEditorEngineProcessConnection();

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

  plEditorEngineConnection* CreateEngineConnection(plAssetDocument* pDocument);
  void DestroyEngineConnection(plAssetDocument* pDocument);

  bool SendMessage(plProcessMessage* pMessage);

  /// /brief Waits for a message of type pMessageType. If tTimeout is zero, the function will not timeout. If the timeout is valid
  ///        and is it, PL_FAILURE is returned. If the message type matches and pCallback is valid, the function will be called
  ///        and the return values decides whether the message is to be accepted and the waiting has ended.
  plResult WaitForMessage(const plRTTI* pMessageType, plTime timeout, plProcessCommunicationChannel ::WaitForMessageCallback* pCallback = nullptr);
  /// /brief Same as WaitForMessage but the message must be to a specific document. Therefore,
  ///        pMessageType must be derived from plEditorEngineDocumentMsg and the function will only return if the received
  ///        message matches both type, document and is accepted by pCallback.
  plResult WaitForDocumentMessage(
    const plUuid& assetGuid, const plRTTI* pMessageType, plTime timeout, plProcessCommunicationChannel::WaitForMessageCallback* pCallback = nullptr);

  bool IsEngineSetup() const { return m_bClientIsConfigured; }

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

  bool m_bProcessShouldBeRunning;
  bool m_bProcessCrashed;
  bool m_bClientIsConfigured;
  plEventSubscriptionID m_TickEventSubscriptionID = 0;
  plUInt32 m_uiRedrawCountSent = 0;
  plUInt32 m_uiRedrawCountReceived = 0;

  plEditorProcessCommunicationChannel m_IPC;
  plUniquePtr<plEditorProcessRemoteCommunicationChannel> m_pRemoteProcess;
  plApplicationFileSystemConfig m_FileSystemConfig;
  plApplicationPluginConfig m_PluginConfig;
  plHashTable<plUuid, plAssetDocument*> m_DocumentByGuid;
};

class PL_EDITORFRAMEWORK_DLL plEditorEngineConnection
{
public:
  bool SendMessage(plEditorEngineDocumentMsg* pMessage);
  void SendHighlightObjectMessage(plViewHighlightMsgToEngine* pMessage);

  plDocument* GetDocument() const { return m_pDocument; }

private:
  friend class plEditorEngineProcessConnection;
  plEditorEngineConnection(plDocument* pDocument) { m_pDocument = pDocument; }
  ~plEditorEngineConnection() = default;

  plDocument* m_pDocument;
};
