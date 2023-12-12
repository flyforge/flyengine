#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/ResourceManager/ResourceManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Components/RecastAgentComponent.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/Utils/RcMath.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRcAgentComponent, 2, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("WalkSpeed",m_fWalkSpeed)->AddAttributes(new plDefaultValueAttribute(4.0f)),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plRcAgentComponent::plRcAgentComponent() {}
plRcAgentComponent::~plRcAgentComponent() {}

void plRcAgentComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_fWalkSpeed;
}

void plRcAgentComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_fWalkSpeed;
}

plResult plRcAgentComponent::InitializeRecast()
{
  if (m_bRecastInitialized)
    return PLASMA_SUCCESS;

  const dtNavMesh* pNavMesh = GetWorld()->GetOrCreateModule<plRecastWorldModule>()->GetDetourNavMesh();
  if (pNavMesh == nullptr)
    return PLASMA_FAILURE;

  m_bRecastInitialized = true;

  m_pQuery = PLASMA_DEFAULT_NEW(dtNavMeshQuery);
  m_pCorridor = PLASMA_DEFAULT_NEW(dtPathCorridor);

  /// \todo Hard-coded limits
  m_pQuery->init(pNavMesh, 512);
  m_pCorridor->init(256);

  return PLASMA_SUCCESS;
}

void plRcAgentComponent::UninitializeRecast()
{
  if (!m_bRecastInitialized)
    return;

  m_bRecastInitialized = false;
  m_pQuery.Clear();
  m_pCorridor.Clear();

  if (m_PathToTargetState != plAgentPathFindingState::HasNoTarget)
    SetTargetPosition(m_vTargetPosition);
  else
    ClearTargetPosition();
}

void plRcAgentComponent::ClearTargetPosition()
{
  m_iNumNextSteps = 0;
  m_iFirstNextStep = 0;
  m_PathCorridor.Clear();
  m_vCurrentSteeringDirection.SetZero();

  if (m_PathToTargetState != plAgentPathFindingState::HasNoTarget)
  {
    m_PathToTargetState = plAgentPathFindingState::HasNoTarget;

    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = plAgentSteeringEvent::TargetCleared;

    m_SteeringEvents.Broadcast(e, 1);
  }
}

plAgentPathFindingState::Enum plRcAgentComponent::GetPathToTargetState() const
{
  return m_PathToTargetState;
}

void plRcAgentComponent::SetTargetPosition(const plVec3& vPos)
{
  ClearTargetPosition();

  m_vTargetPosition = vPos;
  m_PathToTargetState = plAgentPathFindingState::HasTargetWaitingForPath;
}

plVec3 plRcAgentComponent::GetTargetPosition() const
{
  return m_vTargetPosition;
}

plResult plRcAgentComponent::FindNavMeshPolyAt(const plVec3& vPosition, dtPolyRef& out_PolyRef, plVec3* out_vAdjustedPosition /*= nullptr*/, float fPlaneEpsilon /*= 0.01f*/, float fHeightEpsilon /*= 1.0f*/) const
{
  plRcPos rcPos = vPosition;
  plVec3 vSize(fPlaneEpsilon, fHeightEpsilon, fPlaneEpsilon);

  plRcPos resultPos;
  dtQueryFilter filter; /// \todo Hard-coded filter
  if (dtStatusFailed(m_pQuery->findNearestPoly(rcPos, &vSize.x, &m_QueryFilter, &out_PolyRef, resultPos)))
    return PLASMA_FAILURE;

  if (!plMath::IsEqual(vPosition.x, resultPos.m_Pos[0], fPlaneEpsilon) || !plMath::IsEqual(vPosition.y, resultPos.m_Pos[2], fPlaneEpsilon) || !plMath::IsEqual(vPosition.z, resultPos.m_Pos[1], fHeightEpsilon))
    return PLASMA_FAILURE;

  if (out_vAdjustedPosition != nullptr)
  {
    *out_vAdjustedPosition = resultPos;
  }

  return PLASMA_SUCCESS;
}

plResult plRcAgentComponent::ComputePathCorridor(dtPolyRef startPoly, dtPolyRef endPoly, bool& bFoundPartialPath)
{
  bFoundPartialPath = false;

  plRcPos rcStart = m_vCurrentPositionOnNavmesh;
  plRcPos rcEnd = m_vTargetPosition;

  plInt32 iPathCorridorLength = 0;

  // make enough room
  m_PathCorridor.SetCountUninitialized(256);
  if (dtStatusFailed(m_pQuery->findPath(startPoly, endPoly, rcStart, rcEnd, &m_QueryFilter, m_PathCorridor.GetData(), &iPathCorridorLength, (int)m_PathCorridor.GetCount())) || iPathCorridorLength <= 0)
  {
    m_PathCorridor.Clear();
    return PLASMA_FAILURE;
  }

  // reduce to actual length
  m_PathCorridor.SetCountUninitialized(iPathCorridorLength);

  if (m_PathCorridor[iPathCorridorLength - 1] != endPoly)
  {
    // if this is the case, the target position cannot be reached, but we can walk close to it
    bFoundPartialPath = true;
  }

  m_pCorridor->reset(startPoly, rcStart);
  m_pCorridor->setCorridor(rcEnd, m_PathCorridor.GetData(), iPathCorridorLength);

  return PLASMA_SUCCESS;
}

plResult plRcAgentComponent::ComputePathToTarget()
{
  const plVec3 vStartPos = GetOwner()->GetGlobalPosition();

  dtPolyRef startPoly;
  if (FindNavMeshPolyAt(vStartPos, startPoly, &m_vCurrentPositionOnNavmesh).Failed())
  {
    m_PathToTargetState = plAgentPathFindingState::HasTargetPathFindingFailed;

    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = plAgentSteeringEvent::ErrorOutsideNavArea;
    m_SteeringEvents.Broadcast(e);
    return PLASMA_FAILURE;
  }

  dtPolyRef endPoly;
  if (FindNavMeshPolyAt(m_vTargetPosition, endPoly).Failed())
  {
    m_PathToTargetState = plAgentPathFindingState::HasTargetPathFindingFailed;

    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = plAgentSteeringEvent::ErrorInvalidTargetPosition;
    m_SteeringEvents.Broadcast(e);
    return PLASMA_FAILURE;
  }

  /// \todo Optimize case when endPoly is same as previously ?

  bool bFoundPartialPath = false;
  if (ComputePathCorridor(startPoly, endPoly, bFoundPartialPath).Failed() || bFoundPartialPath)
  {
    m_PathToTargetState = plAgentPathFindingState::HasTargetPathFindingFailed;

    /// \todo For now a partial path is considered an error

    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = bFoundPartialPath ? plAgentSteeringEvent::WarningNoFullPathToTarget : plAgentSteeringEvent::ErrorNoPathToTarget;
    m_SteeringEvents.Broadcast(e);
    return PLASMA_FAILURE;
  }

  m_PathToTargetState = plAgentPathFindingState::HasTargetAndValidPath;

  plAgentSteeringEvent e;
  e.m_pComponent = this;
  e.m_Type = plAgentSteeringEvent::PathToTargetFound;
  m_SteeringEvents.Broadcast(e);
  return PLASMA_SUCCESS;
}

bool plRcAgentComponent::HasReachedPosition(const plVec3& pos, float fMaxDistance) const
{
  plVec3 vTargetPos = pos;
  plVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  /// \todo The comment below may not always be true
  const float fCellHeight = 1.5f;

  // agent component is assumed to be located on the ground (independent of character height)
  // so max error is dependent on the navmesh resolution mostly (cell height)
  const float fHeightError = fCellHeight;

  if (!plMath::IsInRange(vTargetPos.z, vOwnPos.z - fHeightError, vOwnPos.z + fHeightError))
    return false;

  vTargetPos.z = 0;
  vOwnPos.z = 0;

  return (vTargetPos - vOwnPos).GetLengthSquared() < plMath::Square(fMaxDistance);
}

bool plRcAgentComponent::HasReachedGoal(float fMaxDistance) const
{
  if (GetPathToTargetState() == plAgentPathFindingState::HasNoTarget)
    return true;

  return HasReachedPosition(m_vTargetPosition, fMaxDistance);
}

void plRcAgentComponent::PlanNextSteps()
{
  if (m_PathCorridor.IsEmpty())
    return;

  plUInt8 stepFlags[16];
  dtPolyRef stepPolys[16];

  m_iFirstNextStep = 0;
  m_iNumNextSteps = m_pCorridor->findCorners(&m_vNextSteps[0].x, stepFlags, stepPolys, 4, m_pQuery.Borrow(), &m_QueryFilter);

  // convert from Recast convention (Y up) to pl (Z up)
  for (plInt32 i = 0; i < m_iNumNextSteps; ++i)
  {
    plMath::Swap(m_vNextSteps[i].y, m_vNextSteps[i].z);
  }
}

bool plRcAgentComponent::IsPositionVisible(const plVec3& pos) const
{
  plRcPos endPos = pos;

  dtRaycastHit hit;
  if (dtStatusFailed(m_pQuery->raycast(m_pCorridor->getFirstPoly(), m_pCorridor->getPos(), endPos, &m_QueryFilter, 0, &hit)))
    return false;

  // 'visible' if no hit was detected
  return (hit.t > 100000.0f);
}

void plRcAgentComponent::OnSimulationStarted()
{
  ClearTargetPosition();

  m_bRecastInitialized = false;

  plCharacterControllerComponent* pCC = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<plCharacterControllerComponent>(pCC))
  {
    m_hCharacterController = pCC->GetHandle();
  }
}

void plRcAgentComponent::ApplySteering(const plVec3& vDirection, float fSpeed)
{
  // compute new rotation
  {
    plQuat qDesiredNewRotation;
    qDesiredNewRotation.SetShortestRotation(plVec3(1, 0, 0), vDirection);

    /// \todo Pass through character controller
    GetOwner()->SetGlobalRotation(qDesiredNewRotation);
  }

  if (!m_hCharacterController.IsInvalidated())
  {
    plCharacterControllerComponent* pCharacter = nullptr;
    if (GetWorld()->TryGetComponent(m_hCharacterController, pCharacter))
    {
      // the character controller already applies time scaling
      const plVec3 vRelativeSpeed = (-GetOwner()->GetGlobalRotation() * vDirection) * fSpeed;

      plMsgMoveCharacterController msg;
      msg.m_fMoveForwards = plMath::Max(0.0f, vRelativeSpeed.x);
      msg.m_fMoveBackwards = plMath::Max(0.0f, -vRelativeSpeed.x);
      msg.m_fStrafeLeft = plMath::Max(0.0f, -vRelativeSpeed.y);
      msg.m_fStrafeRight = plMath::Max(0.0f, vRelativeSpeed.y);

      pCharacter->MoveCharacter(msg);
    }
  }
  else
  {
    const float fTimeDiff = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds();
    const plVec3 vOwnerPos = GetOwner()->GetGlobalPosition();
    const plVec3 vDesiredNewPosition = vOwnerPos + vDirection * fSpeed * fTimeDiff;

    GetOwner()->SetGlobalPosition(vDesiredNewPosition);
  }
}

void plRcAgentComponent::SyncSteeringWithReality()
{
  const plRcPos rcCurrentAgentPosition = GetOwner()->GetGlobalPosition();

  if (!m_pCorridor->movePosition(rcCurrentAgentPosition, m_pQuery.Borrow(), &m_QueryFilter))
  {
    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = plAgentSteeringEvent::ErrorSteeringFailed;
    m_SteeringEvents.Broadcast(e);
    ClearTargetPosition();
    return;
  }

  const plRcPos rcPosOnNavmesh = m_pCorridor->getPos();
  m_vCurrentPositionOnNavmesh = rcPosOnNavmesh;

  /// \todo Check when these values diverge
}

void plRcAgentComponent::Update()
{
  // this can happen the first few frames
  if (InitializeRecast().Failed())
    return;

  // visualize various things
  {
    VisualizePathCorridorPosition();
    VisualizePathCorridor();
    VisualizeCurrentPath();
    VisualizeTargetPosition();
  }

  // target is set, but no path is computed yet
  if (GetPathToTargetState() == plAgentPathFindingState::HasTargetWaitingForPath)
  {
    if (ComputePathToTarget().Failed())
      return;

    PlanNextSteps();
  }

  // from here on down, everything has to do with following a valid path

  if (GetPathToTargetState() != plAgentPathFindingState::HasTargetAndValidPath)
    return;

  if (HasReachedGoal(1.0f))
  {
    plAgentSteeringEvent e;
    e.m_pComponent = this;
    e.m_Type = plAgentSteeringEvent::TargetReached;
    m_SteeringEvents.Broadcast(e);

    ClearTargetPosition();
    return;
  }

  ComputeSteeringDirection(1.0f);

  if (m_vCurrentSteeringDirection.IsZero())
  {
    /// \todo This would be some sort of error
    plLog::Error("Steering Direction is zero.");
    ClearTargetPosition();
    return;
  }

  ApplySteering(m_vCurrentSteeringDirection, m_fWalkSpeed);

  SyncSteeringWithReality();
}

void plRcAgentComponent::ComputeSteeringDirection(float fMaxDistance)
{
  plVec3 vCurPos = GetOwner()->GetGlobalPosition();
  vCurPos.z = 0;

  plInt32 iNextStep = 0;
  for (iNextStep = m_iNumNextSteps - 1; iNextStep >= m_iFirstNextStep; --iNextStep)
  {
    plVec3 step = m_vNextSteps[iNextStep];

    if (IsPositionVisible(step))
      break;
  }

  if (iNextStep < m_iFirstNextStep)
  {
    // plLog::Error("Next step not visible");
    iNextStep = m_iFirstNextStep;
  }

  if (iNextStep == m_iNumNextSteps - 1)
  {
    if (HasReachedPosition(m_vNextSteps[iNextStep], 1.0f))
    {
      PlanNextSteps();
      return; // reuse last steering direction
    }
  }

  m_iFirstNextStep = iNextStep;

  if (m_iFirstNextStep >= m_iNumNextSteps)
  {
    PlanNextSteps();
    return; // reuse last steering direction
  }


  plVec3 vDirection = m_vNextSteps[m_iFirstNextStep] - vCurPos;
  vDirection.z = 0;
  vDirection.NormalizeIfNotZero(plVec3::ZeroVector()).IgnoreResult();

  m_vCurrentSteeringDirection = vDirection;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

void plRcAgentComponent::VisualizePathCorridorPosition()
{
  if (GetPathToTargetState() != plAgentPathFindingState::HasTargetAndValidPath)
    return;

  const float* pos = m_pCorridor->getPos();
  const plVec3 vPos(pos[0], pos[2], pos[1]);

  plBoundingBox box;
  box.SetCenterAndHalfExtents(plVec3(0, 0, 1.0f), plVec3(0.3f, 0.3f, 1.0f));

  plTransform t;
  t.SetIdentity();
  t.m_vPosition = vPos;
  t.m_qRotation = GetOwner()->GetGlobalRotation();

  plDebugRenderer::DrawLineBox(GetWorld(), box, plColor::DarkGreen, t);
}

void plRcAgentComponent::VisualizePathCorridor()
{
  if (GetPathToTargetState() != plAgentPathFindingState::HasTargetAndValidPath)
    return;

  for (plUInt32 c = 0; c < m_PathCorridor.GetCount(); ++c)
  {
    dtPolyRef poly = m_PathCorridor[c];

    const dtMeshTile* pTile;
    const dtPoly* pPoly;
    m_pQuery->getAttachedNavMesh()->getTileAndPolyByRef(poly, &pTile, &pPoly);

    plHybridArray<plDebugRenderer::Triangle, 32> tris;

    for (plUInt32 i = 2; i < pPoly->vertCount; ++i)
    {
      plRcPos rcPos[3];
      rcPos[0] = &(pTile->verts[pPoly->verts[0] * 3]);
      rcPos[1] = &(pTile->verts[pPoly->verts[i - 1] * 3]);
      rcPos[2] = &(pTile->verts[pPoly->verts[i] * 3]);

      auto& tri = tris.ExpandAndGetRef();
      tri.m_position[0] = plVec3(rcPos[0]) + plVec3(0, 0, 0.1f);
      tri.m_position[1] = plVec3(rcPos[1]) + plVec3(0, 0, 0.1f);
      tri.m_position[2] = plVec3(rcPos[2]) + plVec3(0, 0, 0.1f);
    }

    plDebugRenderer::DrawSolidTriangles(GetWorld(), tris, plColor::OrangeRed.WithAlpha(0.4f));
  }
}

void plRcAgentComponent::VisualizeTargetPosition()
{
  if (GetPathToTargetState() == plAgentPathFindingState::HasNoTarget)
    return;

  plHybridArray<plDebugRenderer::Line, 16> lines;
  auto& line = lines.ExpandAndGetRef();

  line.m_start = m_vTargetPosition - plVec3(0, 0, 0.5f);
  line.m_end = m_vTargetPosition + plVec3(0, 0, 1.5f);

  plDebugRenderer::DrawLines(GetWorld(), lines, plColor::HotPink);
}

void plRcAgentComponent::VisualizeCurrentPath()
{
  if (GetPathToTargetState() != plAgentPathFindingState::HasTargetAndValidPath)
    return;

  plHybridArray<plDebugRenderer::Line, 16> lines;
  lines.Reserve(m_iNumNextSteps);

  plHybridArray<plDebugRenderer::Line, 16> steps;
  steps.Reserve(m_iNumNextSteps);

  /// \todo Hard-coded height offset
  plVec3 vPrev = GetOwner()->GetGlobalPosition() + plVec3(0, 0, 0.5f);
  for (plInt32 i = m_iFirstNextStep; i < m_iNumNextSteps; ++i)
  {
    auto& line = lines.ExpandAndGetRef();
    line.m_start = vPrev;
    line.m_end = m_vNextSteps[i] + plVec3(0, 0, 0.5f);
    vPrev = line.m_end;

    auto& step = steps.ExpandAndGetRef();
    step.m_start = m_vNextSteps[i];
    step.m_end = m_vNextSteps[i] + plVec3(0, 0, 1.0f);
  }

  plDebugRenderer::DrawLines(GetWorld(), lines, plColor::DarkViolet);
  plDebugRenderer::DrawLines(GetWorld(), steps, plColor::LightYellow);
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plRcAgentComponentManager::plRcAgentComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
{
}
plRcAgentComponentManager::~plRcAgentComponentManager() {}

void plRcAgentComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<plRecastWorldModule>();

  m_pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();

  auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plRcAgentComponentManager::Update, this);

  RegisterUpdateFunction(desc);

  plResourceManager::GetResourceEvents().AddEventHandler(plMakeDelegate(&plRcAgentComponentManager::ResourceEventHandler, this));
}

void plRcAgentComponentManager::Deinitialize()
{
  plResourceManager::GetResourceEvents().RemoveEventHandler(plMakeDelegate(&plRcAgentComponentManager::ResourceEventHandler, this));

  SUPER::Deinitialize();
}

void plRcAgentComponentManager::ResourceEventHandler(const plResourceEvent& e)
{
  if (e.m_Type == plResourceEvent::Type::ResourceContentUnloading && e.m_pResource->GetDynamicRTTI()->IsDerivedFrom<plRecastNavMeshResource>())
  {
    for (auto it = this->m_ComponentStorage.GetIterator(); it.IsValid(); ++it)
    {
      // make sure no agent references previous navmeshes
      it->UninitializeRecast();
    }
  }
}

void plRcAgentComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActiveAndSimulating())
      it->Update();
  }
}
