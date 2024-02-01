#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/BoxReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plBoxReflectionProbeComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plClampValueAttribute(plVec3(0.0f), {}), new plDefaultValueAttribute(plVec3(5.0f))),
    PL_ACCESSOR_PROPERTY("InfluenceScale", GetInfluenceScale, SetInfluenceScale)->AddAttributes(new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f)), new plDefaultValueAttribute(plVec3(1.0f))),
    PL_ACCESSOR_PROPERTY("InfluenceShift", GetInfluenceShift, SetInfluenceShift)->AddAttributes(new plClampValueAttribute(plVec3(-1.0f), plVec3(1.0f)), new plDefaultValueAttribute(plVec3(0.0f))),
    PL_ACCESSOR_PROPERTY("PositiveFalloff", GetPositiveFalloff, SetPositiveFalloff)->AddAttributes(new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f)), new plDefaultValueAttribute(plVec3(0.1f, 0.1f, 0.0f))),
    PL_ACCESSOR_PROPERTY("NegativeFalloff", GetNegativeFalloff, SetNegativeFalloff)->AddAttributes(new plClampValueAttribute(plVec3(0.0f), plVec3(1.0f)), new plDefaultValueAttribute(plVec3(0.1f, 0.1f, 0.0f))),
    PL_ACCESSOR_PROPERTY("BoxProjection", GetBoxProjection, SetBoxProjection)->AddAttributes(new plDefaultValueAttribute(true)),
  }
  PL_END_PROPERTIES;
  PL_BEGIN_FUNCTIONS
  {
    PL_FUNCTION_PROPERTY(OnObjectCreated),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PL_MESSAGE_HANDLER(plMsgExtractRenderData, OnMsgExtractRenderData),
    PL_MESSAGE_HANDLER(plMsgTransformChanged, OnTransformChanged),
  }
  PL_END_MESSAGEHANDLERS;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering/Lighting"),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColorScheme::LightUI(plColorScheme::Blue)),
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
    new plBoxReflectionProbeVisualizerAttribute("Extents", "InfluenceScale", "InfluenceShift"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE

PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plBoxReflectionProbeVisualizerAttribute, 1, plRTTIDefaultAllocator<plBoxReflectionProbeVisualizerAttribute>)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_CONSTRUCTOR_PROPERTY(const char*, const char*, const char*),
  }
  PL_END_FUNCTIONS;
}
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plBoxReflectionProbeComponentManager::plBoxReflectionProbeComponentManager(plWorld* pWorld)
  : plComponentManager<plBoxReflectionProbeComponent, plBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

plBoxReflectionProbeComponent::plBoxReflectionProbeComponent() = default;
plBoxReflectionProbeComponent::~plBoxReflectionProbeComponent() = default;

void plBoxReflectionProbeComponent::SetExtents(const plVec3& vExtents)
{
  m_vExtents = vExtents;
}

const plVec3& plBoxReflectionProbeComponent::GetInfluenceScale() const
{
  return m_vInfluenceScale;
}

void plBoxReflectionProbeComponent::SetInfluenceScale(const plVec3& vInfluenceScale)
{
  m_vInfluenceScale = vInfluenceScale;
}

const plVec3& plBoxReflectionProbeComponent::GetInfluenceShift() const
{
  return m_vInfluenceShift;
}

void plBoxReflectionProbeComponent::SetInfluenceShift(const plVec3& vInfluenceShift)
{
  m_vInfluenceShift = vInfluenceShift;
}

void plBoxReflectionProbeComponent::SetPositiveFalloff(const plVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vPositiveFalloff = vFalloff.CompClamp(plVec3(plMath::DefaultEpsilon<float>()), plVec3(1.0f));
}

void plBoxReflectionProbeComponent::SetNegativeFalloff(const plVec3& vFalloff)
{
  // Does not affect cube generation so m_bStatesDirty is not set.
  m_vNegativeFalloff = vFalloff.CompClamp(plVec3(plMath::DefaultEpsilon<float>()), plVec3(1.0f));
}

void plBoxReflectionProbeComponent::SetBoxProjection(bool bBoxProjection)
{
  m_bBoxProjection = bBoxProjection;
}

const plVec3& plBoxReflectionProbeComponent::GetExtents() const
{
  return m_vExtents;
}

void plBoxReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = plReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void plBoxReflectionProbeComponent::OnDeactivated()
{
  plReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void plBoxReflectionProbeComponent::OnObjectCreated(const plAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void plBoxReflectionProbeComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(plDefaultSpatialDataCategories::RenderDynamic);
}

void plBoxReflectionProbeComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
{
  // Don't trigger reflection rendering in shadow or other reflection views.
  if (msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Shadow || msg.m_pView->GetCameraUsageHint() == plCameraUsageHint::Reflection)
    return;

  if (m_bStatesDirty)
  {
    m_bStatesDirty = false;
    plReflectionPool::UpdateReflectionProbe(GetWorld(), m_Id, m_Desc, this);
  }

  auto pRenderData = plCreateRenderDataForThisFrame<plReflectionProbeRenderData>(GetOwner());
  pRenderData->m_GlobalTransform = GetOwner()->GetGlobalTransform();
  pRenderData->m_vProbePosition = pRenderData->m_GlobalTransform * m_Desc.m_vCaptureOffset;
  pRenderData->m_vHalfExtents = m_vExtents / 2.0f;
  pRenderData->m_vInfluenceScale = m_vInfluenceScale;
  pRenderData->m_vInfluenceShift = m_vInfluenceShift;
  pRenderData->m_vPositiveFalloff = m_vPositiveFalloff;
  pRenderData->m_vNegativeFalloff = m_vNegativeFalloff;
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = 0;
  if (m_bBoxProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const plVec3 vScale = pRenderData->m_GlobalTransform.m_vScale.CompMul(m_vExtents);
  const float fVolume = plMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fVolume, vScale);
  plReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void plBoxReflectionProbeComponent::OnTransformChanged(plMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void plBoxReflectionProbeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
  s << m_vInfluenceScale;
  s << m_vInfluenceShift;
  s << m_vPositiveFalloff;
  s << m_vNegativeFalloff;
  s << m_bBoxProjection;
}

void plBoxReflectionProbeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
  s >> m_vInfluenceScale;
  s >> m_vInfluenceShift;
  s >> m_vPositiveFalloff;
  s >> m_vNegativeFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bBoxProjection;
  }
}

//////////////////////////////////////////////////////////////////////////

plBoxReflectionProbeVisualizerAttribute::plBoxReflectionProbeVisualizerAttribute()
  : plVisualizerAttribute(nullptr)
{
}

plBoxReflectionProbeVisualizerAttribute::plBoxReflectionProbeVisualizerAttribute(const char* szExtentsProperty, const char* szInfluenceScaleProperty, const char* szInfluenceShiftProperty)
  : plVisualizerAttribute(szExtentsProperty, szInfluenceScaleProperty, szInfluenceShiftProperty)
{
}

PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_BoxReflectionProbeComponent);
