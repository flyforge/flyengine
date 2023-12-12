#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/AI/AgentSteeringComponent.h>
#include <Recast/DetourCrowd.h>
#include <RecastPlugin/Components/SoldierComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plSoldierComponent, 1, plComponentMode::Dynamic)
PLASMA_END_COMPONENT_TYPE
// clang-format on

plSoldierComponent::plSoldierComponent() = default;
plSoldierComponent::~plSoldierComponent() = default;

void plSoldierComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  // plStreamWriter& s = stream.GetStream();
}

void plSoldierComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  // plStreamReader& s = stream.GetStream();
}

void plSoldierComponent::OnSimulationStarted()
{
  plAgentSteeringComponent* pSteering = nullptr;
  if (GetOwner()->TryGetComponentOfBaseType<plAgentSteeringComponent>(pSteering))
  {
    m_hSteeringComponent = pSteering->GetHandle();

    pSteering->m_SteeringEvents.AddEventHandler(plMakeDelegate(&plSoldierComponent::SteeringEventHandler, this));
  }

  m_State = State::Idle;
}

void plSoldierComponent::Deinitialize()
{
  plAgentSteeringComponent* pSteering;
  if (GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
  {
    if (pSteering->m_SteeringEvents.HasEventHandler(plMakeDelegate(&plSoldierComponent::SteeringEventHandler, this)))
    {
      pSteering->m_SteeringEvents.RemoveEventHandler(plMakeDelegate(&plSoldierComponent::SteeringEventHandler, this));
    }
  }

  SUPER::Deinitialize();
}

void plSoldierComponent::Update()
{
  if (!IsActiveAndSimulating())
    return;

  if (m_State == State::Idle)
  {
    plRecastWorldModule* pRecastModule = GetWorld()->GetOrCreateModule<plRecastWorldModule>();

    if (pRecastModule == nullptr)
      return;

    plAgentSteeringComponent* pSteering = nullptr;
    if (!GetWorld()->TryGetComponent(m_hSteeringComponent, pSteering))
      return;

    const auto pPoiGraph = pRecastModule->GetNavMeshPointsOfInterestGraph();

    if (pPoiGraph == nullptr)
      return;


    plVec3 vNewTargetPos;
    bool bFoundAny = false;

    {
      const auto& graph = pPoiGraph->GetGraph();

      const plUInt32 uiTimestamp = pPoiGraph->GetCheckVisibilityTimeStamp() - 10;

      const plVec3 vOwnPos = GetOwner()->GetGlobalPosition();

      plDynamicArray<plUInt32> points;
      graph.FindPointsOfInterest(vOwnPos, 10.0, points);

      float fBestDistance = 1000;

      for (plUInt32 i = 0; i < points.GetCount(); ++i)
      {
        const plUInt32 marker = graph.GetPoints()[points[i]].m_uiVisibleMarker;

        const bool bHalfVisible = (marker >= uiTimestamp) && ((marker & 3U) == 2);
        const bool bInvisible = (marker < uiTimestamp) || ((marker & 3U) == 0);

        const plVec3 ptPos = graph.GetPoints()[points[i]].m_vFloorPosition;
        const float fDist = (vOwnPos - ptPos).GetLength();

        if (bHalfVisible) // top visible, bottom invisible
        {
          if (fDist < fBestDistance)
          {
            bFoundAny = true;
            fBestDistance = fDist;
            vNewTargetPos = ptPos;
          }
        }

        if (bInvisible)
        {
          if (fDist * 2 < fBestDistance)
          {
            bFoundAny = true;
            fBestDistance = fDist * 2;
            vNewTargetPos = ptPos;
          }
        }
      }
    }

    if (bFoundAny)
    {
      pSteering->SetTargetPosition(vNewTargetPos);
      m_State = State::WaitingForPath;
    }
  }
}

void plSoldierComponent::SteeringEventHandler(const plAgentSteeringEvent& e)
{
  switch (e.m_Type)
  {
    case plAgentSteeringEvent::TargetReached:
    case plAgentSteeringEvent::TargetCleared:
    case plAgentSteeringEvent::ErrorInvalidTargetPosition:
    case plAgentSteeringEvent::ErrorNoPathToTarget:
    case plAgentSteeringEvent::WarningNoFullPathToTarget:
    {
      e.m_pComponent->ClearTargetPosition();
      m_State = State::Idle;
    }
    break;

    case plAgentSteeringEvent::PathToTargetFound:
    {
      m_State = State::Walking;
    }
    break;

    case plAgentSteeringEvent::ErrorOutsideNavArea:
    case plAgentSteeringEvent::ErrorSteeringFailed:
    {
      PLASMA_ASSERT_DEV(m_State != State::ErrorState, "Multi-error state?");

      e.m_pComponent->ClearTargetPosition();

      m_State = State::ErrorState;
      plLog::Error("NPC is now in error state");
    }
    break;
  }
}
