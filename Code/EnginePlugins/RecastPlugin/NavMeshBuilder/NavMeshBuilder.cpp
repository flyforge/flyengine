#include <RecastPlugin/RecastPluginPCH.h>

#include <Core/World/World.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshBuilder.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Types/ScopeExit.h>
#include <Foundation/Utilities/GraphicsUtils.h>
#include <Foundation/Utilities/Progress.h>
#include <Recast.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshBuilder.h>
#include <RecastPlugin/Resources/RecastNavMeshResource.h>
#include <RendererCore/Meshes/CpuMeshResource.h>
#include <RendererCore/Meshes/MeshBufferUtils.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plRecastConfig, plNoBase, 1, plRTTIDefaultAllocator<plRecastConfig>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("AgentHeight", m_fAgentHeight)->AddAttributes(new plDefaultValueAttribute(1.5f)),
    PL_MEMBER_PROPERTY("AgentRadius", m_fAgentRadius)->AddAttributes(new plDefaultValueAttribute(0.3f)),
    PL_MEMBER_PROPERTY("AgentClimbHeight", m_fAgentClimbHeight)->AddAttributes(new plDefaultValueAttribute(0.4f)),
    PL_MEMBER_PROPERTY("WalkableSlope", m_WalkableSlope)->AddAttributes(new plDefaultValueAttribute(plAngle::MakeFromDegree(45))),
    PL_MEMBER_PROPERTY("CellSize", m_fCellSize)->AddAttributes(new plDefaultValueAttribute(0.2f)),
    PL_MEMBER_PROPERTY("CellHeight", m_fCellHeight)->AddAttributes(new plDefaultValueAttribute(0.2f)),
    PL_MEMBER_PROPERTY("MinRegionSize", m_fMinRegionSize)->AddAttributes(new plDefaultValueAttribute(3.0f)),
    PL_MEMBER_PROPERTY("RegionMergeSize", m_fRegionMergeSize)->AddAttributes(new plDefaultValueAttribute(20.0f)),
    PL_MEMBER_PROPERTY("SampleDistanceFactor", m_fDetailMeshSampleDistanceFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("SampleErrorFactor", m_fDetailMeshSampleErrorFactor)->AddAttributes(new plDefaultValueAttribute(1.0f)),
    PL_MEMBER_PROPERTY("MaxSimplification", m_fMaxSimplificationError)->AddAttributes(new plDefaultValueAttribute(1.3f)),
    PL_MEMBER_PROPERTY("MaxEdgeLength", m_fMaxEdgeLength)->AddAttributes(new plDefaultValueAttribute(4.0f)),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

class plRcBuildContext : public rcContext
{
public:
  plRcBuildContext() = default;

protected:
  virtual void doLog(const rcLogCategory category, const char* msg, const int len)
  {
    switch (category)
    {
      case RC_LOG_ERROR:
        plLog::Error("Recast: {0}", msg);
        return;
      case RC_LOG_WARNING:
        plLog::Warning("Recast: {0}", msg);
        return;
      case RC_LOG_PROGRESS:
        plLog::Debug("Recast: {0}", msg);
        return;

      default:
        plLog::Error("Unknwon recast log: {0}", msg);
        return;
    }
  }
};

plRecastNavMeshBuilder::plRecastNavMeshBuilder() = default;
plRecastNavMeshBuilder::~plRecastNavMeshBuilder() = default;

void plRecastNavMeshBuilder::Clear()
{
  m_BoundingBox = plBoundingBox::MakeInvalid();
  m_Vertices.Clear();
  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_pRecastContext = nullptr;
}

plResult plRecastNavMeshBuilder::ExtractWorldGeometry(const plWorld& world, plWorldGeoExtractionUtil::MeshObjectList& out_worldGeo)
{
  plWorldGeoExtractionUtil::ExtractWorldGeometry(out_worldGeo, world, plWorldGeoExtractionUtil::ExtractionMode::NavMeshGeneration);

  return PL_SUCCESS;
}

plResult plRecastNavMeshBuilder::Build(const plRecastConfig& config, const plWorldGeoExtractionUtil::MeshObjectList& geo,
  plRecastNavMeshResourceDescriptor& out_navMeshDesc, plProgress& ref_progress)
{
  PL_LOG_BLOCK("plRecastNavMeshBuilder::Build");

  plProgressRange pg("Generating NavMesh", 4, true, &ref_progress);
  pg.SetStepWeighting(0, 0.1f);
  pg.SetStepWeighting(1, 0.1f);
  pg.SetStepWeighting(2, 0.6f);
  pg.SetStepWeighting(3, 0.2f);

  Clear();
  out_navMeshDesc.Clear();

  plUniquePtr<plRcBuildContext> recastContext = PL_DEFAULT_NEW(plRcBuildContext);
  m_pRecastContext = recastContext.Borrow();

  if (!pg.BeginNextStep("Triangulate Mesh"))
    return PL_FAILURE;

  GenerateTriangleMeshFromDescription(geo);

  if (m_Vertices.IsEmpty())
  {
    plLog::Debug("Navmesh is empty");
    return PL_SUCCESS;
  }

  if (!pg.BeginNextStep("Compute AABB"))
    return PL_FAILURE;

  ComputeBoundingBox();

  if (!pg.BeginNextStep("Build Poly Mesh"))
    return PL_FAILURE;

  out_navMeshDesc.m_pNavMeshPolygons = PL_DEFAULT_NEW(rcPolyMesh);

  if (BuildRecastPolyMesh(config, *out_navMeshDesc.m_pNavMeshPolygons, ref_progress).Failed())
    return PL_FAILURE;

  if (!pg.BeginNextStep("Build NavMesh"))
    return PL_FAILURE;

  if (BuildDetourNavMeshData(config, *out_navMeshDesc.m_pNavMeshPolygons, out_navMeshDesc.m_DetourNavmeshData).Failed())
    return PL_FAILURE;

  return PL_SUCCESS;
}

void plRecastNavMeshBuilder::GenerateTriangleMeshFromDescription(const plWorldGeoExtractionUtil::MeshObjectList& objects)
{
  PL_LOG_BLOCK("plRecastNavMeshBuilder::GenerateTriangleMesh");

  m_Triangles.Clear();
  m_TriangleAreaIDs.Clear();
  m_Vertices.Clear();


  plUInt32 uiVertexOffset = 0;
  for (const plWorldGeoExtractionUtil::MeshObject& object : objects)
  {
    plResourceLock<plCpuMeshResource> pCpuMesh(object.m_hMeshResource, plResourceAcquireMode::BlockTillLoaded_NeverFail);
    if (pCpuMesh.GetAcquireResult() != plResourceAcquireResult::Final)
    {
      continue;
    }

    const auto& meshBufferDesc = pCpuMesh->GetDescriptor().MeshBufferDesc();

    m_Triangles.Reserve(m_Triangles.GetCount() + meshBufferDesc.GetPrimitiveCount());
    m_Vertices.Reserve(m_Vertices.GetCount() + meshBufferDesc.GetVertexCount());

    const plVec3* pPositions = nullptr;
    plUInt32 uiElementStride = 0;
    if (plMeshBufferUtils::GetPositionStream(meshBufferDesc, pPositions, uiElementStride).Failed())
    {
      continue;
    }

    // convert from pl convention (Z up) to recast convention (Y up)
    plMat3 m;
    m.SetRow(0, plVec3(1, 0, 0));
    m.SetRow(1, plVec3(0, 0, 1));
    m.SetRow(2, plVec3(0, 1, 0));

    plMat4 transform = plMat4::MakeIdentity();
    transform.SetRotationalPart(m);
    transform = transform * object.m_GlobalTransform.GetAsMat4();

    // collect all vertices
    for (plUInt32 i = 0; i < meshBufferDesc.GetVertexCount(); ++i)
    {
      plVec3 pos = transform.TransformPosition(*pPositions);

      m_Vertices.PushBack(pos);

      pPositions = plMemoryUtils::AddByteOffset(pPositions, uiElementStride);
    }

    // collect all indices
    bool flip = plGraphicsUtils::IsTriangleFlipRequired(transform.GetRotationalPart());

    if (meshBufferDesc.HasIndexBuffer())
    {
      if (meshBufferDesc.Uses32BitIndices())
      {
        const plUInt32* pTypedIndices = reinterpret_cast<const plUInt32*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
      else
      {
        const plUInt16* pTypedIndices = reinterpret_cast<const plUInt16*>(meshBufferDesc.GetIndexBufferData().GetPtr());

        for (plUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
        {
          auto& triangle = m_Triangles.ExpandAndGetRef();
          triangle.m_VertexIdx[0] = pTypedIndices[p * 3 + (flip ? 2 : 0)] + uiVertexOffset;
          triangle.m_VertexIdx[1] = pTypedIndices[p * 3 + 1] + uiVertexOffset;
          triangle.m_VertexIdx[2] = pTypedIndices[p * 3 + (flip ? 0 : 2)] + uiVertexOffset;
        }
      }
    }
    else
    {
      plUInt32 uiVertexIdx = uiVertexOffset;

      for (plUInt32 p = 0; p < meshBufferDesc.GetPrimitiveCount(); ++p)
      {
        auto& triangle = m_Triangles.ExpandAndGetRef();
        triangle.m_VertexIdx[0] = uiVertexIdx + 0;
        triangle.m_VertexIdx[1] = uiVertexIdx + (flip ? 2 : 1);
        triangle.m_VertexIdx[2] = uiVertexIdx + (flip ? 1 : 2);

        uiVertexIdx += 3;
      }
    }

    uiVertexOffset += meshBufferDesc.GetVertexCount();
  }

  // initialize the IDs to zero
  m_TriangleAreaIDs.SetCount(m_Triangles.GetCount());

  plLog::Debug("Vertices: {0}, Triangles: {1}", m_Vertices.GetCount(), m_Triangles.GetCount());
}


void plRecastNavMeshBuilder::ComputeBoundingBox()
{
  if (!m_Vertices.IsEmpty())
  {
    m_BoundingBox = plBoundingBox::MakeFromPoints(m_Vertices.GetData(), m_Vertices.GetCount());
  }
}

void plRecastNavMeshBuilder::FillOutConfig(rcConfig& cfg, const plRecastConfig& config, const plBoundingBox& bbox)
{
  plMemoryUtils::ZeroFill(&cfg, 1);
  cfg.bmin[0] = bbox.m_vMin.x;
  cfg.bmin[1] = bbox.m_vMin.y;
  cfg.bmin[2] = bbox.m_vMin.z;
  cfg.bmax[0] = bbox.m_vMax.x;
  cfg.bmax[1] = bbox.m_vMax.y;
  cfg.bmax[2] = bbox.m_vMax.z;
  cfg.ch = config.m_fCellHeight;
  cfg.cs = config.m_fCellSize;
  cfg.walkableSlopeAngle = config.m_WalkableSlope.GetDegree();
  cfg.walkableHeight = (int)ceilf(config.m_fAgentHeight / cfg.ch);
  cfg.walkableClimb = (int)floorf(config.m_fAgentClimbHeight / cfg.ch);
  cfg.walkableRadius = (int)ceilf(config.m_fAgentRadius / cfg.cs);
  cfg.maxEdgeLen = (int)(config.m_fMaxEdgeLength / cfg.cs);
  cfg.maxSimplificationError = config.m_fMaxSimplificationError;
  cfg.minRegionArea = (int)plMath::Square(config.m_fMinRegionSize);
  cfg.mergeRegionArea = (int)plMath::Square(config.m_fRegionMergeSize);
  cfg.maxVertsPerPoly = 6;
  cfg.detailSampleDist = config.m_fDetailMeshSampleDistanceFactor < 0.9f ? 0 : cfg.cs * config.m_fDetailMeshSampleDistanceFactor;
  cfg.detailSampleMaxError = cfg.ch * config.m_fDetailMeshSampleErrorFactor;

  rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);
}

plResult plRecastNavMeshBuilder::BuildRecastPolyMesh(const plRecastConfig& config, rcPolyMesh& out_polyMesh, plProgress& progress)
{
  plProgressRange pgRange("Build Poly Mesh", 13, true, &progress);

  rcConfig cfg;
  FillOutConfig(cfg, config, m_BoundingBox);

  plRcBuildContext* pContext = m_pRecastContext;
  const float* pVertices = &m_Vertices[0].x;
  const plInt32* pTriangles = &m_Triangles[0].m_VertexIdx[0];

  rcHeightfield* heightfield = rcAllocHeightfield();
  PL_SCOPE_EXIT(rcFreeHeightField(heightfield));

  if (!pgRange.BeginNextStep("Creating Heightfield"))
    return PL_FAILURE;

  if (!rcCreateHeightfield(pContext, *heightfield, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch))
  {
    pContext->log(RC_LOG_ERROR, "Could not create solid heightfield");
    return PL_FAILURE;
  }

  if (!pgRange.BeginNextStep("Mark Walkable Area"))
    return PL_FAILURE;

  // TODO Instead of this, it should use area IDs and then clear the non-walkable triangles
  rcMarkWalkableTriangles(
    pContext, cfg.walkableSlopeAngle, pVertices, m_Vertices.GetCount(), pTriangles, m_Triangles.GetCount(), m_TriangleAreaIDs.GetData());

  if (!pgRange.BeginNextStep("Rasterize Triangles"))
    return PL_FAILURE;

  if (!rcRasterizeTriangles(
        pContext, pVertices, m_Vertices.GetCount(), pTriangles, m_TriangleAreaIDs.GetData(), m_Triangles.GetCount(), *heightfield, cfg.walkableClimb))
  {
    pContext->log(RC_LOG_ERROR, "Could not rasterize triangles");
    return PL_FAILURE;
  }

  // Optional stuff
  {
    if (!pgRange.BeginNextStep("Filter Low Hanging Obstacles"))
      return PL_FAILURE;

    // if (m_filterLowHangingObstacles)
    rcFilterLowHangingWalkableObstacles(pContext, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Ledge Spans"))
      return PL_FAILURE;

    // if (m_filterLedgeSpans)
    rcFilterLedgeSpans(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield);

    if (!pgRange.BeginNextStep("Filter Low Height Spans"))
      return PL_FAILURE;

    // if (m_filterWalkableLowHeightSpans)
    rcFilterWalkableLowHeightSpans(pContext, cfg.walkableHeight, *heightfield);
  }

  if (!pgRange.BeginNextStep("Build Compact Heightfield"))
    return PL_FAILURE;

  rcCompactHeightfield* compactHeightfield = rcAllocCompactHeightfield();
  PL_SCOPE_EXIT(rcFreeCompactHeightfield(compactHeightfield));

  if (!rcBuildCompactHeightfield(pContext, cfg.walkableHeight, cfg.walkableClimb, *heightfield, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not build compact data");
    return PL_FAILURE;
  }

  if (!pgRange.BeginNextStep("Erode Walkable Area"))
    return PL_FAILURE;

  if (!rcErodeWalkableArea(pContext, cfg.walkableRadius, *compactHeightfield))
  {
    pContext->log(RC_LOG_ERROR, "Could not erode with character radius");
    return PL_FAILURE;
  }

  // (Optional) Mark areas.
  //{
  //  const ConvexVolume* vols = m_geom->getConvexVolumes();
  //  for (int i = 0; i < m_geom->getConvexVolumeCount(); ++i)
  //    rcMarkConvexPolyArea(pContext, vols[i].verts, vols[i].nverts, vols[i].hmin, vols[i].hmax, (unsigned char)vols[i].area,
  //    *compactHeightfield);
  //}


  // Partition the heightfield so that we can use simple algorithm later to triangulate the walkable areas.
  // Default algorithm is 'Watershed'
  {
    // PARTITION_WATERSHED
    {
      if (!pgRange.BeginNextStep("Build Distance Field"))
        return PL_FAILURE;

      // Prepare for region partitioning, by calculating distance field along the walkable surface.
      if (!rcBuildDistanceField(pContext, *compactHeightfield))
      {
        pContext->log(RC_LOG_ERROR, "Could not build distance field.");
        return PL_FAILURE;
      }

      if (!pgRange.BeginNextStep("Build Regions"))
        return PL_FAILURE;

      // Partition the walkable surface into simple regions without holes.
      if (!rcBuildRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
      {
        pContext->log(RC_LOG_ERROR, "Could not build watershed regions.");
        return PL_FAILURE;
      }
    }

    //// PARTITION_MONOTONE
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  // Monotone partitioning does not need distance field.
    //  if (!rcBuildRegionsMonotone(pContext, *compactHeightfield, 0, cfg.minRegionArea, cfg.mergeRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build monotone regions.");
    //    return PL_FAILURE;
    //  }
    //}

    //// PARTITION_LAYERS
    //{
    //  // Partition the walkable surface into simple regions without holes.
    //  if (!rcBuildLayerRegions(pContext, *compactHeightfield, 0, cfg.minRegionArea))
    //  {
    //    pContext->log(RC_LOG_ERROR, "Could not build layer regions.");
    //    return PL_FAILURE;
    //  }
    //}
  }

  if (!pgRange.BeginNextStep("Build Contours"))
    return PL_FAILURE;

  rcContourSet* contourSet = rcAllocContourSet();
  PL_SCOPE_EXIT(rcFreeContourSet(contourSet));

  if (!rcBuildContours(pContext, *compactHeightfield, cfg.maxSimplificationError, cfg.maxEdgeLen, *contourSet))
  {
    pContext->log(RC_LOG_ERROR, "Could not create contours");
    return PL_FAILURE;
  }

  if (!pgRange.BeginNextStep("Build Poly Mesh"))
    return PL_FAILURE;

  if (!rcBuildPolyMesh(pContext, *contourSet, cfg.maxVertsPerPoly, out_polyMesh))
  {
    pContext->log(RC_LOG_ERROR, "Could not triangulate contours");
    return PL_FAILURE;
  }

  //////////////////////////////////////////////////////////////////////////
  // Detour Navmesh

  if (!pgRange.BeginNextStep("Set Area Flags"))
    return PL_FAILURE;

  // TODO modify area IDs and flags

  for (int i = 0; i < out_polyMesh.npolys; ++i)
  {
    if (out_polyMesh.areas[i] == RC_WALKABLE_AREA)
    {
      out_polyMesh.flags[i] = 0xFFFF;
    }
  }

  return PL_SUCCESS;
}

plResult plRecastNavMeshBuilder::BuildDetourNavMeshData(const plRecastConfig& config, const rcPolyMesh& polyMesh, plDataBuffer& NavmeshData)
{
  dtNavMeshCreateParams params;
  plMemoryUtils::ZeroFill(&params, 1);

  params.verts = polyMesh.verts;
  params.vertCount = polyMesh.nverts;
  params.polys = polyMesh.polys;
  params.polyAreas = polyMesh.areas;
  params.polyFlags = polyMesh.flags;
  params.polyCount = polyMesh.npolys;
  params.nvp = polyMesh.nvp;
  params.walkableHeight = config.m_fAgentHeight;
  params.walkableRadius = config.m_fAgentRadius;
  params.walkableClimb = config.m_fAgentClimbHeight;
  rcVcopy(params.bmin, polyMesh.bmin);
  rcVcopy(params.bmax, polyMesh.bmax);
  params.cs = config.m_fCellSize;
  params.ch = config.m_fCellHeight;
  params.buildBvTree = true;

  plUInt8* navData = nullptr;
  plInt32 navDataSize = 0;

  if (!dtCreateNavMeshData(&params, &navData, &navDataSize))
  {
    plLog::Error("Could not build Detour navmesh.");
    return PL_FAILURE;
  }

  NavmeshData.SetCountUninitialized(navDataSize);
  plMemoryUtils::Copy(NavmeshData.GetData(), navData, navDataSize);

  dtFree(navData);
  return PL_SUCCESS;
}

plResult plRecastConfig::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(1);

  inout_stream << m_fAgentHeight;
  inout_stream << m_fAgentRadius;
  inout_stream << m_fAgentClimbHeight;
  inout_stream << m_WalkableSlope;
  inout_stream << m_fCellSize;
  inout_stream << m_fCellHeight;
  inout_stream << m_fMaxEdgeLength;
  inout_stream << m_fMaxSimplificationError;
  inout_stream << m_fMinRegionSize;
  inout_stream << m_fRegionMergeSize;
  inout_stream << m_fDetailMeshSampleDistanceFactor;
  inout_stream << m_fDetailMeshSampleErrorFactor;

  return PL_SUCCESS;
}

plResult plRecastConfig::Deserialize(plStreamReader& inout_stream)
{
  inout_stream.ReadVersion(1);

  inout_stream >> m_fAgentHeight;
  inout_stream >> m_fAgentRadius;
  inout_stream >> m_fAgentClimbHeight;
  inout_stream >> m_WalkableSlope;
  inout_stream >> m_fCellSize;
  inout_stream >> m_fCellHeight;
  inout_stream >> m_fMaxEdgeLength;
  inout_stream >> m_fMaxSimplificationError;
  inout_stream >> m_fMinRegionSize;
  inout_stream >> m_fRegionMergeSize;
  inout_stream >> m_fDetailMeshSampleDistanceFactor;
  inout_stream >> m_fDetailMeshSampleErrorFactor;

  return PL_SUCCESS;
}
