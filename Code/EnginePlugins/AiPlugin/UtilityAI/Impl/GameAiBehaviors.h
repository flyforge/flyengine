#pragma once

#include <AiPlugin/UtilityAI/Framework/AiBehavior.h>

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiBehaviorGoToPOI : public plAiBehavior
{
public:
  plAiBehaviorGoToPOI();
  ~plAiBehaviorGoToPOI();

  virtual void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager) override;

  virtual plAiBehaviorScore DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override;

private:
  plVec3 m_vTargetPosition = plVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiBehaviorWander : public plAiBehavior
{
public:
  plAiBehaviorWander();
  ~plAiBehaviorWander();

  virtual void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager) override;

  virtual plAiBehaviorScore DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override {}

private:
  plVec3 m_vTargetPosition = plVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiBehaviorShoot : public plAiBehavior
{
public:
  plAiBehaviorShoot();
  ~plAiBehaviorShoot();

  virtual void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager) override;

  virtual plAiBehaviorScore DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override {}
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PLASMA_AIPLUGIN_DLL plAiBehaviorQuip : public plAiBehavior
{
public:
  plAiBehaviorQuip();
  ~plAiBehaviorQuip();

  virtual void FlagNeededPerceptions(plAiPerceptionManager& ref_PerceptionManager) override;

  virtual plAiBehaviorScore DetermineBehaviorScore(plGameObject& owner, const plAiPerceptionManager& perceptionManager) override;
  virtual void ActivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override;
  void ReactivateBehavior(plGameObject& owner, const plAiPerception* pPerception, plAiActionQueue& inout_ActionQueue) override {}

  virtual plTime GetCooldownDuration() override;
};
