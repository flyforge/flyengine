#pragma once

#include <GameEngine/GameEngineDLL.h>

class plAiBehavior;
class plAiPerceptionManager;
class plAiPerception;
class plGameObject;
class plAiActionQueue;

enum class plAiScoreCategory
{
  Fallback,    ///< if really nothing else is available
  Idle,        ///< Idle actions (simple animation playback and such)
  ActiveIdle,  ///< The main actions to do when an NPC is idle, e.g. wander around, follow a patrol path
  Investigate, ///< in case something interesting is detected
  Command,     ///< things the player (or level designer) instructs the NPC to do that should have high priority
  Combat,      ///< combat related behavior
  Interrupt,   ///< things that even override combat scenarios (hit reactions, falling down, etc)
};

class plAiBehaviorScore
{
public:
  plAiBehaviorScore() {}

  void SetScore(plAiScoreCategory category, float fValue)
  {
    PLASMA_ASSERT_DEBUG(fValue >= 0.0f && fValue <= 1.0f, "Value is out of 0-1 range.");
    m_fValue = static_cast<float>(category) + fValue;
  }

  float GetScore() const { return m_fValue; }

  const plAiPerception* m_pPerception = nullptr;

private:
  float m_fValue = 0.0f;
};

struct plAiBehaviorCandidate
{
  plAiBehavior* m_pBehavior = nullptr;
  const plAiPerception* m_pPerception = nullptr;
  float m_fScore = 0.0f;
};

class PLASMA_AIPLUGIN_DLL plAiBehavior
{
public:
  plAiBehavior() = default;
  virtual ~plAiBehavior() = default;

  struct ContinuationState
  {
    /// \brief If false, the behavior is in a state where it really wants to finish something, before another behavior may take over.
    bool m_bAllowBehaviorSwitch = true;
    /// \brief In case the behavior wants to quit.
    bool m_bEndBehavior = false;
  };

  virtual bool IsAvailable(float fActiveBehaviorScore) const { return true; }
  virtual void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager) = 0;

  virtual plAiBehaviorScore DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager) = 0;

  virtual void ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) = 0;
  virtual void ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) = 0;
  virtual void DeactivateBehavior(plGameObject& owner, plAiActionQueue& inout_ActionQueue) {}
  virtual ContinuationState ContinueBehavior(plGameObject& owner, plAiActionQueue& inout_ActionQueue)
  {
    ContinuationState res;
    res.m_bEndBehavior = inout_ActionQueue.IsEmpty();
    return res;
  }

  virtual plTime GetCooldownDuration() { return plTime::MakeZero(); }
};
