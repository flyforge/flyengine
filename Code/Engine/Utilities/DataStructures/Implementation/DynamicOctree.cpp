#include <Utilities/UtilitiesPCH.h>

#include <Utilities/DataStructures/DynamicOctree.h>

const float plDynamicOctree::s_LooseOctreeFactor = 1.1f;

plDynamicOctree::plDynamicOctree()
  : m_uiMaxTreeDepth(0)
  , m_uiAddIDTopLevel(0)
{
}

void plDynamicOctree::CreateTree(const plVec3& vCenter, const plVec3& vHalfExtents, float fMinNodeSize)
{
  m_uiMultiMapCounter = 1;

  m_NodeMap.Clear();

  // the real bounding box might be long and thing -> bad node-size
  // but still it can be used to reject inserting objects that are entirely outside the world
  m_fRealMinX = vCenter.x - vHalfExtents.x;
  m_fRealMaxX = vCenter.x + vHalfExtents.x;
  m_fRealMinY = vCenter.y - vHalfExtents.y;
  m_fRealMaxY = vCenter.y + vHalfExtents.y;
  m_fRealMinZ = vCenter.z - vHalfExtents.z;
  m_fRealMaxZ = vCenter.z + vHalfExtents.z;

  // the bounding box should be square, so use the maximum of the x, y and z extents
  float fMax = plMath::Max(vHalfExtents.x, plMath::Max(vHalfExtents.y, vHalfExtents.z));

  m_BBox.SetCenterAndHalfExtents(vCenter, plVec3(fMax));

  float fLength = fMax * 2.0f;

  m_uiMaxTreeDepth = 0;
  while (fLength > fMinNodeSize)
  {
    ++m_uiMaxTreeDepth;
    fLength = (fLength / 2.0f) * s_LooseOctreeFactor;
  }

  m_uiAddIDTopLevel = 0;
  for (plUInt32 i = 0; i < m_uiMaxTreeDepth; ++i)
    m_uiAddIDTopLevel += plMath::Pow(8, i);
}

/// The object lies at vCenter and has vHalfExtents as its bounding box.
/// If bOnlyIfInside is false, the object is ALWAYS inserted, even if it is outside the tree.
/// \note In such a case it is inserted at the root-node and thus ALWAYS returned in range/view-frustum queries.
///
/// If bOnlyIfInside is true, the object is discarded, if it is not inside the actual bounding box of the tree.
plResult plDynamicOctree::InsertObject(const plVec3& vCenter, const plVec3& vHalfExtents, plInt32 iObjectType, plInt32 iObjectInstance,
  plDynamicTreeObject* out_Object, bool bOnlyIfInside)
{
  if (out_Object)
    *out_Object = plDynamicTreeObject();

  if (bOnlyIfInside)
  {
    if (vCenter.x + vHalfExtents.x < m_fRealMinX)
      return PLASMA_FAILURE;

    if (vCenter.x - vHalfExtents.x > m_fRealMaxX)
      return PLASMA_FAILURE;

    if (vCenter.y + vHalfExtents.y < m_fRealMinY)
      return PLASMA_FAILURE;

    if (vCenter.y - vHalfExtents.y > m_fRealMaxY)
      return PLASMA_FAILURE;

    if (vCenter.z + vHalfExtents.z < m_fRealMinZ)
      return PLASMA_FAILURE;

    if (vCenter.z - vHalfExtents.z > m_fRealMaxZ)
      return PLASMA_FAILURE;
  }

  plDynamicTree::plObjectData oData;
  oData.m_iObjectType = iObjectType;
  oData.m_iObjectInstance = iObjectInstance;

  // insert the object into the best child
  if (!InsertObject(vCenter, vHalfExtents, oData, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
        m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, plMath::Pow(8, m_uiMaxTreeDepth - 1), out_Object))
  {
    if (!bOnlyIfInside)
    {
      plDynamicTree::plMultiMapKey mmk;
      mmk.m_uiKey = 0;
      mmk.m_uiCounter = m_uiMultiMapCounter++;

      auto key = m_NodeMap.Insert(mmk, oData);

      if (out_Object)
        *out_Object = key;

      return PLASMA_SUCCESS;
    }

    return PLASMA_FAILURE;
  }

  return PLASMA_SUCCESS;
}

bool plDynamicOctree::InsertObject(const plVec3& vCenter, const plVec3& vHalfExtents, const plDynamicTree::plObjectData& Obj, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, plUInt32 uiNodeID, plUInt32 uiAddID, plUInt32 uiSubAddID, plDynamicTreeObject* out_Object)
{
  if (vCenter.x - vHalfExtents.x < minx)
    return false;
  if (vCenter.x + vHalfExtents.x > maxx)
    return false;
  if (vCenter.y - vHalfExtents.y < miny)
    return false;
  if (vCenter.y + vHalfExtents.y > maxy)
    return false;
  if (vCenter.z - vHalfExtents.z < minz)
    return false;
  if (vCenter.z + vHalfExtents.z > maxz)
    return false;

  if (uiAddID > 0)
  {
    const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
    const float ly = ((maxy - miny) * 0.5f) * s_LooseOctreeFactor;
    const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

    const plUInt32 uiNodeIDBase = uiNodeID + 1;
    const plUInt32 uiAddIDChild = uiAddID - uiSubAddID;
    const plUInt32 uiSubAddIDChild = uiSubAddID >> 3;

    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
    if (InsertObject(vCenter, vHalfExtents, Obj, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7, uiAddIDChild,
          uiSubAddIDChild, out_Object))
      return true;
  }

  plDynamicTree::plMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;
  mmk.m_uiCounter = m_uiMultiMapCounter++;

  auto key = m_NodeMap.Insert(mmk, Obj);

  if (out_Object)
    *out_Object = key;

  return true;
}

void plDynamicOctree::FindObjectsInRange(const plVec3& vPoint, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  if (m_NodeMap.IsEmpty())
    return;

  if (!m_BBox.Contains(vPoint))
    return;

  FindObjectsInRange(vPoint, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, plMath::Pow(8, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

void plDynamicOctree::FindVisibleObjects(const plFrustum& Viewfrustum, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  PLASMA_ASSERT_DEV(m_uiMaxTreeDepth > 0, "plDynamicOctree::FindVisibleObjects: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindVisibleObjects(Viewfrustum, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, plMath::Pow(4, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

void plDynamicOctree::FindVisibleObjects(const plFrustum& Viewfrustum, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, plUInt32 uiNodeID, plUInt32 uiAddID, plUInt32 uiSubAddID, plUInt32 uiNextNodeID) const
{
  plVec3 v[8];
  v[0].Set(minx, miny, minz);
  v[1].Set(minx, miny, maxz);
  v[2].Set(minx, maxy, minz);
  v[3].Set(minx, maxy, maxz);
  v[4].Set(maxx, miny, minz);
  v[5].Set(maxx, miny, maxz);
  v[6].Set(maxx, maxy, minz);
  v[7].Set(maxx, maxy, maxz);

  plVolumePosition::Enum pos = Viewfrustum.GetObjectPosition(&v[0], 8);

  if (pos == plVolumePosition::Outside)
    return;

  plDynamicTree::plMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  plDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return;

  if (pos == plVolumePosition::Inside)
  {
    mmk.m_uiKey = uiNextNodeID;

    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      plDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    return;
  }
  else if (pos == plVolumePosition::Intersecting)
  {
    mmk.m_uiKey = uiNodeID + 1;

    while (it1.IsValid())
    {
      // first increase the iterator, the user could erase it in the callback
      plDynamicTreeObjectConst temp = it1;
      ++it1;

      Callback(pPassThrough, temp);
    }

    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const plUInt32 uiNodeIDBase = uiNodeID + 1;
      const plUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const plUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
        uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7);
      FindVisibleObjects(Viewfrustum, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
        uiAddIDChild, uiSubAddIDChild, uiNextNodeID);
    }
  }
}

void plDynamicOctree::RemoveObject(plDynamicTreeObject obj)
{
  m_NodeMap.Remove(obj);
}

void plDynamicOctree::RemoveObject(plInt32 iObjectType, plInt32 iObjectInstance)
{
  for (plDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid(); ++it)
  {
    if ((it.Value().m_iObjectInstance == iObjectInstance) && (it.Value().m_iObjectType == iObjectType))
    {
      m_NodeMap.Remove(it);
      return;
    }
  }
}

void plDynamicOctree::RemoveObjectsOfType(plInt32 iObjectType)
{
  for (plDynamicTreeObject it = m_NodeMap.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_iObjectType == iObjectType)
    {
      plDynamicTreeObject itold = it;
      ++it;

      m_NodeMap.Remove(itold);
    }
    else
      ++it;
  }
}



bool plDynamicOctree::FindObjectsInRange(const plVec3& vPoint, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx, float maxx,
  float miny, float maxy, float minz, float maxz, plUInt32 uiNodeID, plUInt32 uiAddID, plUInt32 uiSubAddID, plUInt32 uiNextNodeID) const
{
  if (vPoint.x < minx)
    return true;
  if (vPoint.x > maxx)
    return true;
  if (vPoint.y < miny)
    return true;
  if (vPoint.y > maxy)
    return true;
  if (vPoint.z < minz)
    return true;
  if (vPoint.z > maxz)
    return true;

  plDynamicTree::plMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  plDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {
    {
      plDynamicTree::plMultiMapKey mmk2;
      mmk2.m_uiKey = uiNodeID + 1;

      const plDynamicTreeObjectConst itlast = m_NodeMap.LowerBound(mmk2);

      while (it1 != itlast)
      {
        // first increase the iterator, the user could erase it in the callback
        plDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const plUInt32 uiNodeIDBase = uiNodeID + 1;
      const plUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const plUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7))
        return false;
      if (!FindObjectsInRange(vPoint, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
            uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}

void plDynamicOctree::FindObjectsInRange(const plVec3& vPoint, float fRadius, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough) const
{
  PLASMA_ASSERT_DEV(m_uiMaxTreeDepth > 0, "plDynamicOctree::FindObjectsInRange: You have to first create the tree.");

  if (m_NodeMap.IsEmpty())
    return;

  FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, m_BBox.m_vMin.x, m_BBox.m_vMax.x, m_BBox.m_vMin.y, m_BBox.m_vMax.y, m_BBox.m_vMin.z,
    m_BBox.m_vMax.z, 0, m_uiAddIDTopLevel, plMath::Pow(8, m_uiMaxTreeDepth - 1), 0xFFFFFFFF);
}

bool plDynamicOctree::FindObjectsInRange(const plVec3& vPoint, float fRadius, PLASMA_VISIBLE_OBJ_CALLBACK Callback, void* pPassThrough, float minx,
  float maxx, float miny, float maxy, float minz, float maxz, plUInt32 uiNodeID, plUInt32 uiAddID, plUInt32 uiSubAddID, plUInt32 uiNextNodeID) const
{
  if (vPoint.x + fRadius < minx)
    return true;
  if (vPoint.x - fRadius > maxx)
    return true;
  if (vPoint.y + fRadius < miny)
    return true;
  if (vPoint.y - fRadius > maxy)
    return true;
  if (vPoint.z + fRadius < minz)
    return true;
  if (vPoint.z - fRadius > maxz)
    return true;

  plDynamicTree::plMultiMapKey mmk;
  mmk.m_uiKey = uiNodeID;

  plDynamicTreeObjectConst it1 = m_NodeMap.LowerBound(mmk);

  // if the whole sub-tree doesn't contain any data, no need to check further
  if ((!it1.IsValid()) || (it1.Key().m_uiKey >= uiNextNodeID))
    return true;

  {

    // return all objects stored at this node
    {
      while (it1.IsValid() && (it1.Key().m_uiKey == uiNodeID))
      {
        // first increase the iterator, the user could erase it in the callback
        plDynamicTreeObjectConst temp = it1;
        ++it1;

        if (!Callback(pPassThrough, temp))
          return false;
      }
    }

    // if the node has children
    if (uiAddID > 0)
    {
      const float lx = ((maxx - minx) * 0.5f) * s_LooseOctreeFactor;
      const float ly = ((maxy - miny) * 0.5f) * s_LooseOctreeFactor;
      const float lz = ((maxz - minz) * 0.5f) * s_LooseOctreeFactor;

      const plUInt32 uiNodeIDBase = uiNodeID + 1;
      const plUInt32 uiAddIDChild = uiAddID - uiSubAddID;
      const plUInt32 uiSubAddIDChild = uiSubAddID >> 3;

      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 0,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 1))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 1,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 2))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 2,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 3))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, minx, minx + lx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 3,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 4))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, minz, minz + lz, uiNodeIDBase + uiAddID * 4,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 5))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, miny, miny + ly, maxz - lz, maxz, uiNodeIDBase + uiAddID * 5,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 6))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, minz, minz + lz, uiNodeIDBase + uiAddID * 6,
            uiAddIDChild, uiSubAddIDChild, uiNodeIDBase + uiAddID * 7))
        return false;
      if (!FindObjectsInRange(vPoint, fRadius, Callback, pPassThrough, maxx - lx, maxx, maxy - ly, maxy, maxz - lz, maxz, uiNodeIDBase + uiAddID * 7,
            uiAddIDChild, uiSubAddIDChild, uiNextNodeID))
        return false;
    }
  }

  return true;
}



PLASMA_STATICLINK_FILE(Utilities, Utilities_DataStructures_Implementation_DynamicOctree);
