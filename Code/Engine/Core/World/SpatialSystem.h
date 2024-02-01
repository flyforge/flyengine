#pragma once

#include <Core/World/SpatialData.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Memory/CommonAllocators.h>
#include <Foundation/SimdMath/SimdBBoxSphere.h>
#include <Foundation/Types/TagSet.h>

class PL_CORE_DLL plSpatialSystem : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plSpatialSystem, plReflectedClass);

public:
  plSpatialSystem();
  ~plSpatialSystem();

  virtual void StartNewFrame();

  /// \name Spatial Data Functions
  ///@{

  virtual plSpatialDataHandle CreateSpatialData(const plSimdBBoxSphere& bounds, plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags) = 0;
  virtual plSpatialDataHandle CreateSpatialDataAlwaysVisible(plGameObject* pObject, plUInt32 uiCategoryBitmask, const plTagSet& tags) = 0;

  virtual void DeleteSpatialData(const plSpatialDataHandle& hData) = 0;

  virtual void UpdateSpatialDataBounds(const plSpatialDataHandle& hData, const plSimdBBoxSphere& bounds) = 0;
  virtual void UpdateSpatialDataObject(const plSpatialDataHandle& hData, plGameObject* pObject) = 0;

  ///@}
  /// \name Simple Queries
  ///@{

  using QueryCallback = plDelegate<plVisitorExecution::Enum(plGameObject*)>;

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  struct QueryStats
  {
    plUInt32 m_uiTotalNumObjects = 0;  ///< The total number of spatial objects in this system.
    plUInt32 m_uiNumObjectsTested = 0; ///< Number of objects tested for the query condition.
    plUInt32 m_uiNumObjectsPassed = 0; ///< Number of objects that passed the query condition.
    plTime m_TimeTaken;                ///< Time taken to execute the query
  };
#endif

  struct QueryParams
  {
    plUInt32 m_uiCategoryBitmask = 0;
    const plTagSet* m_pIncludeTags = nullptr;
    const plTagSet* m_pExcludeTags = nullptr;
#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
    QueryStats* m_pStats = nullptr;
#endif
  };

  virtual void FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_objects) const;
  virtual void FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, QueryCallback callback) const = 0;

  virtual void FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_objects) const;
  virtual void FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, QueryCallback callback) const = 0;

  ///@}
  /// \name Visibility Queries
  ///@{

  using IsOccludedFunc = plDelegate<bool(const plSimdBBox&)>;

  virtual void FindVisibleObjects(const plFrustum& frustum, const QueryParams& queryParams, plDynamicArray<const plGameObject*>& out_objects, IsOccludedFunc isOccluded, plVisibilityState visType) const = 0;

  /// \brief Retrieves a state describing how visible the object is.
  ///
  /// An object may be invisible, fully visible, or indirectly visible (through shadows or reflections).
  ///
  /// \param uiNumFramesBeforeInvisible Used to treat an object that was visible and just became invisible as visible for a few more frames.
  virtual plVisibilityState GetVisibilityState(const plSpatialDataHandle& hData, plUInt32 uiNumFramesBeforeInvisible) const = 0;

  ///@}

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
  virtual void GetInternalStats(plStringBuilder& ref_sSb) const;
#endif

protected:
  plProxyAllocator m_Allocator;

  plUInt64 m_uiFrameCounter = 0;
};
