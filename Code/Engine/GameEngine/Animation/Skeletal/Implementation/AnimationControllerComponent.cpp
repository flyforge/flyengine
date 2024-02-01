#include <GameEngine/GameEnginePCH.h>

#include <Core/Input/InputManager.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Strings/HashedString.h>
#include <GameEngine/Animation/Skeletal/AnimatedMeshComponent.h>
#include <GameEngine/Animation/Skeletal/AnimationControllerComponent.h>
#include <GameEngine/Gameplay/BlackboardComponent.h>
#include <GameEngine/Physics/CharacterControllerComponent.h>
#include <RendererCore/AnimationSystem/AnimGraph/AnimGraphResource.h>
#include <RendererCore/AnimationSystem/SkeletonResource.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAnimationControllerComponent, 2, plComponentMode::Static);
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("AnimGraph", GetAnimGraphFile, SetAnimGraphFile)->AddAttributes(new plAssetBrowserAttribute("CompatibleAsset_Keyframe_Graph")),

    PL_ENUM_MEMBER_PROPERTY("RootMotionMode", plRootMotionMode, m_RootMotionMode),
    PL_ENUM_MEMBER_PROPERTY("InvisibleUpdateRate", plAnimationInvisibleUpdateRate, m_InvisibleUpdateRate),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
      new plCategoryAttribute("Animation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plAnimationControllerComponent::plAnimationControllerComponent() = default;
plAnimationControllerComponent::~plAnimationControllerComponent() = default;

void plAnimationControllerComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  s << m_hAnimGraph;
  s << m_RootMotionMode;
  s << m_InvisibleUpdateRate;
}

void plAnimationControllerComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  s >> m_hAnimGraph;
  s >> m_RootMotionMode;

  if (uiVersion >= 2)
  {
    s >> m_InvisibleUpdateRate;
  }
}

void plAnimationControllerComponent::SetAnimGraphFile(const char* szFile)
{
  plAnimGraphResourceHandle hResource;

  if (!plStringUtils::IsNullOrEmpty(szFile))
  {
    hResource = plResourceManager::LoadResource<plAnimGraphResource>(szFile);
  }

  m_hAnimGraph = hResource;
}


const char* plAnimationControllerComponent::GetAnimGraphFile() const
{
  if (!m_hAnimGraph.IsValid())
    return "";

  return m_hAnimGraph.GetResourceID();
}

void plAnimationControllerComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  if (!m_hAnimGraph.IsValid())
    return;

  plMsgQueryAnimationSkeleton msg;
  GetOwner()->SendMessage(msg);

  if (!msg.m_hSkeleton.IsValid())
    return;

  m_AnimController.Initialize(msg.m_hSkeleton, m_PoseGenerator, plBlackboardComponent::FindBlackboard(GetOwner()));
  m_AnimController.AddAnimGraph(m_hAnimGraph);
}

void plAnimationControllerComponent::Update()
{
  plTime tMinStep = plTime::MakeFromSeconds(0);
  plVisibilityState visType = GetOwner()->GetVisibilityState();

  if (visType != plVisibilityState::Direct)
  {
    if (m_InvisibleUpdateRate == plAnimationInvisibleUpdateRate::Pause && visType == plVisibilityState::Invisible)
      return;

    tMinStep = plAnimationInvisibleUpdateRate::GetTimeStep(m_InvisibleUpdateRate);
  }

  m_ElapsedTimeSinceUpdate += GetWorld()->GetClock().GetTimeDiff();

  if (m_ElapsedTimeSinceUpdate < tMinStep)
    return;

  m_AnimController.Update(m_ElapsedTimeSinceUpdate, GetOwner());
  m_ElapsedTimeSinceUpdate = plTime::MakeZero();

  plVec3 translation;
  plAngle rotationX;
  plAngle rotationY;
  plAngle rotationZ;
  m_AnimController.GetRootMotion(translation, rotationX, rotationY, rotationZ);

  plRootMotionMode::Apply(m_RootMotionMode, GetOwner(), translation, rotationX, rotationY, rotationZ);
}

PL_STATICLINK_FILE(GameEngine, GameEngine_Animation_Skeletal_Implementation_AnimationControllerComponent);
