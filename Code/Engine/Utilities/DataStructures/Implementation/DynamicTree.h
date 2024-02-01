#pragma once

#include <Foundation/Containers/Map.h>
#include <Foundation/Math/BoundingBox.h>
#include <Foundation/Math/Frustum.h>
#include <Foundation/Math/Vec3.h>
#include <Utilities/UtilitiesDLL.h>

struct plDynamicTree
{
  struct plObjectData
  {
    plInt32 m_iObjectType;
    plInt32 m_iObjectInstance;
  };

  struct plMultiMapKey
  {
    plUInt32 m_uiKey;
    plUInt32 m_uiCounter;

    plMultiMapKey()
    {
      m_uiKey = 0;
      m_uiCounter = 0;
    }

    inline bool operator<(const plMultiMapKey& rhs) const
    {
      if (m_uiKey == rhs.m_uiKey)
        return m_uiCounter < rhs.m_uiCounter;

      return m_uiKey < rhs.m_uiKey;
    }

    inline bool operator==(const plMultiMapKey& rhs) const { return (m_uiCounter == rhs.m_uiCounter && m_uiKey == rhs.m_uiKey); }
  };
};

using plDynamicTreeObject = plMap<plDynamicTree::plMultiMapKey, plDynamicTree::plObjectData>::Iterator;
using plDynamicTreeObjectConst = plMap<plDynamicTree::plMultiMapKey, plDynamicTree::plObjectData>::ConstIterator;

/// \brief Callback type for object queries. Return "false" to abort a search (e.g. when the desired element has been found).
using PL_VISIBLE_OBJ_CALLBACK = bool (*)(void*, plDynamicTreeObjectConst);

class plDynamicOctree;
class plDynamicQuadtree;
