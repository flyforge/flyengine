#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Actors/JoltDynamicActorComponent.h>
#include <JoltPlugin/Actors/JoltQueryShapeActorComponent.h>
#include <JoltPlugin/Components/JoltHitboxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeBoxComponent.h>
#include <JoltPlugin/Shapes/JoltShapeCapsuleComponent.h>
#include <JoltPlugin/Shapes/JoltShapeSphereComponent.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <RendererCore/AnimationSystem/Declarations.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plJoltHitboxComponent, 2, plComponentMode::Dynamic)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("QueryShapeOnly", m_bQueryShapeOnly)->AddAttributes(new plDefaultValueAttribute(true)),
    PL_MEMBER_PROPERTY("UpdateThreshold", m_UpdateThreshold),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgAnimationPoseUpdated, OnAnimationPoseUpdated),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(GetObjectFilterID),
    PL_SCRIPT_FUNCTION_PROPERTY(RecreatePhysicsShapes),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Physics/Jolt/Animation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltHitboxComponent::plJoltHitboxComponent() = default;
plJoltHitboxComponent::~plJoltHitboxComponent() = default;

void plJoltHitboxComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_bQueryShapeOnly;
  s << m_UpdateThreshold;
}

void plJoltHitboxComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_bQueryShapeOnly;
  s >> m_UpdateThreshold;
}

void plJoltHitboxComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  RecreatePhysicsShapes();
}

void plJoltHitboxComponent::OnDeactivated()
{
  if (m_uiObjectFilterID != plInvalidIndex)
  {
    plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
    pModule->DeleteObjectFilterID(m_uiObjectFilterID);
  }

  DestroyPhysicsShapes();

  SUPER::OnDeactivated();
}

void plJoltHitboxComponent::OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& ref_msg)
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

void plJoltHitboxComponent::RecreatePhysicsShapes()
{
  plMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  DestroyPhysicsShapes();
  CreatePhysicsShapes(msg.m_hSkeleton);

  m_LastUpdate = plTime::MakeZero();
}

void plJoltHitboxComponent::CreatePhysicsShapes(const plSkeletonResourceHandle& hSkeleton)
{
  plResourceLock<plSkeletonResource> pSkeleton(hSkeleton, plResourceAcquireMode::BlockTillLoaded);

  const auto& desc = pSkeleton->GetDescriptor();

  PL_ASSERT_DEV(m_Shapes.IsEmpty(), "");
  m_Shapes.Reserve(desc.m_Geometry.GetCount());

  plJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<plJoltWorldModule>();
  m_uiObjectFilterID = pModule->CreateObjectFilterID();

  const auto srcBoneDir = pSkeleton->GetDescriptor().m_Skeleton.m_BoneDirection;
  const plQuat qBoneDirAdjustment = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveX, srcBoneDir);

  const plQuat qFinalBoneRot = /*boneRot **/ qBoneDirAdjustment;

  plQuat qRotZtoX; // the capsule should extend along X, but the capsule shape goes along Z
  qRotZtoX = plQuat::MakeFromAxisAndAngle(plVec3(0, 1, 0), plAngle::MakeFromDegree(-90));

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


    if (geo.m_Type == plSkeletonJointGeometryType::Sphere)
    {
      plJoltShapeSphereComponent* pShapeComp = nullptr;
      plJoltShapeSphereComponent::CreateComponent(pGO, pShapeComp);
      pShapeComp->SetRadius(geo.m_Transform.m_vScale.z);
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
    }
    else
    {
      PL_ASSERT_NOT_IMPLEMENTED;
    }
  }
}

void plJoltHitboxComponent::DestroyPhysicsShapes()
{
  for (auto& shape : m_Shapes)
  {
    GetWorld()->DeleteObjectDelayed(shape.m_hActorObject);
  }

  m_Shapes.Clear();
}

PL_STATICLINK_FILE(JoltPlugin, JoltPlugin_Components_Implementation_JoltBoneColliderComponent);
