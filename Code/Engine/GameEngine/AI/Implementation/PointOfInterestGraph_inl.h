#pragma once

#include <GameEngine/AI/PointOfInterestGraph.h>

template <typename POINTTYPE>
void plPointOfInterestGraph<POINTTYPE>::Initialize(const plVec3& center, const plVec3& halfExtents, float cellSize)
{
  m_Points.Clear();
  m_Octree.CreateTree(center, halfExtents, cellSize);
}

template <typename POINTTYPE>
POINTTYPE& plPointOfInterestGraph<POINTTYPE>::AddPoint(const plVec3& position)
{
  const plUInt32 id = m_Points.GetCount();
  auto& pt = m_Points.ExpandAndGetRef();

  m_Octree.InsertObject(position, plVec3::ZeroVector(), 0, id, nullptr, true).IgnoreResult();

  return pt;
}

template <typename POINTTYPE>
void plPointOfInterestGraph<POINTTYPE>::FindPointsOfInterest(const plVec3& position, float radius, plDynamicArray<plUInt32>& out_Points) const
{
  if (m_Octree.IsEmpty())
    return;

  struct Data
  {
    plDynamicArray<plUInt32>* m_pResults;
  };

  Data data;
  data.m_pResults = &out_Points;

  auto cb = [](void* pPassThrough, plDynamicTreeObjectConst Object) -> bool {
    auto pData = static_cast<Data*>(pPassThrough);

    const plUInt32 id = (plUInt32)Object.Value().m_iObjectInstance;
    pData->m_pResults->PushBack(id);

    return true;
  };

  m_Octree.FindObjectsInRange(position, radius, cb, &data);
}
