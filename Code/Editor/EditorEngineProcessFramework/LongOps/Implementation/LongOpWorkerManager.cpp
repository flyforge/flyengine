#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpWorkerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

PLASMA_IMPLEMENT_SINGLETON(plLongOpWorkerManager);

plLongOpWorkerManager::plLongOpWorkerManager()
  : m_SingletonRegistrar(this)
{
}

plLongOpWorkerManager::~plLongOpWorkerManager() = default;

class plLongOpTask final : public plTask
{
public:
  plLongOpWorker* m_pWorkerOp = nullptr;
  plUuid m_OperationGuid;
  plProgress* m_pProgress = nullptr;

  plLongOpTask()
  {
    plStringBuilder name;
    name.Format("Long Op: '{}'", "TODO: NAME"); // TODO
    ConfigureTask(name, plTaskNesting::Maybe);
  }

  ~plLongOpTask() = default;

  virtual void Execute() override
  {
    if (HasBeenCanceled())
      return;

    plDataBuffer resultData;
    plMemoryStreamContainerWrapperStorage<plDataBuffer> storage(&resultData);
    plMemoryStreamWriter writer(&storage);

    const plResult res = m_pWorkerOp->Execute(*m_pProgress, writer);

    plLongOpWorkerManager::GetSingleton()->WorkerOperationFinished(m_OperationGuid, res, std::move(resultData));
  }
};

void plLongOpWorkerManager::ProcessCommunicationChannelEventHandler(const plProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = plDynamicCast<const plLongOpReplicationMsg*>(e.m_pMessage))
  {
    PLASMA_LOCK(m_Mutex);

    plRawMemoryStreamReader reader(pMsg->m_ReplicationData);
    const plRTTI* pRtti = plRTTI::FindTypeByName(pMsg->m_sReplicationType);

    auto& opInfoPtr = m_WorkerOps.ExpandAndGetRef();
    opInfoPtr = PLASMA_DEFAULT_NEW(WorkerOpInfo);

    auto& opInfo = *opInfoPtr;
    opInfo.m_DocumentGuid = pMsg->m_DocumentGuid;
    opInfo.m_OperationGuid = pMsg->m_OperationGuid;
    opInfo.m_pWorkerOp = pRtti->GetAllocator()->Allocate<plLongOpWorker>();

    LaunchWorkerOperation(opInfo, reader);
    return;
  }

  if (auto pMsg = plDynamicCast<const plLongOpResultMsg*>(e.m_pMessage))
  {
    PLASMA_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      PLASMA_ASSERT_DEBUG(pMsg->m_bSuccess == false, "Only Cancel messages are allowed to send to the processor");

      pOpInfo->m_Progress.UserClickedCancel();
    }

    return;
  }
}

void plLongOpWorkerManager::LaunchWorkerOperation(WorkerOpInfo& opInfo, plStreamReader& config)
{
  opInfo.m_Progress.SetCompletion(0.0f);
  opInfo.m_Progress.m_pUserData = &opInfo;
  opInfo.m_Progress.m_Events.AddEventHandler(
    plMakeDelegate(&plLongOpWorkerManager::WorkerProgressBarEventHandler, this), opInfo.m_ProgressSubscription);

  SendProgress(opInfo);

  if (opInfo.m_pWorkerOp->InitializeExecution(config, opInfo.m_DocumentGuid).Failed())
  {
    WorkerOperationFinished(opInfo.m_OperationGuid, PLASMA_FAILURE, plDataBuffer());
  }
  else
  {
    plSharedPtr<plLongOpTask> pTask = PLASMA_DEFAULT_NEW(plLongOpTask);
    pTask->m_OperationGuid = opInfo.m_OperationGuid;
    pTask->m_pWorkerOp = opInfo.m_pWorkerOp.Borrow();
    pTask->m_pProgress = &opInfo.m_Progress;
    opInfo.m_TaskID = plTaskSystem::StartSingleTask(pTask, plTaskPriority::LongRunning);
  }
}

void plLongOpWorkerManager::WorkerOperationFinished(plUuid operationGuid, plResult result, plDataBuffer&& resultData)
{
  PLASMA_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(operationGuid);

  if (pOpInfo == nullptr)
    return;

  // tell the controller about the result
  {
    plLongOpResultMsg msg;
    msg.m_OperationGuid = operationGuid;
    msg.m_bSuccess = result.Succeeded();
    msg.m_ResultData = std::move(resultData);

    m_pCommunicationChannel->SendMessage(&msg);
  }

  RemoveOperation(operationGuid);
}

void plLongOpWorkerManager::WorkerProgressBarEventHandler(const plProgressEvent& e)
{
  if (e.m_Type == plProgressEvent::Type::ProgressChanged)
  {
    auto pOpInfo = static_cast<WorkerOpInfo*>(e.m_pProgressbar->m_pUserData);

    SendProgress(*pOpInfo);
  }
}

void plLongOpWorkerManager::RemoveOperation(plUuid opGuid)
{
  PLASMA_LOCK(m_Mutex);

  for (plUInt32 i = 0; i < m_WorkerOps.GetCount(); ++i)
  {
    if (m_WorkerOps[i]->m_OperationGuid == opGuid)
    {
      m_WorkerOps.RemoveAtAndSwap(i);
      return;
    }
  }
}

plLongOpWorkerManager::WorkerOpInfo* plLongOpWorkerManager::GetOperation(const plUuid& guid) const
{
  PLASMA_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_WorkerOps)
  {
    if (opInfoPtr->m_OperationGuid == guid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void plLongOpWorkerManager::SendProgress(WorkerOpInfo& opInfo)
{
  plLongOpProgressMsg msg;
  msg.m_OperationGuid = opInfo.m_OperationGuid;
  msg.m_fCompletion = opInfo.m_Progress.GetCompletion();

  m_pCommunicationChannel->SendMessage(&msg);
}
