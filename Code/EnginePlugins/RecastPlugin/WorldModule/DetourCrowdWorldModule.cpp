#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <DetourCrowd.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <Foundation/Configuration/CVar.h>
#include <RecastPlugin/WorldModule/DetourCrowdWorldModule.h>

plCVarBool cvar_DetourCrowdVisAgents("Recast.Crowd.VisAgents", false, plCVarFlags::Default, "Draws DetourCrowd agents, if any");
plCVarBool cvar_DetourCrowdVisCorners("Recast.Crowd.VisCorners", false, plCVarFlags::Default, "Draws next few path cornders of the DetourCrowd agents");
plCVarInt cvar_DetourCrowdMaxAgents("Recast.Crowd.MaxAgents", 128, plCVarFlags::Save, "Determines how many DetourCrowd agents can be created");
plCVarFloat cvar_DetourCrowdMaxRadius("Recast.Crowd.MaxRadius", 2.0f, plCVarFlags::Save, "Determines the maximum allowed radius of a DetourCrowd agent");

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_IMPLEMENT_WORLD_MODULE(plDetourCrowdWorldModule);
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plDetourCrowdWorldModule, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on


plDetourCrowdWorldModule::plDetourCrowdWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
{
}

plDetourCrowdWorldModule::~plDetourCrowdWorldModule() = default;

void plDetourCrowdWorldModule::Initialize()
{
  SUPER::Initialize();

  m_pRecastModule = GetWorld()->GetOrCreateModule<plRecastWorldModule>();

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plDetourCrowdWorldModule::UpdateNavMesh, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PreAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;
    //desc.m_DependsOn.PushBack(plMakeHashedString("plRecastWorldModule::UpdateNavMesh"));

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plDetourCrowdWorldModule::UpdateCrowd, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::Async;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }

  {
    auto desc = PL_CREATE_MODULE_UPDATE_FUNCTION_DESC(plDetourCrowdWorldModule::VisualizeCrowd, this);
    desc.m_Phase = plWorldModule::UpdateFunctionDesc::Phase::PostAsync;
    desc.m_bOnlyUpdateWhenSimulating = true;
    desc.m_fPriority = 0.0f;

    RegisterUpdateFunction(desc);
  }
}

void plDetourCrowdWorldModule::Deinitialize()
{
  if (m_pDtCrowd != nullptr)
  {
    dtFreeCrowd(m_pDtCrowd);
    m_pDtCrowd = nullptr;
  }
  
  SUPER::Deinitialize();
}

bool plDetourCrowdWorldModule::IsInitializedAndReady() const
{
  if (m_pDtCrowd == nullptr)
    return false;

  if (m_pRecastModule == nullptr)
    return false;
  
  const dtNavMesh* pNavMesh = m_pRecastModule->GetDetourNavMesh();

  return m_pDtCrowd->getNavMeshQuery()->getAttachedNavMesh() == pNavMesh;
}

const dtCrowdAgent* plDetourCrowdWorldModule::GetAgentById(plInt32 iAgentId) const
{
  if (!IsInitializedAndReady())
    return nullptr;

  return m_pDtCrowd->getAgent(iAgentId);
}

void plDetourCrowdWorldModule::FillDtCrowdAgentParams(const plDetourCrowdAgentParams& params, struct dtCrowdAgentParams& out_params) const
{
  out_params.radius = plMath::Clamp(params.m_fRadius, 0.0f, m_fMaxAgentRadius);
  out_params.height = params.m_fHeight;
  out_params.maxAcceleration = params.m_fMaxAcceleration;
  out_params.maxSpeed = params.m_fMaxSpeed;
  out_params.collisionQueryRange = plMath::Max(1.2f, 12.0f * params.m_fRadius);
  out_params.pathOptimizationRange = plMath::Max(3.0f, 30.0f * params.m_fRadius);
  out_params.updateFlags = DT_CROWD_ANTICIPATE_TURNS | DT_CROWD_OPTIMIZE_VIS | DT_CROWD_OPTIMIZE_TOPO 
    | DT_CROWD_OBSTACLE_AVOIDANCE | DT_CROWD_SEPARATION;
  out_params.obstacleAvoidanceType = 3;
  out_params.separationWeight = params.m_fSeparationWeight;
  out_params.userData = params.m_pUserData;
}

plInt32 plDetourCrowdWorldModule::CreateAgent(const plVec3& vPos, const plDetourCrowdAgentParams& params)
{
  if (!IsInitializedAndReady())
    return -1;

  dtCrowdAgentParams dtParams{};
  FillDtCrowdAgentParams(params, dtParams);

  plInt32 iAgentId = m_pDtCrowd->addAgent(plRcPos(vPos), &dtParams);

  return iAgentId;
}

void plDetourCrowdWorldModule::DestroyAgent(plInt32 iAgentId)
{
  if (!IsInitializedAndReady())
    return;

  m_pDtCrowd->removeAgent(iAgentId);
}

void plDetourCrowdWorldModule::SetAgentTargetPosition(plInt32 iAgentId, const plVec3& vPos, const plVec3& vQueryHalfExtents)
{
  if (!IsInitializedAndReady())
    return;

  float vNavPos[3];
  dtPolyRef navPolyRef;
  m_pDtCrowd->getNavMeshQuery()->findNearestPoly(plRcPos(vPos), plRcPos(vQueryHalfExtents), m_pDtCrowd->getFilter(0), &navPolyRef, vNavPos);
  m_pDtCrowd->requestMoveTarget(iAgentId, navPolyRef, vNavPos);
}

void plDetourCrowdWorldModule::ClearAgentTargetPosition(plInt32 iAgentId)
{
  if (!IsInitializedAndReady())
    return;

  m_pDtCrowd->resetMoveTarget(iAgentId);
}

void plDetourCrowdWorldModule::UpdateAgentParams(plInt32 iAgentId, const plDetourCrowdAgentParams& params)
{
  if (!IsInitializedAndReady())
    return;

  dtCrowdAgentParams dtParams{};
  FillDtCrowdAgentParams(params, dtParams);

  m_pDtCrowd->updateAgentParameters(iAgentId, &dtParams);
}

void plDetourCrowdWorldModule::UpdateNavMesh(const plWorldModule::UpdateContext& ctx)
{
  const dtNavMesh* pNavMesh = m_pRecastModule->GetDetourNavMesh();

  plInt32 iDesiredMaxAgents = plMath::Clamp(cvar_DetourCrowdMaxAgents.GetValue(), 8, 2048);
  plInt32 fDesiredMaxRadius = plMath::Clamp(cvar_DetourCrowdMaxRadius.GetValue(), 0.01f, 5.0f);

  if (pNavMesh != nullptr && (m_pDtCrowd == nullptr || m_pDtCrowd->getNavMeshQuery()->getAttachedNavMesh() != pNavMesh 
    || m_iMaxAgents != iDesiredMaxAgents || m_fMaxAgentRadius != fDesiredMaxRadius))
  {
    if (m_pDtCrowd == nullptr)
      m_pDtCrowd = dtAllocCrowd();
    m_iMaxAgents = iDesiredMaxAgents;
    m_fMaxAgentRadius = fDesiredMaxRadius;
    m_pDtCrowd->init(m_iMaxAgents, m_fMaxAgentRadius, const_cast<dtNavMesh*>(pNavMesh));
  }
}

void plDetourCrowdWorldModule::UpdateCrowd(const plWorldModule::UpdateContext& ctx)
{
  if (!IsInitializedAndReady())
    return;

  const float fDeltaTime = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();
  m_pDtCrowd->update(fDeltaTime, nullptr);
}

void plDetourCrowdWorldModule::VisualizeCrowd(const UpdateContext& ctx)
{
  if (!IsInitializedAndReady() || !cvar_DetourCrowdVisAgents)
    return;

  const plInt32 iNumAgents = m_pDtCrowd->getAgentCount();
  for (int i = 0; i < iNumAgents; ++i)
  {
    const dtCrowdAgent* pAgent = m_pDtCrowd->getAgent(i);
    if (pAgent->active)
    {
      const float fHeight = pAgent->params.height;
      const float fRadius = pAgent->params.radius;

      plTransform xform(plRcPos(pAgent->npos));
      xform.m_vPosition.z += fHeight * 0.5f;

      // Draw agent cylinder
      plDebugRenderer::DrawLineCylinderZ(GetWorld(), fHeight, fRadius, plColor::BlueViolet, xform);

      // Draw velocity arrow
      plVec3 vVelocity = plRcPos(pAgent->vel);
      vVelocity.z = 0;
      if (!vVelocity.IsZero())
      {
        vVelocity.Normalize();
        xform.m_qRotation = plQuat::MakeShortestRotation(plVec3(1, 0, 0), vVelocity);
        plDebugRenderer::DrawArrow(GetWorld(), 1.0f, plColor::BlueViolet, xform);
      }

      // Draw corners
      if (cvar_DetourCrowdVisCorners.GetValue() && pAgent->ncorners > 0)
      {
        plDebugRenderer::Line lines[DT_CROWDAGENT_MAX_CORNERS];

        lines[0].m_start = plRcPos(pAgent->npos);
        lines[0].m_end = plRcPos(pAgent->cornerVerts);

        for (int i = 1; i < pAgent->ncorners; ++i)
        {
          lines[i].m_start = plRcPos(pAgent->cornerVerts + 3 * (i-1));
          lines[i].m_end = plRcPos(pAgent->cornerVerts + 3 * i);
        }

        plDebugRenderer::DrawLines(GetWorld(), plArrayPtr(lines, pAgent->ncorners), plColor::Cyan, plTransform::Make(plVec3(0.0f, 0.0f, 0.1f)));
      }
    }
  }
}
