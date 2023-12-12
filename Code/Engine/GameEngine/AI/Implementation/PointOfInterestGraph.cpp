#include <GameEngine/GameEnginePCH.h>

#include <GameEngine/AI/PointOfInterestGraph.h>

struct plDummyPointType
{
  PLASMA_DECLARE_POD_TYPE();

  plUInt32 value;
};

void CompileDummy()
{
  plPointOfInterestGraph<plDummyPointType> graph;
  graph.Initialize(plVec3::ZeroVector(), plVec3::ZeroVector());
  auto& pt = graph.AddPoint(plVec3::ZeroVector());

  plDynamicArray<plUInt32> points;
  graph.FindPointsOfInterest(plVec3::ZeroVector(), 0, points);
}


PLASMA_STATICLINK_FILE(GameEngine, GameEngine_AI_Implementation_PointOfInterestGraph);
