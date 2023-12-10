#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAiNavigationComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiNavmeshConfig")),
    PLASMA_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiPathSearchConfig")),
    PLASMA_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(5.0f)),
    PLASMA_MEMBER_PROPERTY("ReachedDistance", m_fReachedDistance)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PLASMA_MEMBER_PROPERTY("VisualizePathCorridor", m_bVisualizePathCorridor)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("VisualizePathLine", m_bVisualizePathLine)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Navigation"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination"),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plAiNavigationComponent::plAiNavigationComponent() = default;
plAiNavigationComponent::~plAiNavigationComponent() = default;

void plAiNavigationComponent::SetDestination(const plVec3& vGlobalPos)
{
  m_Navigation.SetTargetPosition(vGlobalPos);
  m_State = State::Active;
}

void plAiNavigationComponent::CancelNavigation()
{
  m_Navigation.CancelNavigation();
  m_State = State::Idle;
}

void plAiNavigationComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_fReachedDistance;
  s << m_fSpeed;
  s << m_bVisualizePathCorridor;
  s << m_bVisualizePathLine;
  s << m_bVisualizePathState;
}

void plAiNavigationComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  plStreamReader& s = inout_stream.GetStream();
   const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_fReachedDistance;
  s >> m_fSpeed;

  if(uiVersion >= 2)
  {
    s >> m_bVisualizePathCorridor;
    s >> m_bVisualizePathLine;
    s >> m_bVisualizePathState;
  }
}

void plAiNavigationComponent::Update()
{
  if (m_State != State::Active)
    return;

  if ((m_Navigation.GetTargetPosition() - m_Steering.m_vPosition).GetLengthSquared() < plMath::Square(m_fReachedDistance))
  {
    // reached the goal
    CancelNavigation();
    m_State = State::Idle;
    return;
  }

  if (plAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<plAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(*pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());

  m_Navigation.Update();

  if (m_Navigation.GetState() != plAiNavigation::State::FullPathFound)
    return;

  if (m_fSpeed <= 0)
    return;

  plVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  vForwardDir.NormalizeIfNotZero(plVec2(1, 0)).IgnoreResult();

  m_Steering.m_fMaxSpeed = m_fSpeed;
  m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  m_Steering.m_MinTurnSpeed = plAngle::MakeFromDegree(180);
  m_Steering.m_fAcceleration = 5;
  m_Steering.m_fDecceleration = 10;

  const float fBrakingDistance = 1.2f * (plMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  m_Steering.Calculate(GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds(), GetWorld());

  plVec3 vTargetPos = m_Steering.m_vPosition;
  vTargetPos.z = m_Navigation.GetCurrentElevation();

  // TODO: turn this into a dedicated component
  if (plPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>())
  {
    plPhysicsCastResult res;
    plPhysicsQueryParameters params(0, plPhysicsShapeType::Static);
    if (pPhysicsInterface->SweepTestSphere(res, 0.1f, vTargetPos + plVec3::MakeAxisZ(), -plVec3::MakeAxisZ(), 1.5f, params))
    {
      vTargetPos.z = res.m_vPosition.z;
    }

    m_Steering.m_vPosition.z = plMath::Lerp(GetOwner()->GetGlobalPosition().z, vTargetPos.z, plMath::Min(1.0f, 25.0f * GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds()));
  }

  GetOwner()->SetGlobalPosition(m_Steering.m_vPosition);
  GetOwner()->SetGlobalRotation(m_Steering.m_qRotation);

  if (m_bVisualizePathCorridor || m_bVisualizePathLine)
  {
    m_Navigation.DebugDraw(GetWorld(), m_bVisualizePathCorridor ? plColor::Aquamarine.WithAlpha(0.2f) : plColor::MakeZero(), m_bVisualizePathLine ? plColor::Lime : plColor::MakeZero());
  }

  if (m_bVisualizePathState)
  {
    const plAiNavigation::State state = m_Navigation.GetState();

    switch (state)
    {
      case plAiNavigation::State::Idle:
        plDebugRenderer::Draw3DText(GetWorld(), "Idle", GetOwner()->GetGlobalPosition(), plColor::Grey);
        break;
      case plAiNavigation::State::StartNewSearch:
        plDebugRenderer::Draw3DText(GetWorld(), "Starting...", GetOwner()->GetGlobalPosition(), plColor::Yellow);
        break;
      case plAiNavigation::State::InvalidCurrentPosition:
        plDebugRenderer::Draw3DText(GetWorld(), "Invalid Start Position", GetOwner()->GetGlobalPosition(), plColor::Black);
        break;
      case plAiNavigation::State::InvalidTargetPosition:
        plDebugRenderer::Draw3DText(GetWorld(), "Invalid Target Position", GetOwner()->GetGlobalPosition(), plColor::IndianRed);
        break;
      case plAiNavigation::State::NoPathFound:
        plDebugRenderer::Draw3DText(GetWorld(), "No Path Found", GetOwner()->GetGlobalPosition(), plColor::White);
        break;
      case plAiNavigation::State::PartialPathFound:
        plDebugRenderer::Draw3DText(GetWorld(), "Partial Path Found", GetOwner()->GetGlobalPosition(), plColor::Turquoise);
        break;
      case plAiNavigation::State::FullPathFound:
        plDebugRenderer::Draw3DText(GetWorld(), "Full Path Found", GetOwner()->GetGlobalPosition(), plColor::LawnGreen);
        break;
      case plAiNavigation::State::Searching:
        plDebugRenderer::Draw3DText(GetWorld(), "Searching...", GetOwner()->GetGlobalPosition(), plColor::Yellow);
        break;
    }
  }
}
