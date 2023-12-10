#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem.h>

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpatialSystem, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plSpatialSystem::plSpatialSystem()
  : m_Allocator("Spatial System", plFoundation::GetDefaultAllocator())
{
}

plSpatialSystem::~plSpatialSystem() = default;

void plSpatialSystem::StartNewFrame()
{
  ++m_uiFrameCounter;
}

void plSpatialSystem::FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_Objects) const
{
  out_Objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](plGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return plVisitorExecution::Continue;
    });
}

void plSpatialSystem::FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_Objects) const
{
  out_Objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](plGameObject* pObject) {
      out_Objects.PushBack(pObject);

      return plVisitorExecution::Continue;
    });
}

#if PLASMA_ENABLED(PLASMA_COMPILE_FOR_DEVELOPMENT)
void plSpatialSystem::GetInternalStats(plStringBuilder& sb) const
{
  sb.Clear();
}
#endif

PLASMA_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
