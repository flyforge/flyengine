#include <GameEngine/GameEnginePCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <GameEngine/Gameplay/GrabbableItemComponent.h>
#include <RendererCore/Debug/DebugRenderer.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

struct GICFlags
{
  enum Enum
  {
    DebugShowPoints = 0,
  };
};

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plGrabbableItemGrabPoint, plNoBase, 1, plRTTIDefaultAllocator<plGrabbableItemGrabPoint>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("LocalPosition", m_vLocalPosition),
    PL_MEMBER_PROPERTY("LocalRotation", m_qLocalRotation),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_ATTRIBUTES
  {
    new plTransformManipulatorAttribute("LocalPosition", "LocalRotation"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE


PL_BEGIN_COMPONENT_TYPE(plGrabbableItemComponent, 1, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("DebugShowPoints", GetDebugShowPoints, SetDebugShowPoints),
    PL_ARRAY_MEMBER_PROPERTY("GrabPoints", m_GrabPoints),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnExtractRenderData),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Input"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plGrabbableItemComponent::plGrabbableItemComponent() = default;
plGrabbableItemComponent::~plGrabbableItemComponent() = default;

void plGrabbableItemComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  auto& s = inout_stream.GetStream();

  const plUInt8 uiNumGrabPoints = static_cast<plUInt8>(m_GrabPoints.GetCount());
  s << uiNumGrabPoints;
  for (const auto& gb : m_GrabPoints)
  {
    s << gb.m_vLocalPosition;
    s << gb.m_qLocalRotation;
  }
}

void plGrabbableItemComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  auto& s = inout_stream.GetStream();

  plUInt8 uiNumGrabPoints;
  s >> uiNumGrabPoints;
  m_GrabPoints.SetCount(uiNumGrabPoints);
  for (auto& gb : m_GrabPoints)
  {
    s >> gb.m_vLocalPosition;
    s >> gb.m_qLocalRotation;
  }
}

void plGrabbableItemComponent::SetDebugShowPoints(bool bShow)
{
  SetUserFlag(GICFlags::DebugShowPoints, bShow);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

bool plGrabbableItemComponent::GetDebugShowPoints() const
{
  return GetUserFlag(GICFlags::DebugShowPoints);
}

void plGrabbableItemComponent::DebugDrawGrabPoint(const plWorld& world, const plTransform& globalGrabPointTransform)
{
  plDebugRenderer::DrawArrow(&world, 0.75f, plColorScheme::LightUI(plColorScheme::Red), globalGrabPointTransform, plVec3::MakeAxisX());
  plDebugRenderer::DrawArrow(&world, 0.3f, plColorScheme::LightUI(plColorScheme::Green), globalGrabPointTransform, plVec3::MakeAxisY());
  plDebugRenderer::DrawArrow(&world, 0.3f, plColorScheme::LightUI(plColorScheme::Blue), globalGrabPointTransform, plVec3::MakeAxisZ());
}

void plGrabbableItemComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  if (GetDebugShowPoints())
  {
    msg.AddBounds(plBoundingSphere::MakeFromCenterAndRadius(plVec3::MakeZero(), 1.0f), plDefaultSpatialDataCategories::RenderDynamic);
  }
}

void plGrabbableItemComponent::OnExtractRenderData(plMsgExtractRenderData& msg) const
{
  if (!GetDebugShowPoints() || m_GrabPoints.IsEmpty())
    return;

  if (msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::MainView &&
      msg.m_pView->GetCameraUsageHint() != plCameraUsageHint::EditorView)
    return;

  // Don't extract render data for selection.
  if (msg.m_OverrideCategory != plInvalidRenderDataCategory)
    return;

  const plTransform globalTransform = GetOwner()->GetGlobalTransform();

  for (auto& grabPoint : m_GrabPoints)
  {
    plTransform grabPointTransform = plTransform::MakeGlobalTransform(globalTransform, plTransform(grabPoint.m_vLocalPosition, grabPoint.m_qLocalRotation));
    DebugDrawGrabPoint(*GetWorld(), grabPointTransform);
  }
}


PL_STATICLINK_FILE(GameEngine, GameEngine_Gameplay_Implementation_GrabbableItemComponent);
