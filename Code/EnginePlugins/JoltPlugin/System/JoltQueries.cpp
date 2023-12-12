#include <JoltPlugin/JoltPluginPCH.h>

#include <JoltPlugin/Actors/JoltActorComponent.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltUserData.h>

void FillCastResult(plPhysicsCastResult& ref_result, const plVec3& vStart, const plVec3& vDir, float fDistance, const JPH::BodyID& bodyId, const JPH::SubShapeID& subShapeId, const JPH::BodyLockInterface& lockInterface, const JPH::BodyInterface& bodyInterface, const plJoltWorldModule* pModule)
{
  JPH::BodyLockRead bodyLock(lockInterface, bodyId);
  const auto& body = bodyLock.GetBody();
  ref_result.m_vNormal = plJoltConversionUtils::ToVec3(body.GetWorldSpaceSurfaceNormal(subShapeId, plJoltConversionUtils::ToVec3(ref_result.m_vPosition)));
  ref_result.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();

  if (plComponent* pShapeComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(subShapeId))))
  {
    ref_result.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
  }

  if (plComponent* pActorComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
  {
    ref_result.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
  }

  if (const plJoltMaterial* pMaterial = static_cast<const plJoltMaterial*>(bodyInterface.GetMaterial(bodyId, subShapeId)))
  {
    ref_result.m_hSurface = pMaterial->m_pSurface->GetResourceHandle();
  }

  const size_t uiBodyId = bodyId.GetIndexAndSequenceNumber();
  const size_t uiShapeId = subShapeId.GetValue();
  ref_result.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
  ref_result.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);
}

class plRayCastCollector : public JPH::CastRayCollector
{
public:
  JPH::RayCastResult m_Result;
  bool m_bAnyHit = false;
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    if (result.mFraction < m_Result.mFraction)
    {
      m_Result = result;
      m_bFoundAny = true;

      if (m_bAnyHit)
      {
        ForceEarlyOut();
      }
    }
  }
};

bool plJoltWorldModule::Raycast(plPhysicsCastResult& out_result, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection /*= plPhysicsHitCollection::Closest*/) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin = plJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = plJoltConversionUtils::ToVec3(vDir * fDistance);

  plRayCastCollector collector;
  collector.m_bAnyHit = collection == plPhysicsHitCollection::Any;

  plJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  plJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  plJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  if (params.m_bIgnoreInitialOverlap)
  {
    JPH::RayCastSettings opt;
    opt.mBackFaceMode = JPH::EBackFaceMode::IgnoreBackFaces;
    opt.mTreatConvexAsSolid = false;

    query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

    if (collector.m_bFoundAny == false)
      return false;
  }
  else
  {
    if (!query.CastRay(ray, collector.m_Result, broadphaseFilter, objectFilter, bodyFilter))
      return false;
  }

  out_result.m_fDistance = collector.m_Result.mFraction * fDistance;
  out_result.m_vPosition = vStart + fDistance * collector.m_Result.mFraction * vDir;

  FillCastResult(out_result, vStart, vDir, fDistance, collector.m_Result.mBodyID, collector.m_Result.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class plRayCastCollectorAll : public JPH::CastRayCollector
{
public:
  plArrayPtr<JPH::RayCastResult> m_Results;
  plUInt32 m_uiFound = 0;

  virtual void AddHit(const JPH::RayCastResult& result) override
  {
    m_Results[m_uiFound] = result;
    ++m_uiFound;
  }
};

bool plJoltWorldModule::RaycastAll(plPhysicsCastResultArray& out_results, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params) const
{
  if (fDistance <= 0.001f || vDir.IsZero())
    return false;

  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  JPH::RRayCast ray;
  ray.mOrigin = plJoltConversionUtils::ToVec3(vStart);
  ray.mDirection = plJoltConversionUtils::ToVec3(vDir * fDistance);

  plRayCastCollectorAll collector;
  collector.m_Results = PLASMA_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), JPH::RayCastResult, 256);

  plJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  plJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  plJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RayCastSettings opt;
  opt.mBackFaceMode = params.m_bIgnoreInitialOverlap ? JPH::EBackFaceMode::IgnoreBackFaces : JPH::EBackFaceMode::CollideWithBackFaces;
  opt.mTreatConvexAsSolid = !params.m_bIgnoreInitialOverlap;

  query.CastRay(ray, opt, collector, broadphaseFilter, objectFilter, bodyFilter);

  if (collector.m_uiFound == 0)
    return false;

  out_results.m_Results.SetCount(collector.m_uiFound);

  for (plUInt32 i = 0; i < collector.m_uiFound; ++i)
  {
    out_results.m_Results[i].m_fDistance = collector.m_Results[i].mFraction * fDistance;
    out_results.m_Results[i].m_vPosition = vStart + fDistance * collector.m_Results[i].mFraction * vDir;

    FillCastResult(out_results.m_Results[i], vStart, vDir, fDistance, collector.m_Results[i].mBodyID, collector.m_Results[i].mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);
  }

  return true;
}

class plJoltShapeCastCollector : public JPH::CastShapeCollector
{
public:
  JPH::ShapeCastResult m_Result;
  bool m_bFoundAny = false;
  bool m_bAnyHit = false;

  virtual void AddHit(const JPH::ShapeCastResult& result) override
  {
    if (result.mIsBackFaceHit)
      return;

    if (result.mFraction >= GetEarlyOutFraction())
      return;

    m_bFoundAny = true;
    m_Result = result;

    UpdateEarlyOutFraction(result.mFraction);

    if (m_bAnyHit)
      ForceEarlyOut();
  }
};

bool plJoltWorldModule::SweepTestSphere(plPhysicsCastResult& out_result, float fSphereRadius, const plVec3& vStart, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return SweepTest(out_result, shape, JPH::Mat44::sTranslation(plJoltConversionUtils::ToVec3(vStart)), vDir, fDistance, params, collection);
}

bool plJoltWorldModule::SweepTestBox(plPhysicsCastResult& out_result, plVec3 vBoxExtends, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection) const
{
  const JPH::BoxShape shape(plJoltConversionUtils::ToVec3(vBoxExtends * 0.5f));

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(plJoltConversionUtils::ToQuat(transform.m_qRotation), plJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool plJoltWorldModule::SweepTestCapsule(plPhysicsCastResult& out_result, float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  plQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(plVec3(1, 0, 0), plAngle::Degree(90.0f));

  plQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(plJoltConversionUtils::ToQuat(qRot), plJoltConversionUtils::ToVec3(transform.m_vPosition));

  return SweepTest(out_result, shape, trans, vDir, fDistance, params, collection);
}

bool plJoltWorldModule::SweepTest(plPhysicsCastResult& out_Result, const JPH::Shape& shape, const JPH::Mat44& transform, const plVec3& vDir, float fDistance, const plPhysicsQueryParameters& params, plPhysicsHitCollection collection) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  plJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  plJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  plJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  JPH::RShapeCast cast(&shape, JPH::Vec3(1, 1, 1), transform, plJoltConversionUtils::ToVec3(vDir * fDistance));

  plJoltShapeCastCollector collector;
  collector.m_bAnyHit = collection == plPhysicsHitCollection::Any;

  query.CastShape(cast, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  if (!collector.m_bFoundAny)
    return false;

  const auto& res = collector.m_Result;

  out_Result.m_fDistance = res.mFraction * fDistance;
  out_Result.m_vPosition = plJoltConversionUtils::ToVec3(res.mContactPointOn2);

  FillCastResult(out_Result, plJoltConversionUtils::ToVec3(transform.GetTranslation()), vDir, fDistance, res.mBodyID2, res.mSubShapeID2, m_pSystem->GetBodyLockInterfaceNoLock(), m_pSystem->GetBodyInterfaceNoLock(), this);

  return true;
}

class plJoltShapeCollectorAny : public JPH::CollideShapeCollector
{
public:
  bool m_bFoundAny = false;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_bFoundAny = true;
    ForceEarlyOut();
  }
};

class plJoltShapeCollectorAll : public JPH::CollideShapeCollector
{
public:
  plHybridArray<JPH::CollideShapeResult, 32, plAlignedAllocatorWrapper> m_Results;

  virtual void AddHit(const JPH::CollideShapeResult& result) override
  {
    m_Results.PushBack(result);

    if (m_Results.GetCount() >= 256)
    {
      ForceEarlyOut();
    }
  }
};

bool plJoltWorldModule::OverlapTestSphere(float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const
{
  if (fSphereRadius <= 0.0f)
    return false;

  const JPH::SphereShape shape(fSphereRadius);

  return OverlapTest(shape, JPH::Mat44::sTranslation(plJoltConversionUtils::ToVec3(vPosition)), params);
}

bool plJoltWorldModule::OverlapTestCapsule(float fCapsuleRadius, float fCapsuleHeight, const plTransform& transform, const plPhysicsQueryParameters& params) const
{
  if (fCapsuleRadius <= 0.0f)
    return false;

  const JPH::CapsuleShape shape(fCapsuleHeight * 0.5f, fCapsuleRadius);

  plQuat qFixRot;
  qFixRot.SetFromAxisAndAngle(plVec3(1, 0, 0), plAngle::Degree(90.0f));

  plQuat qRot;
  qRot = transform.m_qRotation;
  qRot = qRot * qFixRot;

  const JPH::Mat44 trans = JPH::Mat44::sRotationTranslation(plJoltConversionUtils::ToQuat(qRot), plJoltConversionUtils::ToVec3(transform.m_vPosition));

  return OverlapTest(shape, trans, params);
}

bool plJoltWorldModule::OverlapTest(const JPH::Shape& shape, const JPH::Mat44& transform, const plPhysicsQueryParameters& params) const
{
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  plJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  plJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);
  plJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);

  plJoltShapeCollectorAny collector;
  query.CollideShape(&shape, JPH::Vec3(1, 1, 1), transform, {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  return collector.m_bFoundAny;
}

void plJoltWorldModule::QueryShapesInSphere(plPhysicsOverlapResultArray& out_results, float fSphereRadius, const plVec3& vPosition, const plPhysicsQueryParameters& params) const
{
  out_results.m_Results.Clear();

  if (fSphereRadius <= 0.0f)
    return;

  const JPH::SphereShape shape(fSphereRadius);
  const JPH::NarrowPhaseQuery& query = m_pSystem->GetNarrowPhaseQuery();

  plJoltBroadPhaseLayerFilter broadphaseFilter(params.m_ShapeTypes);
  plJoltObjectLayerFilter objectFilter(params.m_uiCollisionLayer);
  plJoltBodyFilter bodyFilter(params.m_uiIgnoreObjectFilterID);

  plJoltShapeCollectorAll collector;
  query.CollideShape(&shape, JPH::RVec3(1, 1, 1), JPH::Mat44::sTranslation(plJoltConversionUtils::ToVec3(vPosition)), {}, JPH::RVec3::sZero(), collector, broadphaseFilter, objectFilter, bodyFilter);

  out_results.m_Results.SetCount(collector.m_Results.GetCount());

  auto& lockInterface = m_pSystem->GetBodyLockInterfaceNoLock();

  for (plUInt32 i = 0; i < collector.m_Results.GetCount(); ++i)
  {
    auto& overlapResult = out_results.m_Results[i];
    auto& overlapHit = collector.m_Results[i];

    JPH::BodyLockRead bodyLock(lockInterface, overlapHit.mBodyID2);
    const auto& body = bodyLock.GetBody();

    overlapResult.m_uiObjectFilterID = body.GetCollisionGroup().GetGroupID();
    overlapResult.m_vCenterPosition = plJoltConversionUtils::ToVec3(body.GetCenterOfMassPosition());

    const size_t uiBodyId = body.GetID().GetIndexAndSequenceNumber();
    const size_t uiShapeId = overlapHit.mSubShapeID2.GetValue();
    overlapResult.m_pInternalPhysicsActor = reinterpret_cast<void*>(uiBodyId);
    overlapResult.m_pInternalPhysicsShape = reinterpret_cast<void*>(uiShapeId);

    if (plComponent* pShapeComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetShape()->GetSubShapeUserData(overlapHit.mSubShapeID2))))
    {
      overlapResult.m_hShapeObject = pShapeComponent->GetOwner()->GetHandle();
    }

    if (plComponent* pActorComponent = plJoltUserData::GetComponent(reinterpret_cast<const void*>(body.GetUserData())))
    {
      overlapResult.m_hActorObject = pActorComponent->GetOwner()->GetHandle();
    }
  }
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_System_JoltQueries);
