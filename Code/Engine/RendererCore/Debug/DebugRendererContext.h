#pragma once

#include <Foundation/Algorithm/HashingUtils.h>
#include <RendererCore/RendererCoreDLL.h>

class plWorld;
class plViewHandle;

/// \brief Used in plDebugRenderer to determine where debug geometry should be rendered
class PLASMA_RENDERERCORE_DLL plDebugRendererContext
{
public:
  PLASMA_ALWAYS_INLINE plDebugRendererContext()
    : m_uiId(-1)
  {
  }

  /// \brief If this constructor is used, the geometry is rendered in all views for that scene.
  plDebugRendererContext(const plWorld* pWorld);

  /// \brief If this constructor is used, the geometry is only rendered in this view.
  plDebugRendererContext(const plViewHandle& hView);

  PLASMA_ALWAYS_INLINE bool operator==(const plDebugRendererContext& other) const { return m_uiId == other.m_uiId; }

private:
  friend struct plHashHelper<plDebugRendererContext>;

  plUInt32 m_uiId;
};


template <>
struct plHashHelper<plDebugRendererContext>
{
  PLASMA_ALWAYS_INLINE static plUInt32 Hash(plDebugRendererContext value) { return plHashHelper<plUInt32>::Hash(value.m_uiId); }

  PLASMA_ALWAYS_INLINE static bool Equal(plDebugRendererContext a, plDebugRendererContext b) { return a == b; }
};
