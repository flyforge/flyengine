#pragma once

#include <Core/Interfaces/PhysicsWorldModule.h>
#include <Core/World/Declarations.h>
#include <Core/World/WorldModule.h>
#include <Foundation/Types/UniquePtr.h>
#include <JoltPlugin/Declarations.h>
#include <JoltPlugin/JoltPluginDLL.h>
#include <JoltPlugin/System/JoltCollisionFiltering.h>
#include <JoltPlugin/Utilities/JoltUserData.h>
#include <RendererCore/Meshes/DynamicMeshBufferResource.h>

class plJoltCharacterControllerComponent;
class plJoltContactListener;
class plJoltRagdollComponent;
class plJoltRopeComponent;
class plView;

namespace JPH
{
  class Body;
  class TempAllocator;
  class PhysicsSystem;
  class GroupFilter;
} // namespace JPH

class PL_JOLTPLUGIN_DLL plJoltWorldModule : public plPhysicsWorldModuleInterface
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plJoltWorldModule, plPhysicsWorldModuleInterface);

public:
  plJoltWorldModule(plWorld* pWorld);
  ~plJoltWorldModule();

  virtual void Initialize() override;
  virtual void Deinitialize() override;
  virtual void OnSimulationStarted() override;

  JPH::PhysicsSystem* GetJoltSystem() { return m_pSystem.get(); }
  const JPH::PhysicsSystem* GetJoltSystem() const { return m_pSystem.get(); }

  plUInt32 CreateObjectFilterID();
  void DeleteObjectFilterID(plUInt32& ref_uiObjectFilterID);

  plUInt32 AllocateUserData(plJoltUserData*& out_pUserData);
  void DeallocateUserData(plUInt32& ref_uiUserDataId);
  const plJoltUserData& GetUserData(plUInt32 uiUserDataId) const;

  void SetGravity(const plVec3& vObjectGravity, const plVec3& vCharacterGravity);
  virtual plVec3 GetGravity() const override { return plVec3(0, 0, -10); }
  plVec3 GetCharacterGravity() const { return m_Settings.m_vCharacterGravity; }

  //////////////////////////////////////////////////////////////////////////
  // plPhysicsWorldModuleInterface
  //

  virtual plUInt32 GetCollisionLayerByName(plStringView sName) const override;

  virtual bool Raycast(plPhysicsCastResult& out_result, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const override;

  virtual bool RaycastAll(plPhysicsCastResultArray& out_results, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params) const override;

  virtual bool SweepTestSphere(plPhysicsCastResult& out_result, float fSphereRadius, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestBox(plPhysicsCastResult& out_result, plVec3 vBoxExtends, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const override;

  virtual bool SweepTestCapsule(plPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection = plPhysicsHitCollection::Closest) const override;

  virtual bool OverlapTestSphere(float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const override;

  virtual bool OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plPhysicsQueryParameters& params) const override;

  virtual void QueryShapesInSphere(plPhysicsOverlapResultArray& out_results, float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const override;

  virtual void QueryGeometryInBox(const plPhysicsQueryParameters& params, plBoundingBox box, plDynamicArray<plPhysicsTriangle>& out_triangles) const override;

  virtual void AddStaticCollisionBox(plGameObject* pObject, plVec3 vBoxSize) override;

  virtual void AddFixedJointComponent(plGameObject* pOwner, const plPhysicsWorldModuleInterface::FixedJointConfig& cfg) override;

  virtual plBoundingBoxSphere GetWorldSpaceBounds(plGameObject* pOwner, plUInt32 uiCollisionLayer, plBitflags<plPhysicsShapeType> shapeTypes, bool bIncludeChildObjects) const override;

  plDeque<plComponentHandle> m_RequireUpdate;

  const plSet<plJoltDynamicActorComponent*>& GetActiveActors() const { return m_ActiveActors; }
  const plMap<plJoltRagdollComponent*, plInt32>& GetActiveRagdolls() const { return m_ActiveRagdolls; }
  const plMap<plJoltRopeComponent*, plInt32>& GetActiveRopes() const { return m_ActiveRopes; }
  plArrayPtr<plJoltRagdollComponent*> GetRagdollsPutToSleep() { return m_RagdollsPutToSleep.GetArrayPtr(); }

  /// \brief Returns a uint32 that can be queried for completion with IsBodyStillQueuedToAdd().
  plUInt32 QueueBodyToAdd(JPH::Body* pBody, bool bAwake);

  /// \brief Checks whether the last QueueBodyToAdd() has been processed already, or not.
  ///
  /// Bodies that aren't added to Jolt yet, may not get locked (they are not in the broadphase).
  /// If this is still the case, skip operations that wouldn't have an effect anyway.
  bool IsBodyStillQueuedToAdd(plUInt32 uiBodiesAddCounter) const { return uiBodiesAddCounter == m_uiBodiesAddCounter; }

  JPH::GroupFilter* GetGroupFilter() const { return m_pGroupFilter; }
  JPH::GroupFilter* GetGroupFilterIgnoreSame() const { return m_pGroupFilterIgnoreSame; }

  void EnableJoinedBodiesCollisions(plUInt32 uiObjectFilterID1, plUInt32 uiObjectFilterID2, bool bEnable);

  JPH::TempAllocator* GetTempAllocator() const { return m_pTempAllocator.get(); }

  void ActivateCharacterController(plJoltCharacterControllerComponent* pCharacter, bool bActivate);

  plJoltContactListener* GetContactListener()
  {
    return reinterpret_cast<plJoltContactListener*>(m_pContactListener);
  }

  void CheckBreakableConstraints();

  plSet<plComponentHandle> m_BreakableConstraints;


private:
  bool SweepTest(plPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection) const;
  bool OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const plPhysicsQueryParameters& params) const;

  void FreeUserDataAfterSimulationStep();

  void StartSimulation(const plWorldModule::UpdateContext& context);
  void FetchResults(const plWorldModule::UpdateContext& context);

  void Simulate();

  void UpdateSettingsCfg();
  void ApplySettingsCfg();

  void UpdateConstraints();

  plTime CalculateUpdateSteps();

  void DebugDrawGeometry();
  void DebugDrawGeometry(const plVec3& vCenter, float fRadius, plPhysicsShapeType::Enum shapeType, const plTag& tag);

  struct DebugGeo
  {
    plGameObjectHandle m_hObject;
    plUInt32 m_uiLastSeenCounter = 0;
    bool m_bMutableGeometry = false;
  };

  struct DebugGeoShape
  {
    plDynamicMeshBufferResourceHandle m_hMesh;
    plBoundingBox m_Bounds;
    plUInt32 m_uiLastSeenCounter = 0;
  };

  struct DebugBodyShapeKey
  {
    plUInt32 m_uiBodyID;
    const void* m_pShapePtr;

    bool operator<(const DebugBodyShapeKey& rhs) const
    {
      if (m_uiBodyID == rhs.m_uiBodyID)
        return m_pShapePtr < rhs.m_pShapePtr;

      return m_uiBodyID < rhs.m_uiBodyID;
    }

    bool operator==(const DebugBodyShapeKey& rhs) const
    {
      return (m_uiBodyID == rhs.m_uiBodyID) && (m_pShapePtr == rhs.m_pShapePtr);
    }
  };

  plUInt32 m_uiDebugGeoLastSeenCounter = 0;
  plMap<DebugBodyShapeKey, DebugGeo> m_DebugDrawComponents;
  plMap<const void*, DebugGeoShape> m_DebugDrawShapeGeo;

  plUInt32 m_uiNextObjectFilterID = 1;
  plDynamicArray<plUInt32> m_FreeObjectFilterIDs;

  plDeque<plJoltUserData> m_AllocatedUserData;
  plDynamicArray<plUInt32> m_FreeUserData;
  plDynamicArray<plUInt32> m_FreeUserDataAfterSimulationStep;

  plTime m_AccumulatedTimeSinceUpdate;

  plJoltSettings m_Settings;

  plSharedPtr<plTask> m_pSimulateTask;
  plTaskGroupID m_SimulateTaskGroupId;
  plTime m_SimulatedTimeStep;

  std::unique_ptr<JPH::PhysicsSystem> m_pSystem;
  std::unique_ptr<JPH::TempAllocator> m_pTempAllocator;

  plJoltObjectToBroadphaseLayer m_ObjectToBroadphase;
  plJoltObjectVsBroadPhaseLayerFilter m_ObjectVsBroadphaseFilter;
  plJoltObjectLayerPairFilter m_ObjectLayerPairFilter;

  void* m_pContactListener = nullptr;
  void* m_pActivationListener = nullptr;
  plSet<plJoltDynamicActorComponent*> m_ActiveActors;
  plMap<plJoltRagdollComponent*, plInt32> m_ActiveRagdolls;
  plMap<plJoltRopeComponent*, plInt32> m_ActiveRopes;
  plDynamicArray<plJoltRagdollComponent*> m_RagdollsPutToSleep;

  JPH::GroupFilter* m_pGroupFilter = nullptr;
  JPH::GroupFilter* m_pGroupFilterIgnoreSame = nullptr;

  plUInt32 m_uiBodiesAddedSinceOptimize = 100;
  plUInt32 m_uiBodiesAddCounter = 1; // increased every time bodies get added, can be used to check whether a queued body is still queued
  plDeque<plUInt32> m_BodiesToAdd;
  plDeque<plUInt32> m_BodiesToAddAndActivate;

  plHybridArray<plTime, 4> m_UpdateSteps;
  plHybridArray<plJoltCharacterControllerComponent*, 4> m_ActiveCharacters;
};
