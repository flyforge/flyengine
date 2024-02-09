#pragma once

#include <AiPlugin/UtilityAI/Framework/AiAction.h>
#include <Core/World/GameObject.h>

class PL_AIPLUGIN_DLL plAiActionWait : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionWait);

public:
  plAiActionWait();
  ~plAiActionWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plTime m_Duration;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionLerpRotation : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionLerpRotation);

public:
  plAiActionLerpRotation();
  ~plAiActionLerpRotation();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plVec3 m_vTurnAxis = plVec3::MakeAxisZ();
  plAngle m_TurnAngle;
  plAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionLerpPosition : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionLerpPosition);

public:
  plAiActionLerpPosition();
  ~plAiActionLerpPosition();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  float m_fSpeed = 0.0f;
  plVec3 m_vLocalSpaceSlide = plVec3::MakeZero();
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionLerpRotationTowards : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionLerpRotationTowards);

public:
  plAiActionLerpRotationTowards();
  ~plAiActionLerpRotationTowards();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plVec3 m_vTargetPosition = plVec3::MakeZero();
  plGameObjectHandle m_hTargetObject;
  plAngle m_TargetReachedAngle = plAngle::MakeFromDegree(5);
  plAngle m_TurnAnglesPerSec;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

// class PL_AIPLUGIN_DLL plAiActionFollowPath : public plAiAction
//{
//   PL_DECLARE_AICMD(plAiActionFollowPath);
//
// public:
//   plAiActionFollowPath();
//   ~plAiActionFollowPath();
//
//   virtual void Reset() override;
//   virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
//   virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
//   virtual void Cancel(plGameObject& owner) override;
//
//   plGameObjectHandle m_hPath;
//   float m_fSpeed = 0.0f;
// };

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionBlackboardSetEntry : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionBlackboardSetEntry);

public:
  plAiActionBlackboardSetEntry();
  ~plAiActionBlackboardSetEntry();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  bool m_bNoCancel = false;
  plHashedString m_sEntryName;
  plVariant m_Value;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionBlackboardWait : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionBlackboardWait);

public:
  plAiActionBlackboardWait();
  ~plAiActionBlackboardWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plTempHashedString m_sEntryName;
  plVariant m_Value;
  bool m_bEquals = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionBlackboardSetAndWait : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionBlackboardSetAndWait);

public:
  plAiActionBlackboardSetAndWait();
  ~plAiActionBlackboardSetAndWait();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plHashedString m_sEntryName;
  plVariant m_SetValue;
  plVariant m_WaitValue;
  bool m_bEqualsWaitValue = true;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionCCMoveTo : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionCCMoveTo);

public:
  plAiActionCCMoveTo();
  ~plAiActionCCMoveTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plVec3 m_vTargetPosition = plVec3::MakeZero();
  plGameObjectHandle m_hTargetObject;
  float m_fSpeed = 0.0f;
  float m_fReachedDistSQR = 1.0f;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionSpawn : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionSpawn);

public:
  plAiActionSpawn();
  ~plAiActionSpawn();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plTempHashedString m_sChildObjectName;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionQuip : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionQuip);

public:
  plAiActionQuip();
  ~plAiActionQuip();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plString m_sLogMsg;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

class PL_AIPLUGIN_DLL plAiActionNavigateTo : public plAiAction
{
  PL_DECLARE_AICMD(plAiActionNavigateTo);

public:
  plAiActionNavigateTo();
  ~plAiActionNavigateTo();

  virtual void Reset() override;
  virtual void GetDebugDesc(plStringBuilder& inout_sText) override;
  virtual plAiActionResult Execute(plGameObject& owner, plTime tDiff, plLogInterface* pLog) override;
  virtual void Cancel(plGameObject& owner) override;

  plVec3* m_pTargetPosition = nullptr;
  float m_fSpeed = 0.0f;
  float m_fReachedDist = 1.0f;
  bool m_bStarted = false;
};
