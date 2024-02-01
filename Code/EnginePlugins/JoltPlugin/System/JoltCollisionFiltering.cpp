#include <JoltPlugin/JoltPluginPCH.h>

#include <GameEngine/Physics/CollisionFilter.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>

namespace plJoltCollisionFiltering
{
  plCollisionFilterConfig s_CollisionFilterConfig;

  JPH::ObjectLayer ConstructObjectLayer(plUInt8 uiCollisionGroup, plJoltBroadphaseLayer broadphase)
  {
    return static_cast<JPH::ObjectLayer>(static_cast<plUInt16>(broadphase) << 8 | static_cast<plUInt16>(uiCollisionGroup));
  }

  void LoadCollisionFilters()
  {
    PL_LOG_BLOCK("plJoltCore::LoadCollisionFilters");

    if (s_CollisionFilterConfig.Load().Failed())
    {
      plLog::Info("Collision filter config file could not be found ('{}'). Using default values.", plCollisionFilterConfig::s_sConfigFile);

      // setup some default config

      s_CollisionFilterConfig.SetGroupName(0, "Default");
      s_CollisionFilterConfig.EnableCollision(0, 0);
    }
  }

  plCollisionFilterConfig& GetCollisionFilterConfig()
  {
    return s_CollisionFilterConfig;
  }

  plUInt32 GetBroadphaseCollisionMask(plJoltBroadphaseLayer broadphase)
  {
    // this mapping defines which types of objects can generally collide with each other
    // if a flag is not included here, those types will never collide, no matter what their collision group is and other filter settings are
    // note that this is only used for the simulation, raycasts and shape queries can use their own mapping

    switch (broadphase)
    {
      case plJoltBroadphaseLayer::Static:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Character) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Rope) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Cloth);

      case plJoltBroadphaseLayer::Dynamic:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Static) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Trigger) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Character) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Rope) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Cloth);

      case plJoltBroadphaseLayer::Query:
        // query shapes never interact with anything in the simulation
        return 0;

      case plJoltBroadphaseLayer::Trigger:
        // triggers specifically exclude detail objects such as ropes, ragdolls and queries (also used for hitboxes) for performance reasons
        // if necessary, these shapes can still be found with overlap queries
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Character);

      case plJoltBroadphaseLayer::Character:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Static) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Trigger) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Character) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Cloth);

      case plJoltBroadphaseLayer::Ragdoll:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Static) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Rope) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Cloth);

      case plJoltBroadphaseLayer::Rope:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Static) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Rope);

      case plJoltBroadphaseLayer::Cloth:
        return PL_BIT((plUInt32)plJoltBroadphaseLayer::Static) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Character) | PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll);

        PL_DEFAULT_CASE_NOT_IMPLEMENTED;
    }

    return 0;
  };

} // namespace plJoltCollisionFiltering

plUInt32 plJoltObjectToBroadphaseLayer::GetNumBroadPhaseLayers() const
{
  return (plUInt32)plJoltBroadphaseLayer::ENUM_COUNT;
}

JPH::BroadPhaseLayer plJoltObjectToBroadphaseLayer::GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const
{
  return JPH::BroadPhaseLayer(inLayer >> 8);
}

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
const char* plJoltObjectToBroadphaseLayer::GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const
{
  switch (inLayer)
  {
    case Static:
      return "Static";

    case Dynamic:
      return "Dynamic";

    case Query:
      return "QueryShapes";

    case Trigger:
      return "Trigger";

    case Character:
      return "Character";

    case Ragdoll:
      return "Ragdoll";

    case Rope:
      return "Rope";

    case Cloth:
      return "Cloth";

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }
}
#endif

// if any of these asserts fails, plPhysicsShapeType and plJoltBroadphaseLayer are out of sync
static_assert(plPhysicsShapeType::Static == PL_BIT((plUInt32)plJoltBroadphaseLayer::Static));
static_assert(plPhysicsShapeType::Dynamic == PL_BIT((plUInt32)plJoltBroadphaseLayer::Dynamic));
static_assert(plPhysicsShapeType::Query == PL_BIT((plUInt32)plJoltBroadphaseLayer::Query));
static_assert(plPhysicsShapeType::Trigger == PL_BIT((plUInt32)plJoltBroadphaseLayer::Trigger));
static_assert(plPhysicsShapeType::Character == PL_BIT((plUInt32)plJoltBroadphaseLayer::Character));
static_assert(plPhysicsShapeType::Ragdoll == PL_BIT((plUInt32)plJoltBroadphaseLayer::Ragdoll));
static_assert(plPhysicsShapeType::Rope == PL_BIT((plUInt32)plJoltBroadphaseLayer::Rope));
static_assert(plPhysicsShapeType::Cloth == PL_BIT((plUInt32)plJoltBroadphaseLayer::Cloth));
static_assert(plPhysicsShapeType::Count == (plUInt32)plJoltBroadphaseLayer::ENUM_COUNT);

bool plJoltObjectLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer) const
{
  return plJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(m_uiCollisionLayer, static_cast<plUInt32>(inLayer) & 0xFF);
}

bool plJoltObjectVsBroadPhaseLayerFilter::ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const
{
  const plUInt32 uiMask1 = static_cast<plUInt32>(PL_BIT(inLayer1 >> 8));
  const plUInt32 uiMask2 = plJoltCollisionFiltering::GetBroadphaseCollisionMask(static_cast<plJoltBroadphaseLayer>((plUInt8)inLayer2));

  return (uiMask1 & uiMask2) != 0;
}

bool plJoltObjectLayerPairFilter::ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const
{
  return plJoltCollisionFiltering::s_CollisionFilterConfig.IsCollisionEnabled(static_cast<plUInt32>(inObject1) & 0xFF, static_cast<plUInt32>(inObject2) & 0xFF);
}


PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltCollisionFiltering);
