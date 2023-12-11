#include <RendererCore/RendererCorePCH.h>

#include <Core/Messages/TransformChangedMessage.h>
#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Components/OccluderComponent.h>
#include <RendererCore/Pipeline/RenderData.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plOccluderComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Extents", GetExtents, SetExtents)->AddAttributes(new plClampValueAttribute(plVec3(0.0f), {}), new plDefaultValueAttribute(plVec3(1.0f))),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
    PLASMA_MESSAGE_HANDLER(plMsgExtractOccluderData, OnMsgExtractOccluderData),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Rendering"),
    new plBoxVisualizerAttribute("Extents", 1.0f, plColorScheme::LightUI(plColorScheme::Blue)),
    new plBoxManipulatorAttribute("Extents", 1.0f, true),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_COMPONENT_TYPE
// clang-format on

plOccluderComponentManager::plOccluderComponentManager(plWorld* pWorld)
  : plComponentManager<plOccluderComponent, plBlockStorageType::FreeList>(pWorld)
{
}

//////////////////////////////////////////////////////////////////////////

plOccluderComponent::plOccluderComponent() = default;
plOccluderComponent::~plOccluderComponent() = default;

void plOccluderComponent::SetExtents(const plVec3& vExtents)
{
  m_vExtents = vExtents;
  m_pOccluderObject.Clear();

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plOccluderComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg)
{
  if (GetOwner()->IsStatic())
    msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), plDefaultSpatialDataCategories::OcclusionStatic);
  else
    msg.AddBounds(plBoundingBoxSphere::MakeFromBox(plBoundingBox::MakeFromMinMax(-m_vExtents * 0.5f, m_vExtents * 0.5f)), plDefaultSpatialDataCategories::OcclusionDynamic);
}

void plOccluderComponent::OnMsgExtractOccluderData(plMsgExtractOccluderData& msg) const
{
  if (IsActiveAndInitialized())
  {
    if (m_pOccluderObject == nullptr)
    {
      m_pOccluderObject = plRasterizerObject::CreateBox(m_vExtents);
    }

    msg.AddOccluder(m_pOccluderObject.Borrow(), GetOwner()->GetGlobalTransform());
  }
}

void plOccluderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  s << m_vExtents;
}

void plOccluderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  // const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  s >> m_vExtents;
}

void plOccluderComponent::OnActivated()
{
  m_pOccluderObject.Clear();
  GetOwner()->UpdateLocalBounds();
}

void plOccluderComponent::OnDeactivated()
{
  m_pOccluderObject.Clear();
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Components_Implementation_OccluderComponent);