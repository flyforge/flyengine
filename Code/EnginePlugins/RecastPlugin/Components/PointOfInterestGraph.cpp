#include <RecastPlugin/RecastPluginPCH.h>

#include <RecastPlugin/Components/PointOfInterestGraph.h>

struct plDummyPointType
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 value;
};

void CompileDummy()
{
  plPointOfInterestGraph<plDummyPointType> graph;
  graph.Initialize(plVec3::MakeZero(), plVec3::MakeZero());
  auto& pt = graph.AddPoint(plVec3::MakeZero());

  plDynamicArray<plUInt32> points;
  graph.FindPointsOfInterest(plVec3::MakeZero(), 0, points);
}


