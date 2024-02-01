#pragma once

#include <EditorEngineProcessFramework/LongOps/Implementation/LongOpManager.h>

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Threading/Implementation/TaskSystemDeclarations.h>
#include <Foundation/Utilities/Progress.h>

class plLongOpWorker;
struct plProgressEvent;
using plDataBuffer = plDynamicArray<plUInt8>;

/// \brief The LongOp worker manager is active in the engine process of the editor.
///
/// This class has no public functionality, it communicates with the plLongOpControllerManager
/// and executes the plLongOpWorker's that are named by the respective plLongOpProxy's.
class PL_EDITORENGINEPROCESSFRAMEWORK_DLL plLongOpWorkerManager final : public plLongOpManager
{
  PL_DECLARE_SINGLETON(plLongOpWorkerManager);

public:
  plLongOpWorkerManager();
  ~plLongOpWorkerManager();

private:
  friend class plLongOpTask;

  struct WorkerOpInfo
  {
    plUniquePtr<plLongOpWorker> m_pWorkerOp;
    plTaskGroupID m_TaskID;
    plUuid m_DocumentGuid;
    plUuid m_OperationGuid;
    plProgress m_Progress;
    plEvent<const plProgressEvent&>::Unsubscriber m_ProgressSubscription;
  };

  virtual void ProcessCommunicationChannelEventHandler(const plProcessCommunicationChannel::Event& e) override;
  WorkerOpInfo* GetOperation(const plUuid& opGuid) const;
  void LaunchWorkerOperation(WorkerOpInfo& opInfo, plStreamReader& config);
  void WorkerProgressBarEventHandler(const plProgressEvent& e);
  void RemoveOperation(plUuid opGuid);
  void SendProgress(WorkerOpInfo& opInfo);
  void WorkerOperationFinished(plUuid operationGuid, plResult result, plDataBuffer&& resultData);

  plDynamicArray<plUniquePtr<WorkerOpInfo>> m_WorkerOps;
};
