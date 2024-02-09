#include <AiPlugin/Navigation/NavMesh.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <RendererCore/Debug/DebugRenderer.h>

void DrawMeshTilePolygons(const dtMeshTile& meshTile, plDynamicArray<plDebugRenderer::Triangle>& out_triangles, plArrayPtr<plColor> areaColors);
void DrawMeshTileEdges(const dtMeshTile& meshTile, bool bOuterEdges, bool bInnerEdges, bool bInnerDetailEdges, plDynamicArray<plDebugRenderer::Line>& out_lines);

plAiNavMeshSector::plAiNavMeshSector()
{
  m_FlagRequested = 0;
  m_FlagInvalidate = 0;
  m_FlagUpdateAvailable = 0;
  m_FlagUsable = 0;
}

plAiNavMeshSector::~plAiNavMeshSector() = default;

plAiNavMesh::plAiNavMesh(plUInt32 uiNumSectorsX, plUInt32 uiNumSectorsY, float fSectorMetersXY, const plAiNavmeshConfig& navmeshConfig)
{
  m_uiNumSectorsX = uiNumSectorsX;
  m_uiNumSectorsY = uiNumSectorsY;
  m_fSectorMetersXY = fSectorMetersXY;
  m_fInvSectorMetersXY = 1.0f / fSectorMetersXY;
  m_NavmeshConfig = navmeshConfig;

  m_pNavMesh = PL_DEFAULT_NEW(dtNavMesh);

  dtNavMeshParams np;
  np.tileWidth = fSectorMetersXY;
  np.tileHeight = fSectorMetersXY;
  np.orig[0] = -(uiNumSectorsX * 0.5f) * fSectorMetersXY;
  np.orig[1] = 0.0f;
  np.orig[2] = -(uiNumSectorsY * 0.5f) * fSectorMetersXY;
  np.maxTiles = uiNumSectorsX * uiNumSectorsY;
  np.maxPolys = 1 << 16;

  m_pNavMesh->init(&np);
}

plAiNavMesh::~plAiNavMesh()
{
  PL_DEFAULT_DELETE(m_pNavMesh);
}

plVec2I32 plAiNavMesh::CalculateSectorCoord(float fPositionX, float fPositionY) const
{
  fPositionX *= m_fInvSectorMetersXY;
  fPositionY *= m_fInvSectorMetersXY;

  fPositionX += m_uiNumSectorsX * 0.5f;
  fPositionY += m_uiNumSectorsY * 0.5f;

  return plVec2I32((plInt32)plMath::Floor(fPositionX), (plInt32)plMath::Floor(fPositionY));
}

plVec2I32 plAiNavMesh::CalculateSectorCoord(SectorID sectorID) const
{
  PL_ASSERT_DEBUG(sectorID < m_uiNumSectorsX * m_uiNumSectorsY, "Invalid navmesh sector ID");

  return plVec2I32(sectorID % m_uiNumSectorsX, sectorID / m_uiNumSectorsX);
}

const plAiNavMeshSector* plAiNavMesh::GetSector(SectorID sectorID) const
{
  auto it = m_Sectors.Find(sectorID);
  if (it.IsValid())
    return &it.Value();

  return nullptr;
}

bool plAiNavMesh::RequestSector(SectorID sectorID)
{
  auto& sector = m_Sectors.FindOrAdd(sectorID).Value();

  if (sector.m_FlagUsable == 0)
  {
    if (sector.m_FlagRequested == 0)
    {
      sector.m_FlagRequested = 1;
      m_RequestedSectors.PushBack(sectorID);
    }

    return false;
  }

  return true;
}

bool plAiNavMesh::RequestSector(const plVec2& vCenter, const plVec2& vHalfExtents)
{
  plVec2I32 coordMin = CalculateSectorCoord(vCenter.x - vHalfExtents.x, vCenter.y - vHalfExtents.y);
  plVec2I32 coordMax = CalculateSectorCoord(vCenter.x + vHalfExtents.x, vCenter.y + vHalfExtents.y);

  coordMin.x = plMath::Clamp<plInt32>(coordMin.x, 0, m_uiNumSectorsX - 1);
  coordMax.x = plMath::Clamp<plInt32>(coordMax.x, 0, m_uiNumSectorsX - 1);
  coordMin.y = plMath::Clamp<plInt32>(coordMin.y, 0, m_uiNumSectorsY - 1);
  coordMax.y = plMath::Clamp<plInt32>(coordMax.y, 0, m_uiNumSectorsY - 1);

  bool res = true;

  for (plInt32 y = coordMin.y; y <= coordMax.y; ++y)
  {
    for (plInt32 x = coordMin.x; x <= coordMax.x; ++x)
    {
      if (!RequestSector(CalculateSectorID(plVec2I32(x, y))))
      {
        res = false;
      }
    }
  }

  return res;
}

// void plNavMesh::InvalidateSector(SectorID sectorID)
//{
//   auto it = m_Sectors.Find(sectorID);
//   if (!it.IsValid())
//     return;
//
//   auto& sector = it.Value();
//
//   if (sector.m_FlagInvalidate == 0 && (sector.m_FlagUsable == 1 || sector.m_FlagUpdateAvailable == 1))
//   {
//     sector.m_FlagInvalidate = 1;
//     m_InvalidatedSectors.PushBack(sectorID);
//   }
// }

void plAiNavMesh::FinalizeSectorUpdates()
{
  PL_LOCK(m_Mutex);

  for (auto sectorID : m_UpdatingSectors)
  {
    const auto coord = CalculateSectorCoord(sectorID);

    auto& sector = m_Sectors[sectorID];

    PL_ASSERT_DEV(sector.m_FlagUpdateAvailable == 1, "Invalid sector update state");

    if (!sector.m_NavmeshDataCur.IsEmpty())
    {
      sector.m_FlagUsable = 0;

      const auto res = m_pNavMesh->removeTile(sector.m_TileRef, nullptr, nullptr);

      if (res != DT_SUCCESS)
      {
        plLog::Error("NavMesh removeTile error: {}", res);
      }
    }

    sector.m_NavmeshDataCur.Swap(sector.m_NavmeshDataNew);
    sector.m_NavmeshDataNew.Clear();
    sector.m_NavmeshDataNew.Compact();

    if (!sector.m_NavmeshDataCur.IsEmpty())
    {
      auto res = m_pNavMesh->addTile(sector.m_NavmeshDataCur.GetData(), (int)sector.m_NavmeshDataCur.GetCount(), 0, 0, &sector.m_TileRef);

      if (res == DT_SUCCESS)
      {
        plLog::Success("Loaded navmesh tile {}|{}", coord.x, coord.y);
        sector.m_FlagUsable = 1;
      }
      else
      {
        plLog::Error("NavMesh addTile error: {}", res);
      }
    }
    else
    {
        plLog::Success("Loaded empty navmesh tile {}|{}", coord.x, coord.y);
        sector.m_FlagUsable = 1;
    }

    sector.m_FlagInvalidate = 0;
    sector.m_FlagUpdateAvailable = 0;
    // sector.m_FlagRequested = 0; // do not reset the requested flag
  }

  m_UpdatingSectors.Clear();
}

plAiNavMesh::SectorID plAiNavMesh::RetrieveRequestedSector()
{
  if (m_RequestedSectors.IsEmpty())
    return plInvalidIndex;

  plAiNavMesh::SectorID id = m_RequestedSectors.PeekFront();
  m_RequestedSectors.PopFront();

  return id;
}

plVec2 plAiNavMesh::GetSectorPositionOffset(plVec2I32 vCoord) const
{
  return plVec2((vCoord.x - m_uiNumSectorsX * 0.5f) * m_fSectorMetersXY, (vCoord.y - m_uiNumSectorsY * 0.5f) * m_fSectorMetersXY);
}

plBoundingBox plAiNavMesh::GetSectorBounds(plVec2I32 vCoord, float fMinZ /*= 0.0f*/, float fMaxZ /*= 1.0f*/) const
{
  const plVec3 min = GetSectorPositionOffset(vCoord).GetAsVec3(fMinZ);
  const plVec3 max = min + plVec3(m_fSectorMetersXY, m_fSectorMetersXY, fMaxZ - fMinZ);

  return plBoundingBox(min, max);
}

void plAiNavMesh::DebugDraw(plDebugRendererContext context, const plAiNavigationConfig& config)
{
  const auto& dtm = *GetDetourNavMesh();

  for (int i = 0; i < dtm.getMaxTiles(); ++i)
  {
    DebugDrawSector(context, config, i);
  }
}

void plAiNavMesh::DebugDrawSector(plDebugRendererContext context, const plAiNavigationConfig& config, int iTileIdx)
{
  const auto& mesh = *GetDetourNavMesh();

  const dtMeshTile* pTile = mesh.getTile(iTileIdx);

  if (pTile == nullptr || pTile->header == nullptr)
    return;

  plColor areaColors[plAiNumGroundTypes];
  for (plUInt32 i = 0; i < plAiNumGroundTypes; ++i)
  {
    areaColors[i] = config.m_GroundTypes[i].m_Color;
  }

  {
    plDynamicArray<plDebugRenderer::Triangle> triangles;
    triangles.Reserve(pTile->header->polyCount * 2);

    DrawMeshTilePolygons(*pTile, triangles, areaColors);
    plDebugRenderer::DrawSolidTriangles(context, triangles, plColor::White);
  }

  {
    plDynamicArray<plDebugRenderer::Line> lines;
    lines.Reserve(pTile->header->polyCount * 10);
    DrawMeshTileEdges(*pTile, true, true, false, lines);
    plDebugRenderer::DrawLines(context, lines, plColor::White);
  }
}
