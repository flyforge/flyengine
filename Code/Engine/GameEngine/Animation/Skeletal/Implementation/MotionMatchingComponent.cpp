#include <GameEngine/GameEnginePCH.h>

//#include <Core/Input/InputManager.h>
//#include <Core/WorldSerializer/WorldReader.h>
//#include <Core/WorldSerializer/WorldWriter.h>
//#include <GameEngine/Animation/Skeletal/MotionMatchingComponent.h>
//#include <RendererCore/AnimationSystem/AnimationClipResource.h>
//#include <RendererCore/AnimationSystem/SkeletonResource.h>
//#include <RendererCore/Debug/DebugRenderer.h>
//#include <RendererFoundation/Device/Device.h>
//
//// clang-format off
//PL_BEGIN_COMPONENT_TYPE(plMotionMatchingComponent, 2, plComponentMode::Dynamic);
//{
//  PL_BEGIN_PROPERTIES
//  {
//    PL_ARRAY_ACCESSOR_PROPERTY("Animations", Animations_GetCount, Animations_GetValue, Animations_SetValue, Animations_Insert, Animations_Remove)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Keyframe_Animation")),
//  }
//  PL_END_PROPERTIES;
//
//  PL_BEGIN_ATTRIBUTES
//  {
//      new plCategoryAttribute("Animation"),
//  }
//  PL_END_ATTRIBUTES;
//}
//PL_END_COMPONENT_TYPE
//// clang-format on
//
//plMotionMatchingComponent::plMotionMatchingComponent() = default;
//plMotionMatchingComponent::~plMotionMatchingComponent() = default;
//
//void plMotionMatchingComponent::SerializeComponent(plWorldWriter& stream) const
//{
//  SUPER::SerializeComponent(stream);
//  auto& s = stream.GetStream();
//
//  s.WriteArray(m_Animations);
//}
//
//void plMotionMatchingComponent::DeserializeComponent(plWorldReader& stream)
//{
//  SUPER::DeserializeComponent(stream);
//  const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
//  auto& s = stream.GetStream();
//
//  if (uiVersion >= 2)
//  {
//    s.ReadArray(m_Animations);
//  }
//}
//
//void plMotionMatchingComponent::OnSimulationStarted()
//{
//  SUPER::OnSimulationStarted();
//
//  // make sure the skinning buffer is deleted
//  PL_ASSERT_DEBUG(m_hSkinningTransformsBuffer.IsInvalidated(), "The skinning buffer should not exist at this time");
//
//  if (m_hMesh.IsValid())
//  {
//    plResourceLock<plMeshResource> pMesh(m_hMesh, plResourceAcquireMode::BlockTillLoaded);
//    m_hSkeleton = pMesh->GetSkeleton();
//  }
//
//  if (m_hSkeleton.IsValid())
//  {
//    plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::BlockTillLoaded);
//
//    const plSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//    m_AnimationPose.Configure(skeleton);
//    m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//    m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//    // m_SkinningMatrices = m_AnimationPose.GetAllTransforms();
//
//    // Create the buffer for the skinning matrices
//    plGALBufferCreationDescription BufferDesc;
//    BufferDesc.m_uiStructSize = sizeof(plMat4);
//    BufferDesc.m_uiTotalSize = BufferDesc.m_uiStructSize * m_AnimationPose.GetTransformCount();
//    BufferDesc.m_bUseAsStructuredBuffer = true;
//    BufferDesc.m_bAllowShaderResourceView = true;
//    BufferDesc.m_ResourceAccess.m_bImmutable = false;
//
//    m_hSkinningTransformsBuffer = plGALDevice::GetDefaultDevice()->CreateBuffer(
//      BufferDesc, plArrayPtr<const plUInt8>(reinterpret_cast<const plUInt8*>(m_AnimationPose.GetAllTransforms().GetPtr()), BufferDesc.m_uiTotalSize));
//  }
//
//  // m_AnimationClipSampler.RestartAnimation();
//
//  if (m_Animations.IsEmpty())
//    return;
//
//  m_Keyframe0.m_uiAnimClip = 0;
//  m_Keyframe0.m_uiKeyframe = 0;
//  m_Keyframe1.m_uiAnimClip = 0;
//  m_Keyframe1.m_uiKeyframe = 1;
//
//  for (plUInt32 anim = 0; anim < m_Animations.GetCount(); ++anim)
//  {
//    plResourceLock<plAnimationClipResource> pClip(m_Animations[anim], plResourceAcquireMode::BlockTillLoaded);
//    plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::AllowLoadingFallback);
//
//    PrecomputeMotion(m_MotionData, "Bip01_L_Foot", "Bip01_R_Foot", pClip->GetDescriptor(), anim, pSkeleton->GetDescriptor().m_Skeleton);
//  }
//
//  m_vLeftFootPos.SetZero();
//  m_vRightFootPos.SetZero();
//
//  ConfigureInput();
//}
//void plMotionMatchingComponent::ConfigureInput()
//{
//  plInputActionConfig iac;
//  iac.m_bApplyTimeScaling = false;
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_LeftStick_PosY;
//  iac.m_sInputSlotTrigger[1] = plInputSlot_KeyUp;
//  plInputManager::SetInputActionConfig("mm", "forward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_LeftStick_NegY;
//  iac.m_sInputSlotTrigger[1] = plInputSlot_KeyDown;
//  plInputManager::SetInputActionConfig("mm", "backward", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_LeftStick_NegX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  plInputManager::SetInputActionConfig("mm", "left", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_LeftStick_PosX;
//  iac.m_sInputSlotTrigger[1].Clear();
//  plInputManager::SetInputActionConfig("mm", "right", iac, true);
//
//  iac.m_bApplyTimeScaling = true;
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_RightStick_PosX;
//  iac.m_sInputSlotTrigger[1] = plInputSlot_KeyRight;
//  // iac.m_sInputSlotTrigger[1] = plInputSlot_KeyRight;
//  plInputManager::SetInputActionConfig("mm", "turnright", iac, true);
//
//  iac.m_sInputSlotTrigger[0] = plInputSlot_Controller0_RightStick_NegX;
//  iac.m_sInputSlotTrigger[1] = plInputSlot_KeyLeft;
//  // iac.m_sInputSlotTrigger[1] = plInputSlot_KeyRight;
//  plInputManager::SetInputActionConfig("mm", "turnleft", iac, true);
//}
//
//plVec3 plMotionMatchingComponent::GetInputDirection() const
//{
//  float fw, bw, l, r;
//
//  plInputManager::GetInputActionState("mm", "forward", &fw);
//  plInputManager::GetInputActionState("mm", "backward", &bw);
//  plInputManager::GetInputActionState("mm", "left", &l);
//  plInputManager::GetInputActionState("mm", "right", &r);
//
//  plVec3 dir;
//  dir.y = -(fw - bw);
//  dir.x = r - l;
//  dir.z = 0;
//
//  // dir.NormalizeIfNotZero(plVec3::MakeZero());
//  return dir * 3.0f;
//}
//
//plQuat plMotionMatchingComponent::GetInputRotation() const
//{
//  float tl, tr;
//
//  plInputManager::GetInputActionState("mm", "turnleft", &tl);
//  plInputManager::GetInputActionState("mm", "turnright", &tr);
//
//  const plAngle turn = plAngle::MakeFromDegree((tr - tl) * 90.0f);
//
//  plQuat q;
//  q.SetFromAxisAndAngle(plVec3(0, 0, 1), turn);
//  return q;
//}
//
//void plMotionMatchingComponent::Update()
//{
//  if (!m_hSkeleton.IsValid() || m_Animations.IsEmpty())
//    return;
//
//  plResourceLock<plSkeletonResource> pSkeleton(m_hSkeleton, plResourceAcquireMode::AllowLoadingFallback);
//  const plSkeleton& skeleton = pSkeleton->GetDescriptor().m_Skeleton;
//
//  // plTransform rootMotion;
//  // rootMotion.SetIdentity();
//
//  const float fKeyframeFraction = (float)GetWorld()->GetClock().GetTimeDiff().GetSeconds() * 24.0f; // assuming 24 FPS in the animations
//
//  {
//    const plVec3 vTargetDir = GetInputDirection() / GetOwner()->GetGlobalScaling().x;
//
//    plStringBuilder tmp;
//    tmp.SetFormat("Gamepad: {0} / {1}", plArgF(vTargetDir.x, 1), plArgF(vTargetDir.y, 1));
//    plDebugRenderer::DrawInfoText(GetWorld(), tmp, plVec2I32(10, 10), plColor::White);
//
//    m_fKeyframeLerp += fKeyframeFraction;
//    while (m_fKeyframeLerp > 1.0f)
//    {
//
//      m_Keyframe0 = m_Keyframe1;
//      m_Keyframe1 = FindNextKeyframe(m_Keyframe1, vTargetDir);
//
//      // plLog::Info("Old KF: {0} | {1} - {2}", m_Keyframe0.m_uiAnimClip, m_Keyframe0.m_uiKeyframe, m_fKeyframeLerp);
//      m_fKeyframeLerp -= 1.0f;
//      // plLog::Info("New KF: {0} | {1} - {2}", m_Keyframe1.m_uiAnimClip, m_Keyframe1.m_uiKeyframe, m_fKeyframeLerp);
//    }
//  }
//
//  m_AnimationPose.SetToBindPoseInLocalSpace(skeleton);
//
//  {
//    plResourceLock<plAnimationClipResource> pAnimClip0(m_Animations[m_Keyframe0.m_uiAnimClip], plResourceAcquireMode::BlockTillLoaded);
//    plResourceLock<plAnimationClipResource> pAnimClip1(m_Animations[m_Keyframe1.m_uiAnimClip], plResourceAcquireMode::BlockTillLoaded);
//
//    const auto& animDesc0 = pAnimClip0->GetDescriptor();
//    const auto& animDesc1 = pAnimClip1->GetDescriptor();
//
//    const auto& animatedJoints0 = animDesc0.GetAllJointIndices();
//
//    for (plUInt32 b = 0; b < animatedJoints0.GetCount(); ++b)
//    {
//      const plHashedString sJointName = animatedJoints0.GetKey(b);
//      const plUInt32 uiAnimJointIdx0 = animatedJoints0.GetValue(b);
//      const plUInt32 uiAnimJointIdx1 = animDesc1.FindJointIndexByName(sJointName);
//
//      const plUInt16 uiSkeletonJointIdx = skeleton.FindJointByName(sJointName);
//      if (uiSkeletonJointIdx != plInvalidJointIndex)
//      {
//        plArrayPtr<const plTransform> pTransforms0 = animDesc0.GetJointKeyframes(uiAnimJointIdx0);
//        plArrayPtr<const plTransform> pTransforms1 = animDesc1.GetJointKeyframes(uiAnimJointIdx1);
//
//        const plTransform jointTransform1 = pTransforms0[m_Keyframe0.m_uiKeyframe];
//        const plTransform jointTransform2 = pTransforms1[m_Keyframe1.m_uiKeyframe];
//
//        plTransform res;
//        res.m_vPosition = plMath::Lerp(jointTransform1.m_vPosition, jointTransform2.m_vPosition, m_fKeyframeLerp);
//        res.m_qRotation.SetSlerp(jointTransform1.m_qRotation, jointTransform2.m_qRotation, m_fKeyframeLerp);
//        res.m_vScale = plMath::Lerp(jointTransform1.m_vScale, jointTransform2.m_vScale, m_fKeyframeLerp);
//
//        m_AnimationPose.SetTransform(uiSkeletonJointIdx, res.GetAsMat4());
//      }
//    }
//
//    // root motion
//    {
//      auto* pOwner = GetOwner();
//
//      plVec3 vRootMotion0, vRootMotion1;
//      vRootMotion0.SetZero();
//      vRootMotion1.SetZero();
//
//      if (animDesc0.HasRootMotion())
//        vRootMotion0 = animDesc0.GetJointKeyframes(animDesc0.GetRootMotionJoint())[m_Keyframe0.m_uiKeyframe].m_vPosition;
//      if (animDesc1.HasRootMotion())
//        vRootMotion1 = animDesc1.GetJointKeyframes(animDesc1.GetRootMotionJoint())[m_Keyframe1.m_uiKeyframe].m_vPosition;
//
//      const plVec3 vRootMotion = plMath::Lerp(vRootMotion0, vRootMotion1, m_fKeyframeLerp) * fKeyframeFraction * pOwner->GetGlobalScaling().x;
//
//      const plQuat qRotate = GetInputRotation();
//
//      const plQuat qOldRot = pOwner->GetLocalRotation();
//      const plVec3 vNewPos = qOldRot * vRootMotion + pOwner->GetLocalPosition();
//      const plQuat qNewRot = qRotate * qOldRot;
//
//      pOwner->SetLocalPosition(vNewPos);
//      pOwner->SetLocalRotation(qNewRot);
//    }
//  }
//
//  m_AnimationPose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//  const plUInt16 uiLeftFootJoint = skeleton.FindJointByName("Bip01_L_Foot");
//  const plUInt16 uiRightFootJoint = skeleton.FindJointByName("Bip01_R_Foot");
//  if (uiLeftFootJoint != plInvalidJointIndex && uiRightFootJoint != plInvalidJointIndex)
//  {
//    plTransform tLeft, tRight;
//    plBoundingSphere sphere(plVec3::MakeZero(), 0.5f);
//
//    tLeft.SetFromMat4(m_AnimationPose.GetTransform(uiLeftFootJoint));
//    tRight.SetFromMat4(m_AnimationPose.GetTransform(uiRightFootJoint));
//
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiLeftFootJoint);
//    m_AnimationPose.VisualizePose(GetWorld(), skeleton, GetOwner()->GetGlobalTransform(), 1.0f / 6.0f, uiRightFootJoint);
//
//    // const float fScaleToPerSec = (float)(1.0 / GetWorld()->GetClock().GetTimeDiff().GetSeconds());
//
//    // const plVec3 vLeftFootVel = (tLeft.m_vPosition - m_vLeftFootPos) * fScaleToPerSec;
//    // const plVec3 vRightFootVel = (tRight.m_vPosition - m_vRightFootPos) * fScaleToPerSec;
//
//    m_vLeftFootPos = tLeft.m_vPosition;
//    m_vRightFootPos = tRight.m_vPosition;
//  }
//
//  m_AnimationPose.ConvertFromObjectSpaceToSkinningSpace(skeleton);
//
//  plArrayPtr<plMat4> pRenderMatrices = PL_NEW_ARRAY(plFrameAllocator::GetCurrentAllocator(), plMat4, m_AnimationPose.GetTransformCount());
//  plMemoryUtils::Copy(pRenderMatrices.GetPtr(), m_AnimationPose.GetAllTransforms().GetPtr(), m_AnimationPose.GetTransformCount());
//
//  m_SkinningMatrices = pRenderMatrices;
//}
//
//void plMotionMatchingComponent::SetAnimation(plUInt32 uiIndex, const plAnimationClipResourceHandle& hResource)
//{
//  m_Animations.EnsureCount(uiIndex + 1);
//
//  m_Animations[uiIndex] = hResource;
//}
//
//plAnimationClipResourceHandle plMotionMatchingComponent::GetAnimation(plUInt32 uiIndex) const
//{
//  if (uiIndex >= m_Animations.GetCount())
//    return plAnimationClipResourceHandle();
//
//  return m_Animations[uiIndex];
//}
//
//plUInt32 plMotionMatchingComponent::Animations_GetCount() const
//{
//  return m_Animations.GetCount();
//}
//
//const char* plMotionMatchingComponent::Animations_GetValue(plUInt32 uiIndex) const
//{
//  const auto& hMat = GetAnimation(uiIndex);
//
//  if (!hMat.IsValid())
//    return "";
//
//  return hMat.GetResourceID();
//}
//
//void plMotionMatchingComponent::Animations_SetValue(plUInt32 uiIndex, const char* value)
//{
//  if (plStringUtils::IsNullOrEmpty(value))
//    SetAnimation(uiIndex, plAnimationClipResourceHandle());
//  else
//  {
//    auto hMat = plResourceManager::LoadResource<plAnimationClipResource>(value);
//    SetAnimation(uiIndex, hMat);
//  }
//}
//
//void plMotionMatchingComponent::Animations_Insert(plUInt32 uiIndex, const char* value)
//{
//  plAnimationClipResourceHandle hMat;
//
//  if (!plStringUtils::IsNullOrEmpty(value))
//    hMat = plResourceManager::LoadResource<plAnimationClipResource>(value);
//
//  m_Animations.Insert(hMat, uiIndex);
//}
//
//void plMotionMatchingComponent::Animations_Remove(plUInt32 uiIndex)
//{
//  m_Animations.RemoveAtAndCopy(uiIndex);
//}
//
//plMotionMatchingComponent::TargetKeyframe plMotionMatchingComponent::FindNextKeyframe(const TargetKeyframe& current, const plVec3& vTargetDir) const
//{
//  TargetKeyframe kf;
//  kf.m_uiAnimClip = current.m_uiAnimClip;
//  kf.m_uiKeyframe = current.m_uiKeyframe + 1;
//
//  {
//    // plResourceLock<plAnimationClipResource> pAnimClipCur(m_Animations[current.m_uiAnimClip], plResourceAcquireMode::NoFallback);
//    // const auto& animClip = pAnimClipCur->GetDescriptor();
//
//    // const plUInt32 uiLeftFootJoint = animClip.FindJointIndexByName("Bip01_L_Foot");
//    // const plUInt32 uiRightFootJoint = animClip.FindJointIndexByName("Bip01_R_Foot");
//
//    const plVec3 vLeftFootPos = m_vLeftFootPos;   // animClip.GetJointKeyframes(uiLeftFootJoint)[current.m_uiKeyframe].m_vPosition;
//    const plVec3 vRightFootPos = m_vRightFootPos; // animClip.GetJointKeyframes(uiRightFootJoint)[current.m_uiKeyframe].m_vPosition;
//
//    const plUInt32 uiBestMM = FindBestKeyframe(current, vLeftFootPos, vRightFootPos, vTargetDir);
//
//    TargetKeyframe nkf;
//    nkf.m_uiAnimClip = m_MotionData[uiBestMM].m_uiAnimClipIndex;
//    nkf.m_uiKeyframe = m_MotionData[uiBestMM].m_uiKeyframeIndex;
//
//    if ((nkf.m_uiAnimClip != kf.m_uiAnimClip) || (nkf.m_uiKeyframe != kf.m_uiKeyframe && nkf.m_uiKeyframe != current.m_uiKeyframe))
//    {
//      kf = nkf;
//    }
//  }
//
//  plResourceLock<plAnimationClipResource> pAnimClip(m_Animations[kf.m_uiAnimClip], plResourceAcquireMode::BlockTillLoaded);
//
//  if (kf.m_uiKeyframe >= pAnimClip->GetDescriptor().GetNumFrames())
//  {
//    // loop
//    kf.m_uiKeyframe = 0;
//  }
//
//  return kf;
//}
//
//void plMotionMatchingComponent::PrecomputeMotion(plDynamicArray<MotionData>& motionData, plTempHashedString jointName1, plTempHashedString jointName2,
//  const plAnimationClipResourceDescriptor& animClip, plUInt16 uiAnimClipIndex, const plSkeleton& skeleton)
//{
//  const plUInt16 uiRootJoint = animClip.HasRootMotion() ? animClip.GetRootMotionJoint() : 0xFFFFu;
//  // const plUInt16 uiJoint1IndexInAnim = animClip.FindJointIndexByName(jointName1);
//  // const plUInt16 uiJoint2IndexInAnim = animClip.FindJointIndexByName(jointName2);
//
//  const plUInt16 uiJoint1IndexInSkeleton = skeleton.FindJointByName(jointName1);
//  const plUInt16 uiJoint2IndexInSkeleton = skeleton.FindJointByName(jointName2);
//  if (uiJoint1IndexInSkeleton == plInvalidJointIndex || uiJoint2IndexInSkeleton == plInvalidJointIndex)
//    return;
//
//  const auto& jointNamesToIndices = animClip.GetAllJointIndices();
//
//  const plUInt32 uiFirstMotionDataIdx = motionData.GetCount();
//  motionData.Reserve(uiFirstMotionDataIdx + animClip.GetNumFrames());
//
//  const float fRootMotionToVelocity = animClip.GetFramesPerSecond();
//
//  plAnimationPose pose;
//  pose.Configure(skeleton);
//
//  for (plUInt16 uiFrameIdx = 0; uiFrameIdx < animClip.GetNumFrames(); ++uiFrameIdx)
//  {
//    pose.SetToBindPoseInLocalSpace(skeleton);
//
//    for (plUInt32 b = 0; b < jointNamesToIndices.GetCount(); ++b)
//    {
//      const plUInt16 uiJointIndexInPose = skeleton.FindJointByName(jointNamesToIndices.GetKey(b));
//      if (uiJointIndexInPose != plInvalidJointIndex)
//      {
//        const plTransform jointTransform = animClip.GetJointKeyframes(jointNamesToIndices.GetValue(b))[uiFrameIdx];
//
//        pose.SetTransform(uiJointIndexInPose, jointTransform.GetAsMat4());
//      }
//    }
//
//    pose.ConvertFromLocalSpaceToObjectSpace(skeleton);
//
//    MotionData& md = motionData.ExpandAndGetRef();
//    md.m_vLeftFootPosition = pose.GetTransform(uiJoint1IndexInSkeleton).GetTranslationVector();
//    md.m_vRightFootPosition = pose.GetTransform(uiJoint2IndexInSkeleton).GetTranslationVector();
//    md.m_uiAnimClipIndex = uiAnimClipIndex;
//    md.m_uiKeyframeIndex = uiFrameIdx;
//    md.m_vLeftFootVelocity.SetZero();
//    md.m_vRightFootVelocity.SetZero();
//    md.m_vRootVelocity =
//      animClip.HasRootMotion() ? fRootMotionToVelocity * animClip.GetJointKeyframes(uiRootJoint)[uiFrameIdx].m_vPosition : plVec3::MakeZero();
//  }
//
//  // now compute the velocity
//  {
//    const float fScaleToVelPerSec = animClip.GetFramesPerSecond();
//
//    plUInt32 uiPrevMdIdx = motionData.GetCount() - 1;
//
//    for (plUInt32 uiMotionDataIdx = uiFirstMotionDataIdx; uiMotionDataIdx < motionData.GetCount(); ++uiMotionDataIdx)
//    {
//      {
//        plVec3 vel = motionData[uiMotionDataIdx].m_vLeftFootPosition - motionData[uiPrevMdIdx].m_vLeftFootPosition;
//        motionData[uiMotionDataIdx].m_vLeftFootVelocity = vel * fScaleToVelPerSec;
//      }
//      {
//        plVec3 vel = motionData[uiMotionDataIdx].m_vRightFootPosition - motionData[uiPrevMdIdx].m_vRightFootPosition;
//        motionData[uiMotionDataIdx].m_vRightFootVelocity = vel * fScaleToVelPerSec;
//      }
//
//      uiPrevMdIdx = uiMotionDataIdx;
//    }
//  }
//}
//
//plUInt32 plMotionMatchingComponent::FindBestKeyframe(
//  const TargetKeyframe& current, plVec3 vLeftFootPosition, plVec3 vRightFootPosition, plVec3 vTargetDir) const
//{
//  float fClosest = 1000000000.0f;
//  plUInt32 uiClosest = 0xFFFFFFFFu;
//
//  const float fDirWeight = 3.0f;
//
//  for (plUInt32 i = 0; i < m_MotionData.GetCount(); ++i)
//  {
//    const auto& md = m_MotionData[i];
//
//    float penaltyMul = 1.1f;
//    float penaltyAdd = 100;
//
//    if (md.m_uiAnimClipIndex == current.m_uiAnimClip)
//    {
//      // do NOT allow to transition backwards to a keyframe within a certain range
//      if (md.m_uiKeyframeIndex < current.m_uiKeyframe && md.m_uiKeyframeIndex + 10 > current.m_uiKeyframe)
//        continue;
//
//      penaltyMul = 1.0f;
//
//      if (md.m_uiKeyframeIndex == current.m_uiKeyframe)
//      {
//        penaltyAdd = 0;
//        penaltyMul = 0.9f;
//      }
//    }
//
//    const float dirDist = plMath::Pow((md.m_vRootVelocity - vTargetDir).GetLength(), fDirWeight);
//    const float leftFootDist = (md.m_vLeftFootPosition - vLeftFootPosition).GetLengthSquared();
//    const float rightFootDist = (md.m_vRightFootPosition - vRightFootPosition).GetLengthSquared();
//
//    const float fScore = dirDist + (leftFootDist + rightFootDist) * penaltyMul + penaltyAdd;
//
//    if (fScore < fClosest)
//    {
//      fClosest = fScore;
//      uiClosest = i;
//    }
//  }
//
//  return uiClosest;
//}

PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_MotionMatchingComponent);
