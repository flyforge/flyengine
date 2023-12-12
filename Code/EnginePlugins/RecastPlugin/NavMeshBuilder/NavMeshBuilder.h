#pragma once

#include <Foundation/Reflection/Reflection.h>
#include <Foundation/Types/UniquePtr.h>
#include <RecastPlugin/RecastPluginDLL.h>
#include <RendererCore/Utils/WorldGeoExtractionUtil.h>

class plRcBuildContext;
struct rcPolyMesh;
struct rcPolyMeshDetail;
class plWorld;
class dtNavMesh;
struct plRecastNavMeshResourceDescriptor;
class plProgress;
class plStreamWriter;
class plStreamReader;

struct PLASMA_RECASTPLUGIN_DLL plRecastConfig
{
  float m_fAgentHeight = 1.5f;
  float m_fAgentRadius = 0.3f;
  float m_fAgentClimbHeight = 0.4f;
  plAngle m_WalkableSlope = plAngle::Degree(45);
  float m_fCellSize = 0.2f;
  float m_fCellHeight = 0.2f;
  float m_fMaxEdgeLength = 4.0f;
  float m_fMaxSimplificationError = 1.3f;
  float m_fMinRegionSize = 3.0f;
  float m_fRegionMergeSize = 20.0f;
  float m_fDetailMeshSampleDistanceFactor = 1.0f;
  float m_fDetailMeshSampleErrorFactor = 1.0f;

  plResult Serialize(plStreamWriter& stream) const;
  plResult Deserialize(plStreamReader& stream);
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_RECASTPLUGIN_DLL, plRecastConfig);



class PLASMA_RECASTPLUGIN_DLL plRecastNavMeshBuilder
{
public:
  plRecastNavMeshBuilder();
  ~plRecastNavMeshBuilder();

  static plResult ExtractWorldGeometry(const plWorld& world, plWorldGeoExtractionUtil::MeshObjectList& out_worldGeo);

  plResult Build(const plRecastConfig& config, const plWorldGeoExtractionUtil::MeshObjectList& worldGeo, plRecastNavMeshResourceDescriptor& out_NavMeshDesc,
    plProgress& progress);

private:
  static void FillOutConfig(struct rcConfig& cfg, const plRecastConfig& config, const plBoundingBox& bbox);

  void Clear();
  void GenerateTriangleMeshFromDescription(const plWorldGeoExtractionUtil::MeshObjectList& objects);
  void ComputeBoundingBox();
  plResult BuildRecastPolyMesh(const plRecastConfig& config, rcPolyMesh& out_PolyMesh, plProgress& progress);
  static plResult BuildDetourNavMeshData(const plRecastConfig& config, const rcPolyMesh& polyMesh, plDataBuffer& NavmeshData);

  struct Triangle
  {
    Triangle() {}
    Triangle(plInt32 a, plInt32 b, plInt32 c)
    {
      m_VertexIdx[0] = a;
      m_VertexIdx[1] = b;
      m_VertexIdx[2] = c;
    }

    plInt32 m_VertexIdx[3];
  };

  plBoundingBox m_BoundingBox;
  plDynamicArray<plVec3> m_Vertices;
  plDynamicArray<Triangle> m_Triangles;
  plDynamicArray<plUInt8> m_TriangleAreaIDs;
  plRcBuildContext* m_pRecastContext = nullptr;
};
