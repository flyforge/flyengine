#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Memory/FrameAllocator.h>
#include <RecastPlugin/Components/MarkPoiVisibleComponent.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plRcMarkPoiVisibleComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("Radius", m_fRadius)->AddAttributes(new plDefaultValueAttribute(20.0f)),
    PL_MEMBER_PROPERTY("CollisionLayer", m_uiCollisionLayer)->AddAttributes(new plDynamicEnumAttribute("PhysicsCollisionLayer")),
  }
  PL_END_PROPERTIES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plRcMarkPoiVisibleComponent::plRcMarkPoiVisibleComponent() = default;
plRcMarkPoiVisibleComponent::~plRcMarkPoiVisibleComponent() = default;

void plRcMarkPoiVisibleComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_uiCollisionLayer;
}

void plRcMarkPoiVisibleComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_uiCollisionLayer;
}

static const plInt32 g_iMaxPointsToCheckPerFrame = 30;

void plRcMarkPoiVisibleComponent::Update()
{
  if (!IsActiveAndSimulating() || m_pWorldModule == nullptr)
    return;

  if (m_pPhysicsModule == nullptr)
  {
    m_pPhysicsModule = GetWorld()->GetModule<plPhysicsWorldModuleInterface>();

    if (m_pPhysicsModule == nullptr)
      return;
  }

  const plVec3 vOwnPos = GetOwner()->GetGlobalPosition();

  auto pPoiGraph = m_pWorldModule->AccessNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const plUInt32 uiCheckTimeStamp = pPoiGraph->GetCheckVisibilityTimeStamp();
  const plUInt32 uiTimeStampFullyVisible = uiCheckTimeStamp | 3U;
  const plUInt32 uiTimeStampTopVisible = uiCheckTimeStamp | 2U;

  const plUInt32 uiSkipCheckTimeStamp = uiCheckTimeStamp - 10;

  auto& graph = pPoiGraph->GetGraph();
  auto& POIs = graph.AccessPoints();

  plDynamicArray<plUInt32> points(plFrameAllocator::GetCurrentAllocator());
  graph.FindPointsOfInterest(vOwnPos, 20.0f, points);

  plInt32 iPointsToCheck = g_iMaxPointsToCheckPerFrame;

  for (plUInt32 i = 0; i < points.GetCount(); ++i)
  {
    ++m_uiLastFirstCheckedPoint;

    if (m_uiLastFirstCheckedPoint >= points.GetCount())
      m_uiLastFirstCheckedPoint = 0;

    auto& poi = POIs[points[m_uiLastFirstCheckedPoint]];

    if (poi.m_uiVisibleMarker >= uiSkipCheckTimeStamp)
      continue;

    if (--iPointsToCheck <= 0)
      break;

    const plVec3 vTargetBottom = poi.m_vFloorPosition + plVec3(0, 0, 0.5f);
    plVec3 vDirToBottom = vTargetBottom - vOwnPos;
    const float fRayLenBottom = vDirToBottom.GetLengthAndNormalize();

    plPhysicsCastResult hit;

    if (m_pPhysicsModule->Raycast(hit, vOwnPos, vDirToBottom, fRayLenBottom, plPhysicsQueryParameters(m_uiCollisionLayer, plPhysicsShapeType::Static)))
    {
      const plVec3 vTargetTop = poi.m_vFloorPosition + plVec3(0, 0, 1.0f);
      plVec3 vDirToTop = vTargetTop - vOwnPos;
      const float fRayLenTop = vDirToTop.GetLengthAndNormalize();

      --iPointsToCheck;

      plPhysicsCastResult hit2;
      if (m_pPhysicsModule->Raycast(hit2, vOwnPos, vDirToTop, fRayLenTop, plPhysicsQueryParameters(m_uiCollisionLayer, plPhysicsShapeType::Static)))
      {
        poi.m_uiVisibleMarker = uiCheckTimeStamp;
      }
      else
      {
        poi.m_uiVisibleMarker = uiTimeStampTopVisible;
      }
    }
    else
    {
      poi.m_uiVisibleMarker = uiTimeStampFullyVisible;
    }
  }
}

void plRcMarkPoiVisibleComponent::OnSimulationStarted()
{
  m_pWorldModule = GetWorld()->GetOrCreateModule<plRecastWorldModule>();
}
