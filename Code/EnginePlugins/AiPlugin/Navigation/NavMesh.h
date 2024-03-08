#pragma once

#include <AiPlugin/Navigation/NavigationConfig.h>
#include <DetourNavMesh.h>
#include <DetourNavMeshQuery.h>
#include <Foundation/Containers/Deque.h>
#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Math/Vec2.h>
#include <Foundation/Types/RefCounted.h>
#include <Foundation/Types/SharedPtr.h>
#include <Recast.h>
#include <RendererCore/Debug/DebugRendererContext.h>

using plDataBuffer = plDynamicArray<plUInt8>;

class plPhysicsWorldModuleInterface;
class dtNavMesh;

/// \brief Stores indices for a triangle.
struct plAiNavMeshTriangle final
{
  PL_DECLARE_POD_TYPE();

  plAiNavMeshTriangle() = default;
  plAiNavMeshTriangle(plInt32 a, plInt32 b, plInt32 c)
  {
    m_VertexIdx[0] = a;
    m_VertexIdx[1] = b;
    m_VertexIdx[2] = c;
  }

  plInt32 m_VertexIdx[3];
};

/// \brief Stores the geometry from which a navmesh should be generated.
struct plAiNavMeshInputGeo final
{
  plDynamicArray<plVec3> m_Vertices;
  plDynamicArray<plAiNavMeshTriangle> m_Triangles;
  plDynamicArray<plUInt8> m_TriangleAreaIDs;
};

/// \brief State about a single sector (tile / cell) of an plAiNavMesh
struct plAiNavMeshSector final
{
  plAiNavMeshSector();
  ~plAiNavMeshSector();

  plUInt8 m_FlagRequested : 1;
  plUInt8 m_FlagInvalidate : 1;
  plUInt8 m_FlagUpdateAvailable : 1;
  plUInt8 m_FlagUsable : 1;

  plDataBuffer m_NavmeshDataCur;
  plDataBuffer m_NavmeshDataNew;
  dtTileRef m_TileRef = 0;
};

/// \brief A navmesh generated with a specific configuration.
///
/// Each game may use multiple navmeshes for different character types (large, small, etc).
/// All navmeshes always exist, but only some may contain data.
/// You get access to a navmesh through the plAiNavMeshWorldModule.
///
/// To do a path search, use plAiNavigation.
/// Since the navmesh is built in the background, a path search may need to run for multiple frames,
/// before it can return any result.
class PL_AIPLUGIN_DLL plAiNavMesh final
{
  PL_DISALLOW_COPY_AND_ASSIGN(plAiNavMesh);

public:
  plAiNavMesh(plUInt32 uiNumSectorsX, plUInt32 uiNumSectorsY, float fSectorMetersXY, const plAiNavmeshConfig& navmeshConfig);
  ~plAiNavMesh();

  using SectorID = plUInt32;

  plVec2I32 CalculateSectorCoord(float fPositionX, float fPositionY) const;
  plVec2 GetSectorPositionOffset(plVec2I32 vCoord) const;
  plBoundingBox GetSectorBounds(plVec2I32 vCoord, float fMinZ = 0.0f, float fMaxZ = 1.0f) const;
  float GetSectorSize() const { return m_fSectorMetersXY; }
  SectorID CalculateSectorID(plVec2I32 vCoord) const { return vCoord.y * m_uiNumSectorsX + vCoord.x; }
  plVec2I32 CalculateSectorCoord(SectorID sectorID) const;

  const plAiNavMeshSector* GetSector(SectorID sectorID) const;

  /// \brief Marks the sector as requested.
  ///
  /// Returns true, if the sector is already available, false when it needs to be built first.
  bool RequestSector(SectorID sectorID);

  /// \brief Marks all sectors within the given rectangle as requested.
  ///
  /// Returns true, if all the sectors are already available, false when any of them needs to be built first.
  bool RequestSector(const plVec2& vCenter, const plVec2& vHalfExtents);

  /// \brief Marks the sector as invalidated.
  ///
  /// Invalidated sectors are considered out of date and must be rebuilt before they can be used again.
  /// If bRebuildAsSoonAsPossible is true, the sector is queued to be rebuilt as soon as possible.
  /// Otherwise, it will be unloaded and will not be rebuilt until it is requested again.
  void InvalidateSector(SectorID sectorID, bool bRebuildAsSoonAsPossible);

  /// \brief Marks all sectors within the given rectangle as invalidated.
  ///
  /// Invalidated sectors are considered out of date and must be rebuilt before they can be used again.
  /// If bRebuildAsSoonAsPossible is true, the sector is queued to be rebuilt as soon as possible.
  /// Otherwise, it will be unloaded and will not be rebuilt until it is requested again.
  void InvalidateSector(const plVec2& vCenter, const plVec2& vHalfExtents, bool bRebuildAsSoonAsPossible);

  void FinalizeSectorUpdates();

  SectorID RetrieveRequestedSector();
  void BuildSector(SectorID sectorID, const plPhysicsWorldModuleInterface* pPhysics);

  const dtNavMesh* GetDetourNavMesh() const { return m_pNavMesh; }

  void DebugDraw(plDebugRendererContext context, const plAiNavigationConfig& config);

  const plAiNavmeshConfig& GetConfig() const { return m_NavmeshConfig; }
private:
  void DebugDrawSector(plDebugRendererContext context, const plAiNavigationConfig& config, int iTileIdx);

  plAiNavmeshConfig m_NavmeshConfig;

  plUInt32 m_uiNumSectorsX = 0;
  plUInt32 m_uiNumSectorsY = 0;
  float m_fSectorMetersXY = 0;
  float m_fInvSectorMetersXY = 0;

  dtNavMesh* m_pNavMesh = nullptr;
  plMap<SectorID, plAiNavMeshSector> m_Sectors;
  plDeque<SectorID> m_RequestedSectors;

  plMutex m_Mutex;
  plDynamicArray<SectorID> m_UpdatingSectors;

  plDynamicArray<SectorID> m_UnloadingSectors;
};
