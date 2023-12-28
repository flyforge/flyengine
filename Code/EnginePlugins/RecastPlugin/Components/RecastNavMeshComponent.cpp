#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Configuration/CVar.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Time/Stopwatch.h>
#include <Recast/Recast.h>
#include <RecastPlugin/Components/RecastNavMeshComponent.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RecastPlugin/WorldModule/RecastWorldModule.h>
#include <RendererCore/Debug/DebugRenderer.h>

plCVarBool cvar_RecastVisNavMeshes("Recast.VisNavMeshes", false, plCVarFlags::Default, "Draws the navmesh, if one is available");

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plRcComponent, 1)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("AI/Recast"),
  }
  PLASMA_END_ATTRIBUTES;
}

PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plRcComponent::plRcComponent() = default;
plRcComponent::~plRcComponent() = default;

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plRcNavMeshComponent, 2, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plLongOpAttribute("plLongOpProxy_BuildNavMesh"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ShowNavMesh", m_bShowNavMesh),
    PLASMA_MEMBER_PROPERTY("NavMeshConfig", m_NavMeshConfig),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_FUNCTION_PROPERTY(OnObjectCreated),
  }
  PLASMA_END_FUNCTIONS;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plRcNavMeshComponent::plRcNavMeshComponent() {}
plRcNavMeshComponent::~plRcNavMeshComponent() {}

void plRcNavMeshComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  plStreamWriter& s = stream.GetStream();

  s << m_bShowNavMesh;
  s << m_hNavMesh;
}

void plRcNavMeshComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = stream.GetStream();

  s >> m_bShowNavMesh;

  if (uiVersion >= 2)
  {
    s >> m_hNavMesh;
  }
}

void plRcNavMeshComponent::OnObjectCreated(const plAbstractObjectNode& node)
{
  plStringBuilder sComponentGuid, sNavMeshFile;
  plConversionUtils::ToString(node.GetGuid(), sComponentGuid);

  // this is where the editor will put the file for this component
  sNavMeshFile.Format(":project/AssetCache/Generated/{0}.plRecastNavMesh", sComponentGuid);

  m_hNavMesh = plResourceManager::LoadResource<plRecastNavMeshResource>(sNavMeshFile);
}

void plRcNavMeshComponent::Update()
{
  VisualizeNavMesh();
  VisualizePointsOfInterest();
}

PLASMA_ALWAYS_INLINE static plVec3 GetNavMeshVertex(
  const rcPolyMesh* pMesh, plUInt16 uiVertex, const plVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const plUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return plVec3(x, y, z);
}

void plRcNavMeshComponent::OnActivated()
{
  if (m_hNavMesh.IsValid())
  {
    GetWorld()->GetOrCreateModule<plRecastWorldModule>()->SetNavMeshResource(m_hNavMesh);
  }
}

void plRcNavMeshComponent::VisualizeNavMesh()
{
  if (!m_bShowNavMesh && !cvar_RecastVisNavMeshes)
    return;

  auto hNavMesh = GetWorld()->GetOrCreateModule<plRecastWorldModule>()->GetNavMeshResource();
  if (!hNavMesh.IsValid())
    return;

  plResourceLock<plRecastNavMeshResource> pNavMesh(hNavMesh, plResourceAcquireMode::BlockTillLoaded_NeverFail);
  if (pNavMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    return;

  const auto* pMesh = pNavMesh->GetNavMeshPolygons();

  if (pMesh == nullptr)
    return;

  plDynamicArray<plDebugRenderer::Triangle> triangles;
  triangles.Reserve(pMesh->npolys * 3);

  plDynamicArray<plDebugRenderer::Line> contourLines;
  contourLines.Reserve(pMesh->npolys * 2);
  plDynamicArray<plDebugRenderer::Line> innerLines;
  innerLines.Reserve(pMesh->npolys * 3);

  const plInt32 iMaxNumVertInPoly = pMesh->nvp;
  const float fCellSize = pMesh->cs;
  const float fCellHeight = pMesh->ch;
  // add a little height offset to move the visualization up a little
  const plVec3 vMeshOrigin(pMesh->bmin[0], pMesh->bmin[2], pMesh->bmin[1] + fCellHeight * 0.3f);

  for (plInt32 i = 0; i < pMesh->npolys; ++i)
  {
    const plUInt16* polyVtxIndices = &pMesh->polys[i * (iMaxNumVertInPoly * 2)];
    const plUInt16* neighborData = &pMesh->polys[i * (iMaxNumVertInPoly * 2) + iMaxNumVertInPoly];

    // const plUInt8 areaType = pMesh->areas[i];
    // if (areaType == RC_WALKABLE_AREA)
    //  color = duRGBA(0, 192, 255, 64);
    // else if (areaType == RC_NULL_AREA)
    //  color = duRGBA(0, 0, 0, 64);
    // else
    //  color = dd->areaToCol(area);

    plInt32 j;
    for (j = 1; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool bIsContour = neighborData[j - 1] == 0xffff;

      {
        auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
        line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
        line.m_end = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      }
    }

    // close the loop
    const bool bIsContour = neighborData[j - 1] == 0xffff;
    {
      auto& line = bIsContour ? contourLines.ExpandAndGetRef() : innerLines.ExpandAndGetRef();
      line.m_start = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      line.m_end = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
    }

    for (j = 2; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      auto& triangle = triangles.ExpandAndGetRef();

      triangle.m_position[0] = GetNavMeshVertex(pMesh, polyVtxIndices[0], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_position[2] = GetNavMeshVertex(pMesh, polyVtxIndices[j - 1], vMeshOrigin, fCellSize, fCellHeight);
      triangle.m_position[1] = GetNavMeshVertex(pMesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
    }
  }

  plDebugRenderer::DrawSolidTriangles(GetWorld(), triangles, plColor::CadetBlue.WithAlpha(0.25f));
  plDebugRenderer::DrawLines(GetWorld(), contourLines, plColor::DarkOrange);
  plDebugRenderer::DrawLines(GetWorld(), innerLines, plColor::CadetBlue);
}

void plRcNavMeshComponent::VisualizePointsOfInterest()
{
  if (!m_bShowNavMesh && !cvar_RecastVisNavMeshes)
    return;

  auto pPoiGraph = GetWorld()->GetOrCreateModule<plRecastWorldModule>()->GetNavMeshPointsOfInterestGraph();

  if (pPoiGraph == nullptr)
    return;

  const auto& poi = *pPoiGraph;
  const auto& graph = poi.GetGraph();

  const plUInt32 uiCheckTimeStamp = poi.GetCheckVisibilityTimeStamp();
  const plUInt32 uiConsiderInvisibleTimeStamp = uiCheckTimeStamp - 20;

  plDynamicArray<plDebugRenderer::Line> visibleLines;
  plDynamicArray<plDebugRenderer::Line> hiddenLines;

  for (const auto& point : graph.GetPoints())
  {
    if (point.m_uiVisibleMarker < uiConsiderInvisibleTimeStamp || (point.m_uiVisibleMarker & 3U) == 0)
    {
      // not updated for too long -> consider invisible

      auto& line = hiddenLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + plVec3(0, 0, 1.8f);
      continue;
    }

    if ((point.m_uiVisibleMarker & 3U) == 3U) // fully visible
    {
      auto& line = visibleLines.ExpandAndGetRef();

      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + plVec3(0, 0, 1.8f);
      continue;
    }

    // else bottom half invisible
    {
      auto& line = hiddenLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition;
      line.m_end = point.m_vFloorPosition + plVec3(0, 0, 1.0f);
    }

    // top half visible
    {
      auto& line = visibleLines.ExpandAndGetRef();
      line.m_start = point.m_vFloorPosition + plVec3(0, 0, 1.0f);
      line.m_end = point.m_vFloorPosition + plVec3(0, 0, 1.8f);
    }
  }

  plDebugRenderer::DrawLines(GetWorld(), visibleLines, plColor::DeepSkyBlue);
  plDebugRenderer::DrawLines(GetWorld(), hiddenLines, plColor::SlateGrey);
}

//////////////////////////////////////////////////////////////////////////

plRcNavMeshComponentManager::plRcNavMeshComponentManager(plWorld* pWorld)
  : SUPER(pWorld)
  , m_pWorldModule(nullptr)
{
}

plRcNavMeshComponentManager::~plRcNavMeshComponentManager() = default;

void plRcNavMeshComponentManager::Initialize()
{
  SUPER::Initialize();

  // make sure this world module exists
  m_pWorldModule = GetWorld()->GetOrCreateModule<plRecastWorldModule>();

  auto desc = PLASMA_CREATE_MODULE_UPDATE_FUNCTION_DESC(plRcNavMeshComponentManager::Update, this);
  desc.m_bOnlyUpdateWhenSimulating = false;

  RegisterUpdateFunction(desc);
}

void plRcNavMeshComponentManager::Update(const plWorldModule::UpdateContext& context)
{
  for (auto it = this->m_ComponentStorage.GetIterator(context.m_uiFirstComponentIndex, context.m_uiComponentCount); it.IsValid(); ++it)
  {
    if (it->IsActive())
      it->Update();
  }
}
