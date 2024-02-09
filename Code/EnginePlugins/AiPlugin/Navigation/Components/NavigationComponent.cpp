#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavigationComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAiNavigationComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiNavmeshConfig")),
    PL_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiPathSearchConfig")),
    PL_MEMBER_PROPERTY("Speed", m_fSpeed)->AddAttributes(new plDefaultValueAttribute(5.0f)),
    PL_MEMBER_PROPERTY("ReachedDistance", m_fReachedDistance)->AddAttributes(new plDefaultValueAttribute(1.0f)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Navigation"),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(SetDestination, In, "Destination"),
    PL_SCRIPT_FUNCTION_PROPERTY(CancelNavigation),
  }
  PL_END_FUNCTIONS;
}
PL_END_COMPONENT_TYPE
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
}

void plAiNavigationComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  plStreamReader& s = inout_stream.GetStream();
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_fReachedDistance;
  s >> m_fSpeed;
}

void plAiNavigationComponent::Update()
{
  plTransform transform = GetOwner()->GetGlobalTransform();
  Steer(transform);
  PlaceOnGround(transform);

  GetOwner()->SetGlobalPosition(transform.m_vPosition);
  GetOwner()->SetGlobalRotation(transform.m_qRotation);
}

void plAiNavigationComponent::Steer(plTransform& transform)
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
  //vTargetPos.z = m_Navigation.GetCurrentElevation();

  transform.m_vPosition = vTargetPos;
  transform.m_qRotation = m_Steering.m_qRotation;
}

void plAiNavigationComponent::PlaceOnGround(plTransform& transform)
{
  plPhysicsWorldModuleInterface* pPhysicsInterface = GetWorld()->GetOrCreateModule<plPhysicsWorldModuleInterface>();

  if (pPhysicsInterface == nullptr)
    return;

  const float tDiff = GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds();

  const plVec3 vDown = -plVec3::MakeAxisZ();
  const float fRadius = 0.15f;
  const float fHalfDistance = 1.0f;
  const plVec3 vStartPos = transform.m_vPosition - fHalfDistance * vDown;
  const plVec3 vEndPos = transform.m_vPosition + fHalfDistance * vDown;

  plVec3 vTargetPos = vEndPos;

  plDebugRenderer::DrawLineSphere(GetWorld(), plBoundingSphere::MakeFromCenterAndRadius(vStartPos, fRadius), plColor::CadetBlue);
  plDebugRenderer::DrawLineSphere(GetWorld(), plBoundingSphere::MakeFromCenterAndRadius(vEndPos, fRadius), plColor::RebeccaPurple);

  plPhysicsCastResult res;
  plPhysicsQueryParameters params(0, plPhysicsShapeType::Static);
  if (pPhysicsInterface->SweepTestSphere(res, fRadius, vStartPos, vDown, fHalfDistance * 2.0f, params))
  {
    plDebugRenderer::DrawCross(GetWorld(), res.m_vPosition, 0.2f, plColor::GreenYellow);

    vTargetPos.z = res.m_vPosition.z;
  }
  else
  {
    vTargetPos.z = vEndPos.z + vDown.z * fRadius;
  }

  transform.m_vPosition.z = plMath::Lerp(transform.m_vPosition.z, vTargetPos.z, plMath::Min(1.0f, 25.0f * tDiff));
}
