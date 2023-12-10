#pragma once

#include <Core/CoreDLL.h>
#include <Core/World/SpatialData.h>
#include <Foundation/Communication/Message.h>
#include <Foundation/Math/BoundingBoxSphere.h>

struct PLASMA_CORE_DLL plMsgUpdateLocalBounds : public plMessage
{
  PLASMA_DECLARE_MESSAGE_TYPE(plMsgUpdateLocalBounds, plMessage);

  PLASMA_ALWAYS_INLINE void AddBounds(const plBoundingBoxSphere& bounds, plSpatialData::Category category)
  {
    m_ResultingLocalBounds.ExpandToInclude(bounds);
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

  ///\brief Enforces the object to be always visible. Note that you can't set this flag to false again,
  ///  because the same message is sent to multiple components and should accumulate the bounds.
  PLASMA_ALWAYS_INLINE void SetAlwaysVisible(plSpatialData::Category category)
  {
    m_bAlwaysVisible = true;
    m_uiSpatialDataCategoryBitmask |= category.GetBitmask();
  }

private:
  friend class plGameObject;

  plBoundingBoxSphere m_ResultingLocalBounds;
  plUInt32 m_uiSpatialDataCategoryBitmask = 0;
  bool m_bAlwaysVisible = false;
};
