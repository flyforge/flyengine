#pragma once

#include <Foundation/Time/Time.h>
#include <GameEngine/AI/PointOfInterestGraph.h>
#include <RecastPlugin/RecastPluginDLL.h>

struct rcPolyMesh;

struct PLASMA_RECASTPLUGIN_DLL plNavMeshPointsOfInterest
{
  plVec3 m_vFloorPosition;
  plUInt32 m_uiVisibleMarker = 0;
};

class PLASMA_RECASTPLUGIN_DLL plNavMeshPointOfInterestGraph
{
public:
  plNavMeshPointOfInterestGraph();
  ~plNavMeshPointOfInterestGraph();

  void ExtractInterestPointsFromMesh(const rcPolyMesh& mesh, bool bReinitialize = true /* bad interface design */);

  plUInt32 GetCheckVisibilityTimeStamp() const { return m_uiCheckVisibilityTimeStamp; }
  void IncreaseCheckVisibiblityTimeStamp(plTime tNow);

  plPointOfInterestGraph<plNavMeshPointsOfInterest>& GetGraph() { return m_NavMeshPointGraph; }
  const plPointOfInterestGraph<plNavMeshPointsOfInterest>& GetGraph() const { return m_NavMeshPointGraph; }

protected:
  plTime m_LastTimeStampStep;
  plUInt32 m_uiCheckVisibilityTimeStamp = 100;
  plPointOfInterestGraph<plNavMeshPointsOfInterest> m_NavMeshPointGraph;
};
