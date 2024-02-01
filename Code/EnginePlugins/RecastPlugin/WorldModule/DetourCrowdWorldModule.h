#pragma once

#include <RecastPlugin/RecastPluginDLL.h>

#include <Core/World/WorldModule.h>

class dtCrowd;
class dtCrowdAgent;
class plRecastWorldModule;


struct plDetourCrowdAgentParams
{
  /// \brief Agent radius. [Limit: >= 0]
  float m_fRadius;

  /// \brief Agent height. [Limit: > 0]
  float m_fHeight;

  /// \brief Maximum allowed acceleration. [Limit: >= 0]
  float m_fMaxAcceleration;

  /// \brief Maximum allowed speed. [Limit: >= 0]
  float m_fMaxSpeed;

  /// \brief How aggresive the agent manager should be at avoiding collisions with this agent. [Limit: >= 0]
  float m_fSeparationWeight;

  void* m_pUserData;

  static inline plDetourCrowdAgentParams Default()
  {
    plDetourCrowdAgentParams params;
    params.m_fRadius = 0.3f;
    params.m_fHeight = 1.8f;
    params.m_fMaxAcceleration = 10.0f;
    params.m_fMaxSpeed = 3.5f;
    params.m_fSeparationWeight = 2.0f;
    params.m_pUserData = nullptr;
    return params;
  }
};


class PL_RECASTPLUGIN_DLL plDetourCrowdWorldModule : public plWorldModule
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plDetourCrowdWorldModule, plWorldModule);

public:
  plDetourCrowdWorldModule(plWorld* pWorld);
  ~plDetourCrowdWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;

  bool IsInitializedAndReady() const;

  const dtCrowdAgent* GetAgentById(plInt32 iAgentId) const;

  /// \brief Tries to create a new crowd agent and returns its ID or -1.
  plInt32 CreateAgent(const plVec3& vPos, const plDetourCrowdAgentParams& params);

  void DestroyAgent(plInt32 iAgentId);

  void SetAgentTargetPosition(plInt32 iAgentId, const plVec3& vPos, const plVec3& vQueryHalfExtents = plVec3(0.5f, 0.5f, 0.5f));
  
  void ClearAgentTargetPosition(plInt32 iAgentId);

  void UpdateAgentParams(plInt32 iAgentId, const plDetourCrowdAgentParams& params);

private:
  void UpdateNavMesh(const UpdateContext& ctx);
  void UpdateCrowd(const UpdateContext& ctx);
  void VisualizeCrowd(const UpdateContext& ctx);

  void FillDtCrowdAgentParams(const plDetourCrowdAgentParams& params, struct dtCrowdAgentParams& out_params) const;

  plInt32 m_iMaxAgents = 128;
  float m_fMaxAgentRadius = 2.0f;
  dtCrowd* m_pDtCrowd = nullptr;
  plRecastWorldModule* m_pRecastModule = nullptr;
};
