#include <GameEngine/GameEnginePCH.h>

#include <Core/Graphics/Geometry.h>
#include <Foundation/Configuration/Singleton.h>
#include <GameEngine/XR/VisualizeHandComponent.h>
#include <GameEngine/XR/XRHandTrackingInterface.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Debug/DebugRendererContext.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plVisualizeHandComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("XR"),
    new plColorAttribute(plColorScheme::XR),
    new plInDevelopmentAttribute(plInDevelopmentAttribute::Phase::Beta),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE;
// clang-format on

plVisualizeHandComponent::plVisualizeHandComponent() = default;
plVisualizeHandComponent::~plVisualizeHandComponent() = default;

void plVisualizeHandComponent::Update()
{
  plXRHandTrackingInterface* pXRHand = plSingletonRegistry::GetSingletonInstance<plXRHandTrackingInterface>();

  if (!pXRHand)
    return;

  plHybridArray<plXRHandBone, 6> bones;
  for (plXRHand::Enum hand : {plXRHand::Left, plXRHand::Right})
  {
    for (plUInt32 uiPart = 0; uiPart < plXRHandPart::COUNT; ++uiPart)
    {
      plXRHandPart::Enum part = static_cast<plXRHandPart::Enum>(uiPart);
      if (pXRHand->TryGetBoneTransforms(hand, part, plXRTransformSpace::Global, bones) == plXRHandTrackingInterface::HandPartTrackingState::Tracked)
      {
        plHybridArray<plDebugRenderer::Line, 6> m_Lines;
        for (plUInt32 uiBone = 0; uiBone < bones.GetCount(); uiBone++)
        {
          const plXRHandBone& bone = bones[uiBone];
          plBoundingSphere sphere(plVec3::ZeroVector(), bone.m_fRadius);
          plDebugRenderer::DrawLineSphere(GetWorld(), sphere, plColor::Aquamarine, bone.m_Transform);

          if (uiBone + 1 < bones.GetCount())
          {
            const plXRHandBone& nextBone = bones[uiBone + 1];
            m_Lines.PushBack(plDebugRenderer::Line(bone.m_Transform.m_vPosition, nextBone.m_Transform.m_vPosition));
          }
        }
        plDebugRenderer::DrawLines(GetWorld(), m_Lines, plColor::IndianRed);
      }
    }
  }
}

PLASMA_STATICLINK_FILE(GameEngine, GameEngine_XR_Implementation_VisualizeHandComponent);
