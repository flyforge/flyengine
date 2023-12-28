#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Components/JoltBoneColliderComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltBoneColliderComponent, 1, plComponentMode::Dynamic)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("QueryShapeOnly", m_bQueryShapeOnly)->AddAttributes(new plDefaultValueAttribute(true)),
    PLASMA_MEMBER_PROPERTY("UpdateThreshold", m_UpdateThreshold),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Animation"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltBoneColliderComponent::plJoltBoneColliderComponent() = default;
plJoltBoneColliderComponent::~plJoltBoneColliderComponent() = default;

void plJoltBoneColliderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void plJoltBoneColliderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void plJoltBoneColliderComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void plJoltBoneColliderComponent::OnDeactivated()
{
  if (m_uiObjectFilterID != plInvalidIndex)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void plJoltBoneColliderComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_msg)
{
  if (m_UpdateThreshold.IsPositive())
  {
    const plTime tNow = GetWorld()->GetClock().GetAccumulatedTime();

    if (tNow - m_LastUpdate < m_UpdateThreshold)
      return;

    m_LastUpdate = tNow;
  }

  for (const auto& shape : m_Shapes)
  {
    plMat4 boneTrans;
    plQuat boneRot;
    ref_msg.ComputeFullBoneTransform(shape.m_uiAttachedToBone, boneTrans, boneRot);

    plTransform pose;
    pose.SetIdentity();
    pose.m_vPosition = boneTrans.GetTranslationVector() + boneRot * shape.m_vOffsetPos;
    pose.m_qRotation = boneRot * shape.m_qOffsetRot;

    plGameObject* pGO = nullptr;
    if (GetWorld()->TryGetObject(shape.m_hActorObject, pGO))
    {
      pGO->SetLocalPosition(pose.m_vPosition);
      pGO->SetLocalRotation(pose.m_qRotation);
    }
  }
}

void plJoltBoneColliderComponent::RecreatePhysicsShapes()
{
  plMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate.SetZero();
}

void plJoltBoneColliderComponent::CreatePhysicsShapes(const plSkeletonResourceHandle& hSkeleton)
{
  plResourceLock<plSkeletonResource> pSkeleton(hSkeleton, plResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pSkeleton->GetDescriptor();

  PLASMA_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  m_Shapes.Reserve(desc.m_Geometry.GetCount());

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const plQuat qBoneDirAdjustment = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveX, srcBoneDir);

  const plQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  plQuat qRotZtoX; // the capsule should extend along X, but the capsule shape goes along Z
  qRotZtoX.SetFromAxisAndAngle(plVec3(0, 1, 0), plAngle::Degree(-90));

  for (plUInt32 idx = 0; idx < desc.m_Geometry.GetCount(); ++idx)
  {
    const auto& geo = desc.m_Geometry[idx];

    if (geo.m_Type == plSkeletonJointGeometryType::None)
      continue;

    const plSkeletonJoint& joint = desc.m_Skeleton.GetJointByIndex(geo.m_uiAttachedToJoint);

    auto& shape = m_Shapes.ExpandAndGetRef();

    plGameObject* pGO = nullptr;

    {
      plGameObjectDesc god;
      god.m_bDynamic = true;
      god.m_hParent = GetOwner()->GetHandle();
      god.m_sName = joint.GetName();
      god.m_uiTeamID = GetOwner()->GetTeamID();

      shape.m_hActorObject = GetWorld()->CreateObject(god, pGO);

      if (m_bQueryShapeOnly)
      {
        plJoltQueryShapeActorComponent* pDynAct = nullptr;
        plJoltQueryShapeActorComponent::CreateComponent(pGO, pDynAct);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
      else
      {
        plJoltDynamicActorComponent* pDynAct = nullptr;
        plJoltDynamicActorComponent::CreateComponent(pGO, pDynAct);
        pDynAct->SetKinematic(true);

        pDynAct->m_uiCollisionLayer = joint.GetCollisionLayer();
        pDynAct->m_hSurface = joint.GetSurface();
        pDynAct->SetInitialObjectFilterID(m_uiObjectFilterID);
      }
    }

    shape.m_uiAttachedToBone = geo.m_uiAttachedToJoint;
    shape.m_vOffsetPos = /*boneTrans.GetTranslationVector() +*/ qFinalBoneRot * geo.m_Transform.m_vPosition;
    shape.m_qOffsetRot = qFinalBoneRot * geo.m_Transform.m_qRotation;


    plJoltShapeComponent* pShape = nullptr;

    if (geo.m_Type == plSkeletonJointGeometryType::Sphere)
    {
      plJoltShapeSphereComponent* pShapeComp = nullptr;
      plJoltShapeSphereComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShape = pShapeComp;
    }
    else if (geo.m_Type == plSkeletonJointGeometryType::Box)
    {
      plVec3 ext;
      ext.x = geo.m_Transform.m_vScale.x;
      ext.y = geo.m_Transform.m_vScale.y;
      ext.z = geo.m_Transform.m_vScale.z;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * plVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      plJoltShapeBoxComponent* pShapeComp = nullptr;
      plJoltShapeBoxComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetHalfExtents(ext * 0.5f);
      pShape = pShapeComp;
    }
    else if (geo.m_Type == plSkeletonJointGeometryType::Capsule)
    {
      shape.m_qOffsetRot = shape.m_qOffsetRot * qRotZtoX;

      // TODO: if offset desired
      shape.m_vOffsetPos += qFinalBoneRot * plVec3(geo.m_Transform.m_vScale.x * 0.5f, 0, 0);

      plJoltShapeCapsuleComponent* pShapeComp = nullptr;
      plJoltShapeCapsuleComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
      pShapeComp->SetHeight(geo.m_Transform.m_vScale.x);
      pShape = pShapeComp;
    }
    else
    {
      PLASMA_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void plJoltBoneColliderComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltBoneColliderComponent);

