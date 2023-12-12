#pragma once

#include <Foundation/Math/Vec3.h>
#include <GameEngine/GameEngineDLL.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template <typename POINTTYPE>
class plPointOfInterestGraph
{
public:
  void Initialize(const plVec3& center, const plVec3& halfExtents, float cellSize = 1.0f);

  POINTTYPE& AddPoint(const plVec3& position);

  void FindPointsOfInterest(const plVec3& position, float radius, plDynamicArray<plUInt32>& out_Points) const;

  const plDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  plDeque<POINTTYPE>& AccessPoints() { return m_Points; }

private:
  plDeque<POINTTYPE> m_Points;
  plDynamicOctree m_Octree;
};

#include <GameEngine/AI/Implementation/PointOfInterestGraph_inl.h>
