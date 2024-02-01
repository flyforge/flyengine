#include <RecastPlugin/RecastPluginPCH.h>

#include <Foundation/Containers/StaticArray.h>
#include <Recast.h>
#include <RecastPlugin/NavMeshBuilder/NavMeshPointsOfInterest.h>

plNavMeshPointOfInterestGraph::plNavMeshPointOfInterestGraph() = default;
plNavMeshPointOfInterestGraph::~plNavMeshPointOfInterestGraph() = default;

void plNavMeshPointOfInterestGraph::IncreaseCheckVisibiblityTimeStamp(plTime now)
{
  if (now - m_LastTimeStampStep < plTime::MakeFromSeconds(0.5f))
    return;

  m_LastTimeStampStep = now;
  m_uiCheckVisibilityTimeStamp += 4;
}

PL_ALWAYS_INLINE static plVec3 GetNavMeshVertex(const rcPolyMesh* pMesh, plUInt16 uiVertex, const plVec3& vMeshOrigin, float fCellSize, float fCellHeight)
{
  const plUInt16* v = &pMesh->verts[uiVertex * 3];
  const float x = vMeshOrigin.x + v[0] * fCellSize;
  const float y = vMeshOrigin.y + v[2] * fCellSize;
  const float z = vMeshOrigin.z + v[1] * fCellHeight;

  return plVec3(x, y, z);
}

const float toCenterOffset = 0.1f;
const float alongLineOffset = 0.5f;

struct PotentialPoI
{
  bool m_bUsed;
  plVec3 m_vVertexPos;
  plVec3 m_vPosition;
  plVec3 m_vLineDir;
};

PL_ALWAYS_INLINE static void AddToInterestPoints(plDeque<PotentialPoI>& ref_interestPoints, plInt32 iVertexIdx, const plVec3& vPos, const plVec3& vPolyCenter, plVec3 vLineDir)
{
  plVec3 toCenter = vPolyCenter - vPos;
  toCenter.SetLength(toCenterOffset).IgnoreResult();

  const plVec3 posWithOffset = vPos + toCenter + vLineDir * alongLineOffset;

  if (iVertexIdx < 0)
  {
    auto& poi = ref_interestPoints.ExpandAndGetRef();
    poi.m_bUsed = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir = vLineDir;
  }
  else if (!ref_interestPoints[iVertexIdx].m_bUsed)
  {
    auto& poi = ref_interestPoints[iVertexIdx];
    poi.m_bUsed = true;
    poi.m_vPosition = posWithOffset;
    poi.m_vLineDir = vLineDir;
  }
  else
  {
    auto& poi = ref_interestPoints[iVertexIdx];

    plPlane plane;
    plane.SetFromPoints(poi.m_vVertexPos, poi.m_vVertexPos + poi.m_vLineDir, poi.m_vVertexPos + plVec3(0, 0, 1.0f)).IgnoreResult();

    plPositionOnPlane::Enum side1 = plane.GetPointPosition(poi.m_vPosition);
    plPositionOnPlane::Enum side2 = plane.GetPointPosition(posWithOffset);

    if (side1 == side2)
    {
      // same side, collapse points to average position

      poi.m_vPosition = plMath::Lerp(poi.m_vPosition, posWithOffset, 0.5f);
    }
    else
    {
      // different sides, keep both points

      auto& poi2 = ref_interestPoints.ExpandAndGetRef();
      poi2.m_bUsed = true;
      poi2.m_vPosition = posWithOffset;
      poi2.m_vLineDir = vLineDir;
    }
  }
}

void plNavMeshPointOfInterestGraph::ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize)
{
  PL_LOG_BLOCK("Extract NavMesh Points of Interest");

  const plInt32 iMaxNumVertInPoly = mesh.nvp;
  const float fCellSize = mesh.cs;
  const float fCellHeight = mesh.ch;

  const plVec3 vMeshOrigin(mesh.bmin[0], mesh.bmin[2], mesh.bmin[1]);

  plDeque<PotentialPoI> interestPoints;
  interestPoints.SetCount(mesh.nverts);

  for (plInt32 i = 0; i < mesh.nverts; ++i)
  {
    interestPoints[i].m_bUsed = false;
    interestPoints[i].m_vVertexPos = GetNavMeshVertex(&mesh, static_cast<plUInt16>(i), vMeshOrigin, fCellSize, fCellHeight);
  }

  plStaticArray<plVec3, 16> polyVertices;
  plStaticArray<plUInt16, 16> polyVertexIndices;
  plStaticArray<bool, 16> isContourEdge;

  for (plInt32 i = 0; i < mesh.npolys; ++i)
  {
    const plUInt32 uiBaseIndex = i * (iMaxNumVertInPoly * 2);
    const plUInt16* polyVtxIndices = &mesh.polys[uiBaseIndex];
    const plUInt16* neighborData = &mesh.polys[uiBaseIndex + iMaxNumVertInPoly];

    bool hasAnyContour = false;
    polyVertices.Clear();
    polyVertexIndices.Clear();
    isContourEdge.Clear();

    plVec3 vPolyCenter;
    vPolyCenter.SetZero();

    for (plInt32 j = 0; j < iMaxNumVertInPoly; ++j)
    {
      if (polyVtxIndices[j] == RC_MESH_NULL_IDX)
        break;

      const bool isDisconnected = neighborData[j] == 0xffff;
      hasAnyContour |= isDisconnected;

      const plVec3 pos = GetNavMeshVertex(&mesh, polyVtxIndices[j], vMeshOrigin, fCellSize, fCellHeight);
      vPolyCenter += pos;

      polyVertices.PushBack(pos);
      polyVertexIndices.PushBack(polyVtxIndices[j]);
      isContourEdge.PushBack(isDisconnected);
    }

    if (!hasAnyContour)
      continue;

    vPolyCenter /= (float)polyVertices.GetCount();

    // filter out too short edges
    {
      plUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (plUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const plVec3 start = polyVertices[uiCurEdgeIdx];
          const plVec3 end = polyVertices[uiNextEdgeIdx];
          const plVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < plMath::Square(0.5f))
          {
            isContourEdge[uiCurEdgeIdx] = false;
          }
        }

        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }

    // filter out medium edges with neighbors
    {
      plUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      plUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (plUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const plVec3 start = polyVertices[uiCurEdgeIdx];
          const plVec3 end = polyVertices[uiNextEdgeIdx];
          const plVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < plMath::Square(2.0f))
          {
            // if we have neighbor edges, let them try again
            if (isContourEdge[uiPrevEdgeIdx] || isContourEdge[uiNextEdgeIdx])
            {
              // nothing inserted, so treat this as connected edge
              isContourEdge[uiCurEdgeIdx] = false;
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }

    // now insert points of interests along contour edges
    {
      plUInt32 uiPrevEdgeIdx = isContourEdge.GetCount() - 2;
      plUInt32 uiCurEdgeIdx = isContourEdge.GetCount() - 1;

      for (plUInt32 uiNextEdgeIdx = 0; uiNextEdgeIdx < isContourEdge.GetCount(); ++uiNextEdgeIdx)
      {
        if (isContourEdge[uiCurEdgeIdx])
        {
          const plInt32 startIdx = polyVertexIndices[uiCurEdgeIdx];
          const plInt32 endIdx = polyVertexIndices[uiNextEdgeIdx];

          const plVec3 start = polyVertices[uiCurEdgeIdx];
          const plVec3 end = polyVertices[uiNextEdgeIdx];
          const plVec3 startToEnd = end - start;
          const float distSqr = startToEnd.GetLengthSquared();

          if (distSqr < plMath::Square(2.0f))
          {
            AddToInterestPoints(interestPoints, -1, plMath::Lerp(start, end, 0.5f), vPolyCenter, plVec3::MakeZero());
          }
          else
          {
            // to prevent inserting the same point multiple times, only the edge with the larger index is allowed to insert
            // points at connected edges
            // due to the index wrap around, there is no guarantee that uiNextEdgeIdx is always larger than uiCurEdgeIdx etc.

            if (isContourEdge[uiPrevEdgeIdx])
            {
              if (uiCurEdgeIdx > uiPrevEdgeIdx)
              {
                AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, plVec3::MakeZero());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, startIdx, start, vPolyCenter, startToEnd.GetNormalized());
            }

            if (isContourEdge[uiNextEdgeIdx])
            {
              if (uiCurEdgeIdx > uiNextEdgeIdx)
              {
                AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, plVec3::MakeZero());
              }
            }
            else
            {
              AddToInterestPoints(interestPoints, endIdx, end, vPolyCenter, -startToEnd.GetNormalized());
            }
          }
        }

        uiPrevEdgeIdx = uiCurEdgeIdx;
        uiCurEdgeIdx = uiNextEdgeIdx;
      }
    }
  }

  if (bReinitialize)
  {
    plBoundingBox box = plBoundingBox::MakeInvalid();

    // compute bounding box
    {
      for (auto potPoi : interestPoints)
      {
        if (potPoi.m_bUsed)
        {
          box.ExpandToInclude(potPoi.m_vPosition);
        }
      }

      box.Grow(plVec3(1.0f));
    }

    m_NavMeshPointGraph.Initialize(box.GetCenter(), box.GetHalfExtents());
  }



  // add all points
  {
    plUInt32 uiNumPoints = 0;

    for (auto potPoi : interestPoints)
    {
      if (potPoi.m_bUsed)
      {
        ++uiNumPoints;

        auto& poi = m_NavMeshPointGraph.AddPoint(potPoi.m_vPosition);
        poi.m_vFloorPosition = potPoi.m_vPosition;
      }
    }

    plLog::Dev("Num Points of Interest: {0}", uiNumPoints);
  }
}
