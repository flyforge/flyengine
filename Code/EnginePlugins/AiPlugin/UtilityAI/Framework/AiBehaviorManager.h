#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Types/UniquePtr.h>

class plGameObject;
class plAiPerceptionManager;

class PLASMA_AIPLUGIN_DLL plAiBehaviorManager
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plAiBehaviorManager);

public:
  plAiBehaviorManager();
  ~plAiBehaviorManager();

  void DetermineAvailableBehaviors(plTime currentTime, float fActiveBehaviorScore);

  void AddBehavior(plUniquePtr<plAiBehavior>&& pBehavior);

  void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager);

  plAiBehaviorCandidate DetermineBehaviorCandidate(plGameObject& owner, const plAiPerceptionManager& perceptionManager);

  void SetActiveBehavior(plGameObject& owner, plAiBehavior* pBehavior, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue);
  void KeepActiveBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue);

  plAiBehavior::ContinuationState ContinueActiveBehavior(plGameObject& owner, plAiActionQueue& inout_ActionQueue);

  bool HasActiveBehavior() const { return m_pActiveBehavior != nullptr; }

  plAiBehavior* GetActiveBehavior() const { return m_pActiveBehavior; }

private:
  struct BehaviorInfo
  {
    // bool m_bActive = true;
    plTime m_CooldownUntil;
    plUInt32 m_uiNeededInUpdate = 0;

    plUniquePtr<plAiBehavior> m_pBehavior;
  };

  plAiBehavior* m_pActiveBehavior = nullptr;
  plHybridArray<BehaviorInfo, 12> m_Behaviors;

  plUInt32 m_uiUpdateCount = 1;
  plTime m_CurrentTime;
};
