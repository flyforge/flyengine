#include <Core/CorePCH.h>

#include <Core/World/SpatialSystem.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plSpatialSystem, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
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

void plSpatialSystem::FindObjectsInSphere(const plBoundingSphere& sphere, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInSphere(
    sphere, queryParams,
    [&](plGameObject* pObject) {
      out_objects.PushBack(pObject);

      return plVisitorExecution::Continue;
    });
}

void plSpatialSystem::FindObjectsInBox(const plBoundingBox& box, const QueryParams& queryParams, plDynamicArray<plGameObject*>& out_objects) const
{
  out_objects.Clear();

  FindObjectsInBox(
    box, queryParams,
    [&](plGameObject* pObject) {
      out_objects.PushBack(pObject);

      return plVisitorExecution::Continue;
    });
}

#if PL_ENABLED(PL_COMPILE_FOR_DEVELOPMENT)
void plSpatialSystem::GetInternalStats(plStringBuilder& ref_sSb) const
{
  ref_sSb.Clear();
}
#endif

PL_STATICLINK_FILE(Core, Core_World_Implementation_SpatialSystem);
