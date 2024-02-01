#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

class plLongOpProxy;

/// \brief Events about all known long ops. Broadcast by plLongOpControllerManager.
struct plLongOpControllerEvent
{
  enum class Type
  {
    OpAdded,    ///< A new long op has been added / registered.
    OpRemoved,  ///< A long op has been deleted. The GUID is sent, but it cannot be resolved anymore.
    OpProgress, ///< The completion progress of a long op has changed.
  };

  Type m_Type;
  plUuid m_OperationGuid; ///< Use plLongOpControllerManager::GetOperation() to resolve the GUID to the actual long op.
};

/// \brief The LongOp controller is active in the editor process and manages which long ops are available, running, etc.
///
/// All available long ops are registered with the controller, typically automatically by the plLongOpsAdapter,
/// although it is theoretically possible to register additional long ops.
///
/// Through the controller long ops can be started or canceled, which is exposed in the UI by the plQtLongOpsPanel.
///
/// Through the broadcast plLongOpControllerEvent, one can track the state of all long ops.
class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpControllerManager final : public plLongOpManager
{
  PL_DECLARE_SINGLETON(plLongOpControllerManager);

public:
  plLongOpControllerManager();
  ~plLongOpControllerManager();

  /// \brief Holds all information about the proxy long op on the editor side
  struct ProxyOpInfo
  {
    plUniquePtr<plLongOpProxy> m_pProxyOp;
    plUuid m_OperationGuid;     ///< Identifies the operation itself.
    plUuid m_DocumentGuid;      ///< To which document the long op belongs. When the document is closed, all running long ops belonging to it
                                ///< will be canceled.
    plUuid m_ComponentGuid;     ///< To which component in the scene document the long op is linked. If the component is deleted, the long op
                                ///< disappears as well.
    plTime m_StartOrDuration;   ///< While m_bIsRunning is true, this is the time the long op started, once m_bIsRunning it holds the last
                                ///< duration of the long op execution.
    float m_fCompletion = 0.0f; ///< [0; 1] range for the progress.
    bool m_bIsRunning = false;  ///< Whether the long op is currently being executed.
  };

  /// \brief Events about the state of all available long ops.
  plEvent<const plLongOpControllerEvent&> m_Events;

  /// \brief Typically called by plLongOpsAdapter when a component that has an plLongOpAttribute is added to a scene
  void RegisterLongOp(const plUuid& documentGuid, const plUuid& componentGuid, const char* szLongOpType);

  /// \brief Typically called by plLongOpsAdapter when a component that has an plLongOpAttribute is removed from a scene
  void UnregisterLongOp(const plUuid& documentGuid, const plUuid& componentGuid, const char* szLongOpType);

  /// \brief Starts executing the given long op. Typically called by the plQtLongOpsPanel.
  void StartOperation(plUuid opGuid);

  /// \brief Cancels a given long op. Typically called by the plQtLongOpsPanel.
  void CancelOperation(plUuid opGuid);

  /// \brief Cancels and deletes all operations linked to the given document. Makes sure to wait for all canceled ops.
  /// Typically called by the plLongOpsAdapter when a document is about to be closed.
  void CancelAndRemoveAllOpsForDocument(const plUuid& documentGuid);

  /// \brief Returns a pointer to the given long op, or null if the GUID does not exist.
  ProxyOpInfo* GetOperation(const plUuid& opGuid);

  /// \brief Gives access to all currently available long ops. Make sure the lock m_Mutex (of the plLongOpManager base class) while accessing this.
  const plDynamicArray<plUniquePtr<ProxyOpInfo>>& GetOperations() const { return m_ProxyOps; }

private:
  virtual void ProcessCommunicationChannelEventHandler(const plProcessCommunicationChannel::Event& e) override;

  void ReplicateToWorkerProcess(ProxyOpInfo& opInfo);
  void BroadcastProgress(ProxyOpInfo& opInfo);
  void RemoveOperation(plUuid opGuid);

  plDynamicArray<plUniquePtr<ProxyOpInfo>> m_ProxyOps;
};
