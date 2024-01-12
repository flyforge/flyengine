#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Foundation/Basics.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <Physics/Body/BodyFilter.h>

class plCollisionFilterConfig;

namespace JPH
{
  using ObjectLayer = plUInt16;
} // namespace JPH

enum class plJoltBroadphaseLayer : plUInt8
{
  Static,
  Dynamic,
  Query,
  Trigger,
  Character,
  Ragdoll,
  Rope,
  Cloth,

  ENUM_COUNT
};

namespace plJoltCollisionFiltering
{
  /// \brief Constructs the JPH::ObjectLayer value from the desired collision group index and the broadphase into which the object shall be sorted
  PLASMA_JOLTPLUGIN_DLL JPH::ObjectLayer ConstructObjectLayer(plUInt8 uiCollisionGroup, plJoltBroadphaseLayer broadphase);

  PLASMA_JOLTPLUGIN_DLL void LoadCollisionFilters();

  PLASMA_JOLTPLUGIN_DLL plCollisionFilterConfig& GetCollisionFilterConfig();

  /// \brief Returns the (hard-coded) collision mask that determines which other broad-phases to collide with.
  PLASMA_JOLTPLUGIN_DLL plUInt32 GetBroadphaseCollisionMask(plJoltBroadphaseLayer broadphase);

}; // namespace plJoltCollisionFiltering


class plJoltObjectToBroadphaseLayer final : public JPH::BroadPhaseLayerInterface
{
public:
  virtual plUInt32 GetNumBroadPhaseLayers() const override;

  virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override;

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
  virtual const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override;
#endif
};

class plJoltBroadPhaseLayerFilter final : public JPH::BroadPhaseLayerFilter
{
public:
  plJoltBroadPhaseLayerFilter(plBitflags<plPhysicsShapeType> shapeTypes)
  {
    m_uiCollisionMask = shapeTypes.GetValue();
  }

  plUInt32 m_uiCollisionMask = 0;

  virtual bool ShouldCollide(JPH::BroadPhaseLayer inLayer) const override
  {
    return (PLASMA_BIT(static_cast<plUInt8>(inLayer)) & m_uiCollisionMask) != 0;
  }
};

class plJoltObjectLayerFilter final : public JPH::ObjectLayerFilter
{
public:
  plUInt32 m_uiCollisionLayer = 0;

  plJoltObjectLayerFilter(plUInt32 uiCollisionLayer)
    : m_uiCollisionLayer(uiCollisionLayer)
  {
  }

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer) const override;
};

class plJoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
  plJoltObjectVsBroadPhaseLayerFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override;
};

class plJoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
  plJoltObjectLayerPairFilter() = default;

  virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::ObjectLayer inLayer2) const override;
};

class plJoltBodyFilter final : public JPH::BodyFilter
{
public:
  plUInt32 m_uiObjectFilterIDToIgnore = plInvalidIndex - 1;

  plJoltBodyFilter(plUInt32 uiBodyFilterIdToIgnore = plInvalidIndex - 1)
    : m_uiObjectFilterIDToIgnore(uiBodyFilterIdToIgnore)
  {
  }

  void ClearFilter()
  {
    m_uiObjectFilterIDToIgnore = plInvalidIndex - 1;
  }

  virtual bool ShouldCollideLocked(const JPH::Body& body) const override
  {
    return body.GetCollisionGroup().GetGroupID() != m_uiObjectFilterIDToIgnore;
  }
};
