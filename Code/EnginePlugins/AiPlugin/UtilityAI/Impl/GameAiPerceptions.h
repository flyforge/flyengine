#pragma once

#include <Foundation/Math/Vec3.h>
#include <AiPlugin/UtilityAI/Framework/AiPerception.h>

class PL_AIPLUGIN_DLL plAiPerceptionPOI : public plAiPerception
{
public:
  plAiPerceptionPOI() = default;
  ~plAiPerceptionPOI() = default;

  plVec3 m_vGlobalPosition = plVec3::MakeZero();
};


class PL_AIPLUGIN_DLL plAiPerceptionWander : public plAiPerception
{
public:
  plAiPerceptionWander() = default;
  ~plAiPerceptionWander() = default;

  plVec3 m_vGlobalPosition = plVec3::MakeZero();
};

class PL_AIPLUGIN_DLL plAiPerceptionCheckpoint : public plAiPerception
{
public:
  plAiPerceptionCheckpoint() = default;
  ~plAiPerceptionCheckpoint() = default;

  plVec3 m_vGlobalPosition = plVec3::MakeZero();
};
