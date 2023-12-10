#pragma once

#include <RecastPlugin/Components/PointOfInterestGraph.h>

template <typename POINTTYPE>
void plPointOfInterestGraph<POINTTYPE>::Initialize(const plVec3& vCenter, const plVec3& vHalfExtents, float fCellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(vCenter, vHalfExtents, fCellSize);
}

template <typename POINTTYPE>
POINTTYPE& plPointOfInterestGraph<POINTTYPE>::AddPoint(const plVec3& vPosition)
{
  const plUInt32 id = m_Points.GetCount();
  auto& pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(vPosition, plVec3::MakeZero(), 0, id, nullptr, true).IgnoreResult();

  return pt;
}

template <typename POINTTYPE>
void plPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const plVec3& vPosition, float fRadius, plDynamicArray<plUInt32>& out_points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    plDynamicArray<plUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_points;

  auto cb = [](void* pPassThrough, plDynamicTreeObjectConst object) -> bool {
    auto pData = static_cast<Data*>(pPassThrough);

    const plUInt32 id = (plUInt32)object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(vPosition, fRadius, cb, &data);
}
