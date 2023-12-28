#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/LightComponent.h>

PLASMA_BEGIN_STATIC_REFLECTED_ENUM(plLightUnit, 1)
  PLASMA_ENUM_CONSTANT(plLightUnit::Lumen),
  PLASMA_ENUM_CONSTANT(plLightUnit::Candela),
PLASMA_END_STATIC_REFLECTED_ENUM;

// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plLightRenderData, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffset != plInvalidIndex) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PLASMA_BEGIN_ABSTRACT_COMPONENT_TYPE(plLightComponent, 7)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    PLASMA_ENUM_MEMBER_PROPERTY("LightUnit", plLightUnit, m_LightUnit)->AddAttributes(new plDefaultValueAttribute(plLightUnit::Lumen)),
    PLASMA_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(800.0f)),
    PLASMA_ACCESSOR_PROPERTY("SpecularMultiplier", GetSpecularMultiplier, SetSpecularMultiplier)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new plClampValueAttribute(1000.0f, 15000.0f), new plDefaultValueAttribute(6550.0f), new plSuffixAttribute(" kelvin")),
    PLASMA_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    PLASMA_ACCESSOR_PROPERTY("VolumetricIntensity", GetVolumetricIntensity, SetVolumetricIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PLASMA_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new plClampValueAttribute(0.0f, 0.5f), new plDefaultValueAttribute(0.1f), new plSuffixAttribute(" m")),
    PLASMA_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new plClampValueAttribute(0.0f, 10.0f), new plDefaultValueAttribute(0.25f)),
    PLASMA_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new plClampValueAttribute(0.0f, 10.0f), new plDefaultValueAttribute(0.1f))
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Lighting"),
  }
  PLASMA_END_ATTRIBUTES;
  PLASMA_BEGIN_MESSAGEHANDLERS
  {
    PLASMA_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  }
  PLASMA_END_MESSAGEHANDLERS;
}
PLASMA_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plLightComponent::plLightComponent() = default;
plLightComponent::~plLightComponent() = default;

void plLightComponent::SetLightColor(plColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

plColorGammaUB plLightComponent::GetLightColor() const
{
  return m_LightColor;
}

void plLightComponent::SetLightUnit(plEnum<plLightUnit> lightUnit)
{
  m_LightUnit = lightUnit;
}

plEnum<plLightUnit> plLightComponent::GetLightUnit() const
{
  return m_LightUnit;
}

void plLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = plMath::Max(fIntensity, 0.0f);

  InvalidateCachedRenderData();
}

float plLightComponent::GetIntensity() const
{
  return m_fIntensity;
}

void plLightComponent::SetSpecularMultiplier(float fSpecularMultiplier)
{
  m_fSpecularMultiplier = plMath::Max(fSpecularMultiplier, 0.0f);

  InvalidateCachedRenderData();
}

float plLightComponent::GetSpecularMultiplier() const
{
  return m_fSpecularMultiplier;
}

void plLightComponent::SetVolumetricIntensity(float fVolumetricIntensity)
{
  m_fVolumetricIntensity = plMath::Max(fVolumetricIntensity, 0.0f);

  InvalidateCachedRenderData();
}

float plLightComponent::GetVolumetricIntensity() const
{
  return m_fVolumetricIntensity;
}

void plLightComponent::SetTemperature(float fTemperature)
{
  m_fTemperature = plMath::Clamp(fTemperature, 1500.0f, 40000.0f);

  InvalidateCachedRenderData();
}

float plLightComponent::GetTemperature() const
{
  return m_fTemperature;
}

void plLightComponent::SetCastShadows(bool bCastShadows)
{
  m_bCastShadows = bCastShadows;

  InvalidateCachedRenderData();
}

bool plLightComponent::GetCastShadows() const
{
  return m_bCastShadows;
}

void plLightComponent::SetPenumbraSize(float fPenumbraSize)
{
  m_fPenumbraSize = fPenumbraSize;

  InvalidateCachedRenderData();
}

float plLightComponent::GetPenumbraSize() const
{
  return m_fPenumbraSize;
}

void plLightComponent::SetSlopeBias(float fBias)
{
  m_fSlopeBias = fBias;

  InvalidateCachedRenderData();
}

float plLightComponent::GetSlopeBias() const
{
  return m_fSlopeBias;
}

void plLightComponent::SetConstantBias(float fBias)
{
  m_fConstantBias = fBias;

  InvalidateCachedRenderData();
}

float plLightComponent::GetConstantBias() const
{
  return m_fConstantBias;
}

void plLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fTemperature;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
  s << m_LightUnit;
}

void plLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

  if (uiVersion >= 5)
  {
    s >> m_fTemperature;
  }

  if (uiVersion >= 3)
  {
    s >> m_fPenumbraSize;
  }

  if (uiVersion >= 4)
  {
    s >> m_fSlopeBias;
    s >> m_fConstantBias;
  }

  s >> m_bCastShadows;

  if (uiVersion >= 7)
  {
    s >> m_LightUnit;
  }
}

void plLightComponent::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

// static
float plLightComponent::CalculateEffectiveRange(float fRange, float fIntensity)
{
  const float fThreshold = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = plMath::Sqrt(plMath::Max(0.0f, fIntensity)) / plMath::Sqrt(fThreshold);

  PLASMA_ASSERT_DEBUG(!plMath::IsNaN(fEffectiveRange), "Light range is NaN");

  if (fRange <= 0.0f)
  {
    return fEffectiveRange;
  }

  return plMath::Min(fRange, fEffectiveRange);
}

// static
float plLightComponent::CalculateScreenSpaceSize(const plBoundingSphere& sphere, const plCamera& camera)
{
  if (camera.IsPerspective())
  {
    float dist = (sphere.m_vCenter - camera.GetPosition()).GetLength();
    float fHalfHeight = plMath::Tan(camera.GetFovY(1.0f) * 0.5f) * dist;
    return plMath::Pow(sphere.m_fRadius / fHalfHeight, 0.8f); // tweak factor to make transitions more linear.
  }
  else
  {
    float fHalfHeight = camera.GetDimensionY(1.0f) * 0.5f;
    return sphere.m_fRadius / fHalfHeight;
  }
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

#include <Foundation/Serialization/GraphPatch.h>
#include <Foundation/Serialization/AbstractObjectGraph.h>

class plLightComponentPatch_1_2 : public plGraphPatch
{
public:
  plLightComponentPatch_1_2()
    : plGraphPatch("plLightComponent", 2)
  {
  }

  virtual void Patch(plGraphPatchContext& ref_context, plAbstractObjectGraph* pGraph, plAbstractObjectNode* pNode) const override { pNode->RenameProperty("Light Color", "LightColor"); }
};

plLightComponentPatch_1_2 g_plLightComponentPatch_1_2;



PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);
