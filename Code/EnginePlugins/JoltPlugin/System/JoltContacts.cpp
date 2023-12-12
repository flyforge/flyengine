#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Prefabs/PrefabResource.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Foundation/Configuration/CVar.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltTriggerComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/System/JoltContacts.h>
#include <JoltPlugin/System/JoltDebugRenderer.h>
#include <JoltPlugin/System/JoltWorldModule.h>

plCVarInt cvar_PhysicsReactionsMaxImpacts("Jolt.Reactions.MaxImpacts", 4, plCVarFlags::Default, "Maximum number of impact reactions to spawn per frame.");
plCVarInt cvar_PhysicsReactionsMaxSlidesOrRolls("Jolt.Reactions.MaxSlidesOrRolls", 4, plCVarFlags::Default, "Maximum number of active slide or roll reactions.");
plCVarBool cvar_PhysicsReactionsVisImpacts("Jolt.Reactions.VisImpacts", false, plCVarFlags::Default, "Visualize where impact reactions are spawned.");
plCVarBool cvar_PhysicsReactionsVisDiscardedImpacts("Jolt.Reactions.VisDiscardedImpacts", false, plCVarFlags::Default, "Visualize where impact reactions were NOT spawned.");
plCVarBool cvar_PhysicsReactionsVisSlides("Jolt.Reactions.VisSlides", false, plCVarFlags::Default, "Visualize active slide reactions.");
plCVarBool cvar_PhysicsReactionsVisRolls("Jolt.Reactions.VisRolls", false, plCVarFlags::Default, "Visualize active roll reactions.");

void plJoltContactListener::RemoveTrigger(const plJoltTriggerComponent* pTrigger)
{
  PLASMA_LOCK(m_TriggerMutex);

  for (auto it = m_Trigs.GetIterator(); it.IsValid();)
  {
    if (it.Value().m_pTrigger == pTrigger)
    {
      it = m_Trigs.Remove(it);
    }
    else
    {
      ++it;
    }
  }
}

void plJoltContactListener::OnContactAdded(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings)
{
  const plUInt64 uiBody0id = body0.GetID().GetIndexAndSequenceNumber();
  const plUInt64 uiBody1id = body1.GetID().GetIndexAndSequenceNumber();

  if (ActivateTrigger(body0, body1, uiBody0id, uiBody1id))
    return;

  OnContact(body0, body1, manifold, ref_settings, false);
}

void plJoltContactListener::OnContactPersisted(const JPH::Body& body1, const JPH::Body& body2, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings)
{
  OnContact(body1, body2, manifold, ref_settings, true);
}

void plJoltContactListener::OnContactRemoved(const JPH::SubShapeIDPair& subShapePair)
{
  const plUInt64 uiBody1id = subShapePair.GetBody1ID().GetIndexAndSequenceNumber();
  const plUInt64 uiBody2id = subShapePair.GetBody2ID().GetIndexAndSequenceNumber();

  DeactivateTrigger(uiBody1id, uiBody2id);
}

void plJoltContactListener::OnContact(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, JPH::ContactSettings& ref_settings, bool bPersistent)
{
  // compute per-material friction and restitution
  {
    const plJoltMaterial* pMat0 = static_cast<const plJoltMaterial*>(body0.GetShape()->GetMaterial(manifold.mSubShapeID1));
    const plJoltMaterial* pMat1 = static_cast<const plJoltMaterial*>(body1.GetShape()->GetMaterial(manifold.mSubShapeID2));

    if (pMat0 && pMat1)
    {
      ref_settings.mCombinedRestitution = plMath::Max(pMat0->m_fRestitution, pMat1->m_fRestitution);
      ref_settings.mCombinedFriction = plMath::Sqrt(pMat0->m_fFriction * pMat1->m_fFriction);
    }
  }

  m_ContactEvents.m_pWorld = m_pWorld;

  const plJoltDynamicActorComponent* pActor0 = plJoltUserData::GetDynamicActorComponent(reinterpret_cast<const void*>(body0.GetUserData()));
  const plJoltDynamicActorComponent* pActor1 = plJoltUserData::GetDynamicActorComponent(reinterpret_cast<const void*>(body1.GetUserData()));

  if (pActor0 || pActor1)
  {
    const plBitflags<plOnJoltContact> ContactFlags0 = pActor0 ? pActor0->m_OnContact : plOnJoltContact::None;
    const plBitflags<plOnJoltContact> ContactFlags1 = pActor1 ? pActor1->m_OnContact : plOnJoltContact::None;

    plBitflags<plOnJoltContact> CombinedContactFlags;
    CombinedContactFlags.SetValue(ContactFlags0.GetValue() | ContactFlags1.GetValue());

    // bSendContactReport = bSendContactReport || CombinedContactFlags.IsSet(plOnJoltContact::SendReportMsg);

    if (CombinedContactFlags.IsAnySet(plOnJoltContact::AllReactions))
    {
      plVec3 vAvgPos(0);
      const plVec3 vAvgNormal = plJoltConversionUtils::ToVec3(manifold.mWorldSpaceNormal);

      const float fImpactSqr = (body0.GetLinearVelocity() - body1.GetLinearVelocity()).LengthSq();

      for (plUInt32 uiContactPointIndex = 0; uiContactPointIndex < manifold.mRelativeContactPointsOn1.size(); ++uiContactPointIndex)
      {
        vAvgPos += plJoltConversionUtils::ToVec3(manifold.GetWorldSpaceContactPointOn1(uiContactPointIndex));
        vAvgPos -= vAvgNormal * manifold.mPenetrationDepth;
      }

      vAvgPos /= (float)manifold.mRelativeContactPointsOn1.size();

      if (bPersistent)
      {
        m_ContactEvents.OnContact_SlideAndRollReaction(body0, body1, manifold, ContactFlags0, ContactFlags1, vAvgPos, vAvgNormal, CombinedContactFlags);
      }
      else if (fImpactSqr >= 1.0f && CombinedContactFlags.IsAnySet(plOnJoltContact::ImpactReactions))
      {
        const plJoltMaterial* pMat1 = static_cast<const plJoltMaterial*>(body0.GetShape()->GetMaterial(manifold.mSubShapeID1));
        const plJoltMaterial* pMat2 = static_cast<const plJoltMaterial*>(body1.GetShape()->GetMaterial(manifold.mSubShapeID2));

        if (pMat1 == nullptr)
          pMat1 = static_cast<const plJoltMaterial*>(plJoltMaterial::sDefault.GetPtr());
        if (pMat2 == nullptr)
          pMat2 = static_cast<const plJoltMaterial*>(plJoltMaterial::sDefault.GetPtr());

        m_ContactEvents.OnContact_ImpactReaction(vAvgPos, vAvgNormal, fImpactSqr, pMat1->m_pSurface, pMat2->m_pSurface, body0.IsStatic() || body0.IsKinematic());
      }
    }
  }

  //   if (bSendContactReport)
  //   {
  //     SendContactReport(pairHeader, pairs, nbPairs);
  //   }
}

bool plJoltContactListener::ActivateTrigger(const JPH::Body& body1, const JPH::Body& body2, plUInt64 uiBody1id, plUInt64 uiBody2id)
{
  if (!body1.IsSensor() && !body2.IsSensor())
    return false;

  const plJoltTriggerComponent* pTrigger = nullptr;
  const plComponent* pComponent = nullptr;

  if (body1.IsSensor())
  {
    pTrigger = plJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(body1.GetUserData()));
    pComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body2.GetUserData()));
  }
  else
  {
    pTrigger = plJoltUserData::GetTriggerComponent(reinterpret_cast<const void*>(body2.GetUserData()));
    pComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body1.GetUserData()));
  }

  if (pTrigger && pComponent)
  {
    pTrigger->PostTriggerMessage(pComponent->GetOwner()->GetHandle(), plTriggerState::Activated);

    PLASMA_LOCK(m_TriggerMutex);

    const plUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);
    auto& trig = m_Trigs[uiStoreID];
    trig.m_pTrigger = pTrigger;
    trig.m_hTarget = pComponent->GetOwner()->GetHandle();
  }

  // one of the bodies is a trigger
  return true;
}

void plJoltContactListener::DeactivateTrigger(plUInt64 uiBody1id, plUInt64 uiBody2id)
{
  PLASMA_LOCK(m_TriggerMutex);

  const plUInt64 uiStoreID = (uiBody1id < uiBody2id) ? (uiBody1id << 32 | uiBody2id) : (uiBody2id << 32 | uiBody1id);
  auto itTrig = m_Trigs.Find(uiStoreID);

  if (itTrig.IsValid())
  {
    itTrig.Value().m_pTrigger->PostTriggerMessage(itTrig.Value().m_hTarget, plTriggerState::Deactivated);
    m_Trigs.Remove(itTrig);
  }
}

//////////////////////////////////////////////////////////////////////////

void plJoltContactEvents::SpawnPhysicsImpactReactions()
{
  PLASMA_PROFILE_SCOPE("SpawnPhysicsImpactReactions");

  PLASMA_LOCK(m_Mutex);

  plUInt32 uiMaxPrefabsToSpawn = cvar_PhysicsReactionsMaxImpacts;

  for (const auto& ic : m_InteractionContacts)
  {
    if (ic.m_pSurface != nullptr)
    {
      if (uiMaxPrefabsToSpawn > 0 && ic.m_pSurface->InteractWithSurface(m_pWorld, plGameObjectHandle(), ic.m_vPosition, ic.m_vNormal, -ic.m_vNormal, ic.m_sInteraction, nullptr, ic.m_fImpulseSqr))
      {
        --uiMaxPrefabsToSpawn;

        if (cvar_PhysicsReactionsVisImpacts)
        {
          plDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, plColor::LightGreen, plTransform(ic.m_vPosition), plTime::Seconds(3));
        }
      }
      else
      {
        if (cvar_PhysicsReactionsVisDiscardedImpacts)
        {
          plDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, plColor::DarkGray, plTransform(ic.m_vPosition), plTime::Seconds(1));
        }
      }
    }
  }

  m_InteractionContacts.Clear();
}

void plJoltContactEvents::UpdatePhysicsSlideReactions()
{
  PLASMA_PROFILE_SCOPE("UpdatePhysicsSlideReactions");

  PLASMA_LOCK(m_Mutex);

  for (auto& slideInfo : m_SlidingOrRollingActors)
  {
    if (slideInfo.m_pBody == nullptr)
      continue;

    if (slideInfo.m_bStillSliding)
    {
      if (slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        plPrefabResourceHandle hPrefab = plResourceManager::LoadResource<plPrefabResource>(slideInfo.m_sSlideInteractionPrefab);
        plResourceLock<plPrefabResource> pPrefab(hPrefab, plResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == plResourceAcquireResult::Final)
        {
          plHybridArray<plGameObject*, 8> created;

          plPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, plTransform(slideInfo.m_vContactPosition), options);
          slideInfo.m_hSlidePrefab = created[0]->GetHandle();
        }
      }
      else
      {
        plGameObject* pObject;
        if (m_pWorld->TryGetObject(slideInfo.m_hSlidePrefab, pObject))
        {
          pObject->SetGlobalPosition(slideInfo.m_vContactPosition);
        }
        else
        {
          slideInfo.m_hSlidePrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisSlides)
      {
        plDebugRenderer::DrawLineBox(m_pWorld, plBoundingBox(plVec3(-0.5f), plVec3(0.5f)), plColor::BlueViolet, plTransform(slideInfo.m_vContactPosition));
      }

      slideInfo.m_bStillSliding = false;
    }
    else
    {
      if (!slideInfo.m_hSlidePrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(slideInfo.m_hSlidePrefab);
        slideInfo.m_hSlidePrefab.Invalidate();
      }
    }
  }
}

void plJoltContactEvents::UpdatePhysicsRollReactions()
{
  PLASMA_PROFILE_SCOPE("UpdatePhysicsRollReactions");

  PLASMA_LOCK(m_Mutex);

  for (auto& rollInfo : m_SlidingOrRollingActors)
  {
    if (rollInfo.m_pBody == nullptr)
      continue;

    if (rollInfo.m_bStillRolling)
    {
      if (rollInfo.m_hRollPrefab.IsInvalidated())
      {
        plPrefabResourceHandle hPrefab = plResourceManager::LoadResource<plPrefabResource>(rollInfo.m_sRollInteractionPrefab);
        plResourceLock<plPrefabResource> pPrefab(hPrefab, plResourceAcquireMode::AllowLoadingFallback_NeverFail);
        if (pPrefab.GetAcquireResult() == plResourceAcquireResult::Final)
        {
          plHybridArray<plGameObject*, 8> created;

          plPrefabInstantiationOptions options;
          options.m_pCreatedRootObjectsOut = &created;
          options.m_bForceDynamic = true;

          pPrefab->InstantiatePrefab(*m_pWorld, plTransform(rollInfo.m_vContactPosition), options);
          rollInfo.m_hRollPrefab = created[0]->GetHandle();
        }
      }
      else
      {
        plGameObject* pObject;
        if (m_pWorld->TryGetObject(rollInfo.m_hRollPrefab, pObject))
        {
          pObject->SetGlobalPosition(rollInfo.m_vContactPosition);
        }
        else
        {
          rollInfo.m_hRollPrefab.Invalidate();
        }
      }

      if (cvar_PhysicsReactionsVisRolls)
      {
        plDebugRenderer::DrawLineCapsuleZ(m_pWorld, 0.4f, 0.2f, plColor::GreenYellow, plTransform(rollInfo.m_vContactPosition));
      }

      rollInfo.m_bStillRolling = false;
      rollInfo.m_bStillSliding = false; // ensures that no slide reaction is spawned as well
    }
    else
    {
      if (!rollInfo.m_hRollPrefab.IsInvalidated())
      {
        m_pWorld->DeleteObjectDelayed(rollInfo.m_hRollPrefab);
        rollInfo.m_hRollPrefab.Invalidate();
      }
    }
  }
}

void plJoltContactEvents::OnContact_ImpactReaction(const plVec3& vAvgPos, const plVec3& vAvgNormal, float fMaxImpactSqr, const plSurfaceResource* pSurface1, const plSurfaceResource* pSurface2, bool bActor1StaticOrKinematic)
{
  const float fDistanceSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  PLASMA_LOCK(m_Mutex);

  InteractionContact* ic = nullptr;

  if (m_InteractionContacts.GetCount() < (plUInt32)cvar_PhysicsReactionsMaxImpacts * 2)
  {
    ic = &m_InteractionContacts.ExpandAndGetRef();
    ic->m_pSurface = nullptr;
    ic->m_fDistanceSqr = plMath::HighValue<float>();
  }
  else
  {
    // compute a score, which contact point is best to replace
    // * prefer to replace points that are farther away than the new one
    // * prefer to replace points that have a lower impact strength than the new one

    float fBestScore = 0;
    plUInt32 uiBestScore = 0xFFFFFFFFu;

    for (plUInt32 i = 0; i < m_InteractionContacts.GetCount(); ++i)
    {
      float fScore = 0;
      fScore += m_InteractionContacts[i].m_fDistanceSqr - fDistanceSqr;
      fScore += 2.0f * (fMaxImpactSqr - m_InteractionContacts[i].m_fImpulseSqr);

      if (fScore > fBestScore)
      {
        fBestScore = fScore;
        uiBestScore = i;
      }
    }

    if (uiBestScore == 0xFFFFFFFFu)
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        plDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, plColor::DimGrey, plTransform(vAvgPos), plTime::Seconds(3));
      }

      return;
    }
    else
    {
      if (cvar_PhysicsReactionsVisDiscardedImpacts)
      {
        plDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, plColor::DimGrey, plTransform(m_InteractionContacts[uiBestScore].m_vPosition), plTime::Seconds(3));
      }
    }

    // this is the best candidate to replace
    ic = &m_InteractionContacts[uiBestScore];
  }

  if (pSurface1 || pSurface2)
  {
    // if one of the objects doesn't have a surface configured, use the other one
    if (pSurface1 == nullptr)
      pSurface1 = pSurface2;
    if (pSurface2 == nullptr)
      pSurface2 = pSurface1;

    ic->m_fDistanceSqr = fDistanceSqr;
    ic->m_vPosition = vAvgPos;
    ic->m_vNormal = vAvgNormal;
    ic->m_vNormal.NormalizeIfNotZero(plVec3(0, 0, 1)).IgnoreResult();
    ic->m_fImpulseSqr = fMaxImpactSqr;

    // if one actor is static or kinematic, prefer to spawn the interaction from its surface definition
    if (bActor1StaticOrKinematic)
    {
      ic->m_pSurface = pSurface1;
      ic->m_sInteraction = pSurface2->GetDescriptor().m_sOnCollideInteraction;
    }
    else
    {
      ic->m_pSurface = pSurface2;
      ic->m_sInteraction = pSurface1->GetDescriptor().m_sOnCollideInteraction;
    }

    return;
  }

  if (cvar_PhysicsReactionsVisDiscardedImpacts)
  {
    plDebugRenderer::AddPersistentCross(m_pWorld, 1.0f, plColor::DarkOrange, plTransform(vAvgPos), plTime::Seconds(10));
  }
}

plJoltContactEvents::SlideAndRollInfo* plJoltContactEvents::FindSlideOrRollInfo(const JPH::Body* pBody, const plVec3& vAvgPos)
{
  SlideAndRollInfo* pUnused = nullptr;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (info.m_pBody == pBody)
      return &info;

    if (info.m_pBody == nullptr)
      pUnused = &info;
  }

  const float fDistSqr = (vAvgPos - m_vMainCameraPosition).GetLengthSquared();

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  if (m_SlidingOrRollingActors.GetCount() < (plUInt32)cvar_PhysicsReactionsMaxSlidesOrRolls)
  {
    pUnused = &m_SlidingOrRollingActors.ExpandAndGetRef();
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  float fBestDist = 0.0f;

  for (auto& info : m_SlidingOrRollingActors)
  {
    if (!info.m_hRollPrefab.IsInvalidated() || !info.m_hSlidePrefab.IsInvalidated())
      continue;

    // this slot is not yet really in use, so can be replaced by a better match

    if (fDistSqr < info.m_fDistanceSqr && info.m_fDistanceSqr > fBestDist)
    {
      fBestDist = info.m_fDistanceSqr;
      pUnused = &info;
    }
  }

  if (pUnused != nullptr)
  {
    pUnused->m_fDistanceSqr = fDistSqr;
    pUnused->m_pBody = pBody;
    return pUnused;
  }

  return nullptr;
}

void plJoltContactEvents::OnContact_RollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal0)
{
  // only consider something 'rolling' when it turns faster than this (per second)
  constexpr plAngle rollThreshold = plAngle::Degree(45);

  plBitflags<plOnJoltContact> contactFlags[2] = {onContact0, onContact1};
  const JPH::Body* bodies[2] = {&body0, &body1};
  const JPH::SubShapeID shapeIds[2] = {manifold.mSubShapeID1, manifold.mSubShapeID2};

  for (plUInt32 i = 0; i < 2; ++i)
  {
    if (contactFlags[i].IsAnySet(plOnJoltContact::AllRollReactions))
    {
      const plVec3 vAngularVel = plJoltConversionUtils::ToVec3(bodies[i]->GetRotation().InverseRotate(bodies[i]->GetAngularVelocity()));

      if ((contactFlags[i].IsSet(plOnJoltContact::RollXReactions) && plMath::Abs(vAngularVel.x) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(plOnJoltContact::RollYReactions) && plMath::Abs(vAngularVel.y) > rollThreshold.GetRadian()) ||
          (contactFlags[i].IsSet(plOnJoltContact::RollZReactions) && plMath::Abs(vAngularVel.z) > rollThreshold.GetRadian()))
      {
        const plJoltMaterial* pMaterial = static_cast<const plJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

        if (pMaterial && pMaterial->m_pSurface)
        {
          if (!pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab.IsEmpty())
          {
            PLASMA_LOCK(m_Mutex);

            if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
            {
              pInfo->m_bStillRolling = true;
              pInfo->m_vContactPosition = vAvgPos;
              pInfo->m_sRollInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sRollInteractionPrefab;
            }
          }
        }
      }
    }
  }
}

void plJoltContactEvents::OnContact_SlideReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal0)
{
  plVec3 vVelocity[2] = {plVec3::ZeroVector(), plVec3::ZeroVector()};

  {
    vVelocity[0] = plJoltConversionUtils::ToVec3(body0.GetLinearVelocity());

    if (!vVelocity[0].IsValid())
      vVelocity[0].SetZero();
  }

  {
    vVelocity[1] = plJoltConversionUtils::ToVec3(body1.GetLinearVelocity());

    if (!vVelocity[1].IsValid())
      vVelocity[1].SetZero();
  }

  const plVec3 vRelativeVelocity = vVelocity[1] - vVelocity[0];

  if (!vRelativeVelocity.IsZero(0.0001f))
  {
    const plVec3 vRelativeVelocityDir = vRelativeVelocity.GetNormalized();

    plVec3 vAvgNormal = vAvgNormal0;
    vAvgNormal.NormalizeIfNotZero(plVec3::UnitZAxis()).IgnoreResult();

    // an object is only 'sliding' if it moves at roughly 90 degree along another object
    constexpr float slideAngle = 0.17f; // plMath ::Cos(plAngle::Degree(80));

    if (plMath::Abs(vAvgNormal.Dot(vRelativeVelocityDir)) < slideAngle)
    {
      constexpr float slideSpeedThreshold = 0.5f; // in meters per second

      if (vRelativeVelocity.GetLengthSquared() > plMath::Square(slideSpeedThreshold))
      {
        plBitflags<plOnJoltContact> contactFlags[2] = {onContact0, onContact1};
        const JPH::Body* bodies[2] = {&body0, &body1};
        const JPH::SubShapeID shapeIds[2] = {manifold.mSubShapeID1, manifold.mSubShapeID2};

        for (plUInt32 i = 0; i < 2; ++i)
        {
          if (contactFlags[i].IsAnySet(plOnJoltContact::SlideReactions))
          {
            const plJoltMaterial* pMaterial = static_cast<const plJoltMaterial*>(bodies[i]->GetShape()->GetMaterial(shapeIds[i]));

            if (pMaterial && pMaterial->m_pSurface)
            {
              if (!pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab.IsEmpty())
              {
                PLASMA_LOCK(m_Mutex);

                if (auto pInfo = FindSlideOrRollInfo(bodies[i], vAvgPos))
                {
                  if (!pInfo->m_bStillRolling)
                  {
                    pInfo->m_bStillSliding = true;
                    pInfo->m_vContactPosition = vAvgPos;
                    pInfo->m_sSlideInteractionPrefab = pMaterial->m_pSurface->GetDescriptor().m_sSlideInteractionPrefab;
                  }
                }
              }
            }
          }
        }
      }
    }
  }
}

void plJoltContactEvents::OnContact_SlideAndRollReaction(const JPH::Body& body0, const JPH::Body& body1, const JPH::ContactManifold& manifold, plBitflags<plOnJoltContact> onContact0, plBitflags<plOnJoltContact> onContact1, const plVec3& vAvgPos, const plVec3& vAvgNormal, plBitflags<plOnJoltContact> combinedContactFlags)
{
  if (manifold.mRelativeContactPointsOn1.size() >= 2 && combinedContactFlags.IsAnySet(plOnJoltContact::SlideReactions))
  {
    OnContact_SlideReaction(body0, body1, manifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }

  if (combinedContactFlags.IsAnySet(plOnJoltContact::AllRollReactions))
  {
    OnContact_RollReaction(body0, body1, manifold, onContact0, onContact1, vAvgPos, vAvgNormal);
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltContacts);

