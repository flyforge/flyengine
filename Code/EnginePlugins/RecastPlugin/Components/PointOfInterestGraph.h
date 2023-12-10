#pragma once

#include <Foundation/Math/Vec3.h>
#include <Utilities/DataStructures/DynamicOctree.h>

template <typename POINTTYPE>
class plPointOfInterestGraph
{
public:
  void Initialize(const plVec3& vCenter, const plVec3& vHalfExtents, float fCellSize = 1.0f);

  POINTTYPE& AddPoint(const plVec3& vPosition);

  void FindPointsOfInterest(const plVec3& vPosition, float fRadius, plDynamicArray<plUInt32>& out_points) const;

  const plDeque<POINTTYPE>& GetPoints() const { return m_Points; }
  plDeque<POINTTYPE>& AccessPoints() { return m_Points; }

private:
  plDeque<POINTTYPE> m_Points;
  plDynamicOctree m_Octree;
};

#include <RecastPlugin/Components/PointOfInterestGraph_inl.h>
