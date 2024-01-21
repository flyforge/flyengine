#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiActionQueue.h>
#include <Core/World/GameObject.h>
#include <Foundation/Profiling/Profiling.h>
#include <RendererCore/Debug/DebugRenderer.h>

plAiActionQueue::plAiActionQueue() = default;

plAiActionQueue::~plAiActionQueue()
{
  InterruptAndClear();
}

bool plAiActionQueue::IsEmpty() const
{
  return m_Queue.IsEmpty();
}

void plAiActionQueue::InterruptAndClear()
{
  for (auto pCmd : m_Queue)
  {
    pCmd->Destroy();
  }

  m_Queue.Clear();
}

void plAiActionQueue::CancelCurrentActions(plGameObject& owner)
{
  for (plUInt32 i = 0; i < m_Queue.GetCount(); ++i)
  {
    m_Queue[i]->Cancel(owner);
  }
}

void plAiActionQueue::QueueAction(plAiAction* pAction)
{
  m_Queue.PushBack(pAction);
}

void plAiActionQueue::Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog)
{
  PLASMA_LOG_BLOCK("AiActionExecution");
  PLASMA_PROFILE_SCOPE("AiActionExecution");

  while (!m_Queue.IsEmpty())
  {
    plAiAction* pCmd = m_Queue[0];

    const plAiActionResult res = pCmd->Execute(owner, tDiff, pLog);
    if (res == plAiActionResult::Succeded)
      return;

    if (res == plAiActionResult::Failed)
    {
      plStringBuilder str;
      pCmd->GetDebugDesc(str);
      plLog::Error("AI cmd failed: {}", str);

      CancelCurrentActions(owner);
    }

    pCmd->Destroy();
    m_Queue.RemoveAtAndCopy(0);
  }
}

void plAiActionQueue::PrintDebugInfo(plGameObject& owner)
{
  plStringBuilder str;

  if (m_Queue.IsEmpty())
  {
    str = "<AI action queue empty>";
    plDebugRenderer::DrawInfoText(owner.GetWorld(), plDebugTextPlacement::BottomRight, "AI", str, plColor::Orange);
  }
  else
  {
    m_Queue[0]->GetDebugDesc(str);
    plDebugRenderer::DrawInfoText(owner.GetWorld(), plDebugTextPlacement::BottomRight, "AI", str);
  }
}
