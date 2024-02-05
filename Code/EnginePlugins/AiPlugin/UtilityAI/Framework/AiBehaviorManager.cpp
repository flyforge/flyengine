#include <AiPlugin/AiPluginPCH.h>

#include <AiPlugin/UtilityAI/Framework/AiBehaviorManager.h>

plAiBehaviorManager::plAiBehaviorManager() = default;
plAiBehaviorManager::~plAiBehaviorManager() = default;

void plAiBehaviorManager::DetermineAvailableBehaviors(plTime currentTime, float fActiveBehaviorScore)
{
  m_CurrentTime = currentTime;
  ++m_uiUpdateCount;

  for (auto& info : m_Behaviors)
  {
    if (info.m_CooldownUntil > currentTime)
      continue;

    if (info.m_pBehavior->IsAvailable(fActiveBehaviorScore))
    {
      info.m_uiNeededInUpdate = m_uiUpdateCount;
    }
  }
}

void plAiBehaviorManager::AddBehavior(plUniquePtr<plAiBehavior>&& pBehavior)
{
  auto& info = m_Behaviors.ExpandAndGetRef();
  info.m_pBehavior = std::move(pBehavior);
}

void plAiBehaviorManager::FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager)
{
  for (auto& info : m_Behaviors)
  {
    if (info.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      info.m_pBehavior->FlagNeededPerceptions(ref_PerceptionManager);
    }
  }
}

plAiBehaviorCandidate plAiBehaviorManager::DetermineBehaviorCandidate(plGameObject& owner, const plAiPerceptionManager& perceptionManager)
{
  plAiBehaviorScore res;
  plAiBehaviorCandidate candidate;

  for (auto& info : m_Behaviors)
  {
    if (info.m_uiNeededInUpdate == m_uiUpdateCount)
    {
      const plAiBehaviorScore scored = info.m_pBehavior->DetermineBehaviorScore(owner, perceptionManager);

      if (scored.GetScore() > res.GetScore())
      {
        res = scored;
        candidate.m_pBehavior = info.m_pBehavior.Borrow();
        candidate.m_pPerception = scored.m_pPerception;
        candidate.m_fScore = res.GetScore();
      }
    }
  }

  return candidate;
}

void plAiBehaviorManager::SetActiveBehavior(plGameObject& owner, plAiBehavior* pBehavior, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior == pBehavior && pPerception == nullptr)
  {
    // do not deactivate and activate again, just keep calling ContinueBehavior()
    return;
  }

  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->DeactivateBehavior(owner, inout_ActionQueue);
  }

  m_pActiveBehavior = pBehavior;

  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->ActivateBehavior(owner, pPerception, inout_ActionQueue);

    for (auto& info : m_Behaviors)
    {
      if (info.m_pBehavior == m_pActiveBehavior)
      {
        info.m_CooldownUntil = m_CurrentTime + m_pActiveBehavior->GetCooldownDuration();
        break;
      }
    }
  }
}

void plAiBehaviorManager::KeepActiveBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior)
  {
    m_pActiveBehavior->ReactivateBehavior(owner, pPerception, inout_ActionQueue);
  }
}

plAiBehavior::ContinuationState plAiBehaviorManager::ContinueActiveBehavior(plGameObject& owner, plAiActionQueue& inout_ActionQueue)
{
  if (m_pActiveBehavior)
  {
    return m_pActiveBehavior->ContinueBehavior(owner, inout_ActionQueue);
  }

  return {};
}
