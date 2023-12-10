#include <EditorEngineProcessFramework/EditorEngineProcessFrameworkPCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorEngineProcessFramework/LongOps/LongOpControllerManager.h>
#include <EditorEngineProcessFramework/LongOps/LongOps.h>

PLASMA_IMPLEMENT_SINGLETON(plLongOpControllerManager);

plLongOpControllerManager::plLongOpControllerManager()
  : m_SingletonRegistrar(this)
{
}

plLongOpControllerManager::~plLongOpControllerManager() = default;

void plLongOpControllerManager::ProcessCommunicationChannelEventHandler(const plProcessCommunicationChannel::Event& e)
{
  if (auto pMsg = plDynamicCast<const plLongOpProgressMsg*>(e.m_pMessage))
  {
    PLASMA_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_fCompletion = pMsg->m_fCompletion;

      BroadcastProgress(*pOpInfo);
    }

    return;
  }

  if (auto pMsg = plDynamicCast<const plLongOpResultMsg*>(e.m_pMessage))
  {
    PLASMA_LOCK(m_Mutex);

    if (auto pOpInfo = GetOperation(pMsg->m_OperationGuid))
    {
      pOpInfo->m_bIsRunning = false;
      pOpInfo->m_StartOrDuration = plTime::Now() - pOpInfo->m_StartOrDuration;
      pOpInfo->m_fCompletion = 0.0f;

      pOpInfo->m_pProxyOp->Finalize(pMsg->m_bSuccess ? PLASMA_SUCCESS : PLASMA_FAILURE, pMsg->m_ResultData);

      // TODO: show success/failure in UI
      BroadcastProgress(*pOpInfo);
    }
  }
}

void plLongOpControllerManager::StartOperation(plUuid opGuid)
{
  PLASMA_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || pOpInfo->m_bIsRunning)
    return;

  pOpInfo->m_StartOrDuration = plTime::Now();
  pOpInfo->m_bIsRunning = true;

  ReplicateToWorkerProcess(*pOpInfo);

  BroadcastProgress(*pOpInfo);
}

void plLongOpControllerManager::CancelOperation(plUuid opGuid)
{
  PLASMA_LOCK(m_Mutex);

  auto pOpInfo = GetOperation(opGuid);

  if (pOpInfo == nullptr || !pOpInfo->m_bIsRunning)
    return;

  // send a cancel message to the processor
  plLongOpResultMsg msg;
  msg.m_OperationGuid = opGuid;
  msg.m_bSuccess = false;
  m_pCommunicationChannel->SendMessage(&msg);
}

void plLongOpControllerManager::RemoveOperation(plUuid opGuid)
{
  PLASMA_LOCK(m_Mutex);

  for (plUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    if (m_ProxyOps[i]->m_OperationGuid == opGuid)
    {
      m_ProxyOps.RemoveAtAndCopy(i);

      // broadcast the removal to the UI
      {
        plLongOpControllerEvent e;
        e.m_Type = plLongOpControllerEvent::Type::OpRemoved;
        e.m_OperationGuid = opGuid;

        m_Events.Broadcast(e);
      }

      return;
    }
  }
}

void plLongOpControllerManager::RegisterLongOp(const plUuid& documentGuid, const plUuid& componentGuid, const char* szLongOpType)
{
  const plRTTI* pRtti = plRTTI::FindTypeByName(szLongOpType);
  if (pRtti == nullptr)
  {
    plLog::Error("Can't register long op of unknown type '{}'", szLongOpType);
    return;
  }

  auto& opInfoPtr = m_ProxyOps.ExpandAndGetRef();
  opInfoPtr = PLASMA_DEFAULT_NEW(ProxyOpInfo);

  auto& opInfo = *opInfoPtr;
  opInfo.m_DocumentGuid = documentGuid;
  opInfo.m_ComponentGuid = componentGuid;
  opInfo.m_OperationGuid = plUuid::MakeUuid();

  opInfo.m_pProxyOp = pRtti->GetAllocator()->Allocate<plLongOpProxy>();
  opInfo.m_pProxyOp->InitializeRegistered(documentGuid, componentGuid);

  plLongOpControllerEvent e;
  e.m_Type = plLongOpControllerEvent::Type::OpAdded;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}

void plLongOpControllerManager::UnregisterLongOp(const plUuid& documentGuid, const plUuid& componentGuid, const char* szLongOpType)
{
  for (plUInt32 i = 0; i < m_ProxyOps.GetCount(); ++i)
  {
    auto& opInfoPtr = m_ProxyOps[i];

    if (opInfoPtr->m_ComponentGuid == componentGuid && opInfoPtr->m_DocumentGuid == documentGuid &&
        opInfoPtr->m_pProxyOp->GetDynamicRTTI()->GetTypeName() == szLongOpType)
    {
      RemoveOperation(opInfoPtr->m_OperationGuid);
      return;
    }
  }
}

plLongOpControllerManager::ProxyOpInfo* plLongOpControllerManager::GetOperation(const plUuid& opGuid)
{
  PLASMA_LOCK(m_Mutex);

  for (auto& opInfoPtr : m_ProxyOps)
  {
    if (opInfoPtr->m_OperationGuid == opGuid)
      return opInfoPtr.Borrow();
  }

  return nullptr;
}

void plLongOpControllerManager::CancelAndRemoveAllOpsForDocument(const plUuid& documentGuid)
{
  {
    PLASMA_LOCK(m_Mutex);

    for (auto& opInfoPtr : m_ProxyOps)
    {
      CancelOperation(opInfoPtr->m_OperationGuid);
    }
  }

  bool bOperationsStillActive = true;

  while (bOperationsStillActive)
  {
    bOperationsStillActive = false;
    m_pCommunicationChannel->ProcessMessages();

    {
      PLASMA_LOCK(m_Mutex);

      for (plUInt32 i0 = m_ProxyOps.GetCount(); i0 > 0; --i0)
      {
        const plUInt32 i = i0 - 1;

        auto& op = m_ProxyOps[i];
        if (op->m_DocumentGuid == documentGuid)
        {
          if (op->m_bIsRunning)
          {
            bOperationsStillActive = true;
            break;
          }

          plLongOpControllerEvent e;
          e.m_Type = plLongOpControllerEvent::Type::OpRemoved;
          e.m_OperationGuid = m_ProxyOps[i]->m_OperationGuid;

          m_ProxyOps.RemoveAtAndCopy(i);

          m_Events.Broadcast(e);
        }
      }
    }

    if (bOperationsStillActive)
    {
      plThreadUtils::Sleep(plTime::MakeFromMilliseconds(100));
    }
  }
}

void plLongOpControllerManager::ReplicateToWorkerProcess(ProxyOpInfo& opInfo)
{
  PLASMA_LOCK(m_Mutex);

  // send the replication message
  {
    plLongOpReplicationMsg msg;

    plMemoryStreamContainerWrapperStorage<plDataBuffer> storage(&msg.m_ReplicationData);
    plMemoryStreamWriter writer(&storage);

    plStringBuilder replType;
    opInfo.m_pProxyOp->GetReplicationInfo(replType, writer);

    msg.m_sReplicationType = replType;
    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
    msg.m_OperationGuid = opInfo.m_OperationGuid;

    m_pCommunicationChannel->SendMessage(&msg);
  }
}

void plLongOpControllerManager::BroadcastProgress(ProxyOpInfo& opInfo)
{
  // as controller, broadcast progress to the UI
  plLongOpControllerEvent e;
  e.m_Type = plLongOpControllerEvent::Type::OpProgress;
  e.m_OperationGuid = opInfo.m_OperationGuid;
  m_Events.Broadcast(e);
}


// void plLongOpManager::AddLongOperation(plUniquePtr<plLongOp>&& pOperation, const plUuid& documentGuid)
//{
//  PLASMA_LOCK(m_Mutex);
//
//  auto& opInfoPtr = m_Operations.ExpandAndGetRef();
//  opInfoPtr = PLASMA_DEFAULT_NEW(LongOpInfo);
//
//  auto& opInfo = *opInfoPtr;
//  opInfo.m_pOperation = std::move(pOperation);
//  opInfo.m_OperationGuid.CreateNewUuid();
//  opInfo.m_DocumentGuid = documentGuid;
//  opInfo.m_StartOrDuration = plTime::Now();
//  opInfo.m_Progress.m_pUserData = opInfo.m_pOperation.Borrow();
//  opInfo.m_Progress.m_Events.AddEventHandler(
//    plMakeDelegate(&plLongOpManager::ProgressBarEventHandler, this), opInfo.m_ProgressSubscription);
//
//  plLongOp* pNewOp = opInfo.m_pOperation.Borrow();
//
//  if (m_Mode == Mode::Processor || plDynamicCast<plLongOpProxy*>(pNewOp) != nullptr)
//  {
//    plStringBuilder replType;
//
//    plLongOpReplicationMsg msg;
//
//    plMemoryStreamContainerWrapperStorage<plDataBuffer> storage(&msg.m_ReplicationData);
//    plMemoryStreamWriter writer(&storage);
//
//    pNewOp->GetReplicationInfo(replType, writer);
//
//    msg.m_sReplicationType = replType;
//    msg.m_DocumentGuid = opInfo.m_DocumentGuid;
//    msg.m_OperationGuid = opInfo.m_OperationGuid;
//    msg.m_sDisplayName = pNewOp->GetDisplayName();
//
//    m_pCommunicationChannel->SendMessage(&msg);
//  }
//
//  LaunchWorkerOperation(opInfo);
//
//  {
//    plLongOpManagerEvent e;
//    e.m_Type = plLongOpManagerEvent::Type::OpAdded;
//    e.m_uiOperationIndex = m_Operations.GetCount() - 1;
//    m_Events.Broadcast(e);
//  }
//}
