#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/Messages/UpdateLocalBoundsMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <JoltPlugin/Resources/JoltMaterial.h>
#include <JoltPlugin/Shapes/JoltShapeCylinderComponent.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
PLASMA_BEGIN_COMPONENT_TYPE(plJoltShapeCylinderComponent, 1, plComponentMode::Static)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("Height", GetHeight, SetHeight)->AddAttributes(new plDefaultValueAttribute(1.0f), new plClampValueAttribute(0.0f, plVariant())),
    PLASMA_ACCESSOR_PROPERTY("Radius", GetRadius, SetRadius)->AddAttributes(new plDefaultValueAttribute(0.25f), new plClampValueAttribute(0.0f, plVariant())),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgUpdateLocalBounds, OnUpdateLocalBounds),
  }
  PLASMA_END_MESSAGEHANDLERS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCylinderVisualizerAttribute(plBasisAxis::PositiveZ, "Height", "Radius"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plJoltShapeCylinderComponent::plJoltShapeCylinderComponent() = default;
plJoltShapeCylinderComponent::~plJoltShapeCylinderComponent() = default;

void plJoltShapeCylinderComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  auto& s = inout_stream.GetStream();
  s << m_fRadius;
  s << m_fHeight;
}

void plJoltShapeCylinderComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());


  auto& s = inout_stream.GetStream();
  s >> m_fRadius;
  s >> m_fHeight;
}

void plJoltShapeCylinderComponent::OnUpdateLocalBounds(plMsgUpdateLocalBounds& msg) const
{
  msg.AddBounds(plBoundingBox(plVec3(-m_fRadius, -m_fRadius, -m_fHeight * 0.5f), plVec3(m_fRadius, m_fRadius, m_fHeight * 0.5f)), plInvalidSpatialDataCategory);
}

void plJoltShapeCylinderComponent::SetRadius(float f)
{
  m_fRadius = plMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeCylinderComponent::SetHeight(float f)
{
  m_fHeight = plMath::Max(f, 0.0f);

  if (IsActiveAndInitialized())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void plJoltShapeCylinderComponent::CreateShapes(plDynamicArray<plJoltSubShape>& out_Shapes, const plTransform& rootTransform, float fDensity, const plJoltMaterial* pMaterial)
{
  auto pNewShape = new JPH::CylinderShape(m_fHeight * 0.5f, m_fRadius);
  pNewShape->AddRef();
  pNewShape->SetDensity(fDensity);
  pNewShape->SetUserData(reinterpret_cast<plUInt64>(GetUserData()));
  pNewShape->SetMaterial(pMaterial);

  const plQuat qTilt = plBasisAxis::GetBasisRotation(plBasisAxis::PositiveY, plBasisAxis::PositiveZ);

  plTransform tOwn = GetOwner()->GetGlobalTransform();
  tOwn.m_vScale.x = tOwn.m_vScale.z;
  tOwn.m_qRotation = tOwn.m_qRotation * qTilt;

  plJoltSubShape& sub = out_Shapes.ExpandAndGetRef();
  sub.m_pShape = pNewShape;
  sub.m_Transform.SetLocalTransform(rootTransform, tOwn);
}


PLASMA_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeCylinderComponent);

