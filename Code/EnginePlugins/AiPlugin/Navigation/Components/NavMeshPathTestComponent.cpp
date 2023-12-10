#include <AiPlugin/AiPluginPCH.h>
#include <AiPlugin/Navigation/Components/NavMeshPathTestComponent.h>
#include <AiPlugin/Navigation/NavMesh.h>
#include <AiPlugin/Navigation/NavMeshWorldModule.h>
#include <AiPlugin/Navigation/Navigation.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plAiNavMeshPathTestComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("VisualizePathCorridor", m_bVisualizePathCorridor)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("VisualizePathLine", m_bVisualizePathLine)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("VisualizePathState", m_bVisualizePathState)->AddAttributes(new plDefaultValueAttribute(true)),

    PLASMA_ACCESSOR_PROPERTY("PathEnd", DummyGetter, SetPathEndReference)->AddAttributes(new plGameObjectReferenceAttribute()),
    PLASMA_MEMBER_PROPERTY("NavmeshConfig", m_sNavmeshConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiNavmeshConfig")),
    PLASMA_MEMBER_PROPERTY("PathSearchConfig", m_sPathSearchConfig)->AddAttributes(new plDynamicStringEnumAttribute("AiPathSearchConfig")),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Navigation"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
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
}
