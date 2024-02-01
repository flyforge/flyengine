#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/SphereReflectionProbeComponent.h>

#include <../../Data/Base/Shaders/Common/LightData.h>
#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <RendererCore/Lights/Implementation/ReflectionPool.h>
#include <RendererCore/Pipeline/View.h>

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plSphereReflectionProbeComponent, 2, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plClampValueAttribute(0.0f, {}), new plDefaultValueAttribute(5.0f)),
    PL_ACCESSOR_PROPERTY("Falloff", GetFalloff, SetFalloff)->AddAttributes(new plClampValueAttribute(0.0f, 1.0f), new plDefaultValueAttribute(0.1f)),
    PL_ACCESSOR_PROPERTY("SphereProjection", GetSphereProjection, SetSphereProjection)->AddAttributes(new plDefaultValueAttribute(true)),
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
    new plSphereVisualizerAttribute("Radius", plColorScheme::LightUI(plColorScheme::Blue)),
    new plSphereManipulatorAttribute("Radius"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_COMPONENT_TYPE
// clang-format on

plSphereReflectionProbeComponentManager::plSphereReflectionProbeComponentManager(plWorld* pWorld)
  : plComponentManager<plSphereReflectionProbeComponent, plBlockStorageType::Compact>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

plSphereReflectionProbeComponent::plSphereReflectionProbeComponent() = default;
plSphereReflectionProbeComponent::~plSphereReflectionProbeComponent() = default;

void plSphereReflectionProbeComponent::SetRadius(float fRadius)
{
  m_fRadius = plMath::Max(fRadius, 0.0f);
  m_bStatesDirty = true;
}

float plSphereReflectionProbeComponent::GetRadius() const
{
  return m_fRadius;
}

void plSphereReflectionProbeComponent::SetFalloff(float fFalloff)
{
  m_fFalloff = plMath::Clamp(fFalloff, plMath::DefaultEpsilon<float>(), 1.0f);
}

void plSphereReflectionProbeComponent::SetSphereProjection(bool bSphereProjection)
{
  m_bSphereProjection = bSphereProjection;
}

void plSphereReflectionProbeComponent::OnActivated()
{
  GetOwner()->EnableStaticTransformChangesNotifications();
  m_Id = plReflectionPool::RegisterReflectionProbe(GetWorld(), m_Desc, this);
  GetOwner()->UpdateLocalBounds();
}

void plSphereReflectionProbeComponent::OnDeactivated()
{
  plReflectionPool::DeregisterReflectionProbe(GetWorld(), m_Id);
  m_Id.Invalidate();

  GetOwner()->UpdateLocalBounds();
}

void plSphereReflectionProbeComponent::OnObjectCreated(const plAbstractObjectNode& node)
{
  m_Desc.m_uniqueID = node.GetGuid();
}

void plSphereReflectionProbeComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  msg.SetAlwaysVisible(plDefaultSpatialDataCategories::RenderDynamic);
}

void plSphereReflectionProbeComponent::OnMsgExtractRenderData(plMsgExtractRenderData& msg) const
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
  pRenderData->m_vHalfExtents = plVec3(m_fRadius);
  pRenderData->m_vInfluenceScale = plVec3(1.0f);
  pRenderData->m_vInfluenceShift = plVec3(0.0f);
  pRenderData->m_vPositiveFalloff = plVec3(m_fFalloff);
  pRenderData->m_vNegativeFalloff = plVec3(m_fFalloff);
  pRenderData->m_Id = m_Id;
  pRenderData->m_uiIndex = REFLECTION_PROBE_IS_SPHERE;
  if (m_bSphereProjection)
    pRenderData->m_uiIndex |= REFLECTION_PROBE_IS_PROJECTED;

  const plVec3 vScale = pRenderData->m_GlobalTransform.m_vScale * m_fRadius;
  constexpr float fSphereConstant = (4.0f / 3.0f) * plMath::Pi<float>();
  const float fEllipsoidVolume = fSphereConstant * plMath::Abs(vScale.x * vScale.y * vScale.z);

  float fPriority = ComputePriority(msg, pRenderData, fEllipsoidVolume, vScale);
  plReflectionPool::ExtractReflectionProbe(this, msg, pRenderData, GetWorld(), m_Id, fPriority);
}

void plSphereReflectionProbeComponent::OnTransformChanged(plMsgTransformChanged& msg)
{
  m_bStatesDirty = true;
}

void plSphereReflectionProbeComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_fRadius;
  s << m_fFalloff;
  s << m_bSphereProjection;
}

void plSphereReflectionProbeComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_fRadius;
  s >> m_fFalloff;
  if (uiVersion >= 2)
  {
    s >> m_bSphereProjection;
  }
  else
  {
    m_bSphereProjection = false;
  }
}


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>

class plSphereReflectionProbeComponent_1_2 : public plGraphPatch
{
public:
  plSphereReflectionProbeComponent_1_2()
    : plGraphPatch("plSphereReflectionProbeComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override
  {
    pNode->AddProperty("SphereProjection", false);
  }
};

plSphereReflectionProbeComponent_1_2 g_plSphereReflectionProbeComponent_1_2;

PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_SphereReflectionProbeComponent);
