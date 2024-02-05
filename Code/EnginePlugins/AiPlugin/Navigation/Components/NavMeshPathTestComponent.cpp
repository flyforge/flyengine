#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshPathTestComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAiNavMeshPathTestComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("VisualizePathCorridor", m_bVisualizePathCorridor)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("VisualizePathLine", m_bVisualizePathLine)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new plDefaultValueAttribute(true)),

    PL_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PL_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiNavmeshConfig")),
    PL_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiPathSearchConfig")),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Navigation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plAiNavMeshPathTestComponent::plAiNavMeshPathTestComponent() = default;
plAiNavMeshPathTestComponent::~plAiNavMeshPathTestComponent() = default;

void plAiNavMeshPathTestComponent::SetPathEndReference(const char* szReference)
{
  auto resolver = GetWorld()->GetGameObjectReferenceResolver();

  if (!resolver.IsValid())
    return;

  SetPathEnd(resolver(szReference, GetHandle(), "PathEnd"));
}

void plAiNavMeshPathTestComponent::SetPathEnd(plGameObjectHandle hObject)
{
  m_hPathEnd = hObject;
}

void plAiNavMeshPathTestComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  inout_stream.WriteGameObjectHandle(m_hPathEnd);
  s << m_sPathSearchConfig;
  s << m_sNavmeshConfig;
  s << m_bVisualizePathCorridor;
  s << m_bVisualizePathLine;
  s << m_bVisualizePathState;
}

void plAiNavMeshPathTestComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  plStreamReader& s = inout_stream.GetStream();
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  m_hPathEnd = inout_stream.ReadGameObjectHandle();
  s >> m_sPathSearchConfig;
  s >> m_sNavmeshConfig;
  s >> m_bVisualizePathCorridor;
  s >> m_bVisualizePathLine;
  s >> m_bVisualizePathState;
}

void plAiNavMeshPathTestComponent::Update()
{
  if (m_hPathEnd.IsInvalidated())
    return;

  plGameObject* pEnd = nullptr;
  if (!GetWorld()->TryGetObject(m_hPathEnd, pEnd))
    return;

  m_Navigation.SetCurrentPosition(GetOwner()->GetGlobalPosition());
  m_Navigation.SetTargetPosition(pEnd->GetGlobalPosition());

  if (plAiNavMeshWorldModule* pNavMeshModule = GetWorld()->GetOrCreateModule<plAiNavMeshWorldModule>())
  {
    m_Navigation.SetNavmesh(*pNavMeshModule->GetNavMesh(m_sNavmeshConfig));
    m_Navigation.SetQueryFilter(pNavMeshModule->GetPathSearchFilter(m_sPathSearchConfig));
  }

  m_Navigation.Update();

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

  // if (m_fSpeed <= 0)
  //   return;

  // plVec2 vForwardDir = GetOwner()->GetGlobalDirForwards().GetAsVec2();
  // vForwardDir.NormalizeIfNotZero(plVec2(1, 0)).IgnoreResult();

  // m_Steering.m_vPosition = GetOwner()->GetGlobalPosition();
  // m_Steering.m_qRotation = GetOwner()->GetGlobalRotation();
  // m_Steering.m_vVelocity = GetOwner()->GetLinearVelocity();
  // m_Steering.m_fMaxSpeed = m_fSpeed;
  // m_Steering.m_MinTurnSpeed = m_TurnSpeed;
  // m_Steering.m_fAcceleration = m_fAcceleration;
  // m_Steering.m_fDecceleration = m_fDecceleration;

  // const float fBrakingDistance = 1.2f * (plMath::Square(m_Steering.m_fMaxSpeed) / (2.0f * m_Steering.m_fDecceleration));

  // m_Navigation.ComputeSteeringInfo(m_Steering.m_Info, vForwardDir, fBrakingDistance);
  // m_Steering.Calculate(GetWorld()->GetClock().GetTimeDiff().AsFloatInSeconds(), GetWorld());

  // m_Steering.m_vPosition.z = m_Navigation.GetCurrentElevation();

  // GetOwner()->SetGlobalPosition(m_Steering.m_vPosition);
  // GetOwner()->SetGlobalRotation(m_Steering.m_qRotation);
}
