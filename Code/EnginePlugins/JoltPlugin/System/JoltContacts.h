#pragma once

#include <Core/World/Declarations.h>
#include <Physics/Collision/ContactListener.h>

class plWorld;
class plJoltTriggerComponent;
class plJoltContactEvents;

namespace JPH
{
  class SubShapeIDPair;
  class ContactSettings;
  class ContactManifold;
  class Body;
} // namespace JPH

class plJoltContactEvents
{
public:
  struct InteractionContact
  {
    plVec3 m_vPosition;
    plVec3 m_vNormal;
    const plSurfaceResource* m_pSurface;
    plTempHashedString m_sInteraction;
    float m_fImpulseSqr;
    float m_fDistanceSqr;
  };

  struct SlideAndRollInfo
  {
    const JPH::Body* m_pBody = nullptr;
    bool m_bStillSliding = false;
    bool m_bStillRolling = false;

    float m_fDistanceSqr;
    plVec3 m_vContactPosition;
    plGameObjectHandle m_hSlidePrefab;
    plGameObjectHandle m_hRollPrefab;
    plHashedString m_sSlideInteractionPrefab;
    plHashedString m_sRollInteractionPrefab;
  };

  plMutex m_Mutex;
  plWorld* m_pWorld = nullptr;
  plVec3 m_vMainCameraPosition = plVec3::MakeZero();
  plHybridArray<InteractionContact, 8> m_InteractionContacts; // these are spawned PER FRAME, so only a low number is necessary
  plHybridArray<SlideAndRollInfo, 4> m_SlidingOrRollingActors;

  SlideAndRollInfo* FindSlideOrRollInfo(const JPH::Body* pBody, const plVec3& vAvgPos);

  void OnContact_SlideReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal);

  void OnContact_RollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal0);

  void OnContact_ImpactReaction(const plVec3& vAvgPos, const plVec3& vAvgNormal, float fMaxImpactSqr, const plSurfaceResource* pSurface1, const plSurfaceResource* pSurface2, bool bActor1StaticOrKinematic);
  void OnContact_SlideAndRollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal, plBitflags<plOnJoltContact> combinedContactFlags);

  void SpawnPhysicsImpactReactions();
  void UpdatePhysicsSlideReactions();
  void UpdatePhysicsRollReactions();
};

class plJoltContactListener : public JPH::ContactListener
{
public:
  plWorld* m_pWorld = nullptr;
  plJoltContactEvents m_ContactEvents;

  struct TriggerObj
  {
    const plJoltTriggerComponent* m_pTrigger = nullptr;
    plGameObjectHandle m_hTarget;
  };

  plMutex m_TriggerMutex;
  plMap<plUInt64, TriggerObj> m_Trigs;

  void RemoveTrigger(const plJoltTriggerComponent* pTrigger);

  virtual void OnContactAdded(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;
  virtual void OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings) override;

  virtual void OnContactRemoved(const JPH::SubShapeIDPair& subShapePair) override;

  void OnContact(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings, bool bPersistent);

  bool ActivateTrigger(const JPH::Body& body1, const JPH::Body& body2, plUInt64 uiBody1id, plUInt64 uiBody2id);

  void DeactivateTrigger(plUInt64 uiBody1id, plUInt64 uiBody2id);
};
