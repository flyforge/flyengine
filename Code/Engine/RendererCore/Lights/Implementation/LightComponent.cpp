#include <RendererCore/RendererCorePCH.h>

#include <Core/Graphics/Camera.h>
#include <Core/Messages/SetColorMessage.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Lights/LightComponent.h>

// clang-format off
PL_BEGIN_DYNAMIC_REFLECTED_TYPE(plLightRenderData, 1, plRTTINoAllocator)
PL_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

void plLightRenderData::FillBatchIdAndSortingKey(float fScreenSpaceSize)
{
  m_uiSortingKey = (m_uiShadowDataOffset != plInvalidIndex) ? 0 : 1;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
PL_BEGIN_ABSTRACT_COMPONENT_TYPE(plLightComponent, 6)
{
  PL_BEGIN_PROPERTIES
  {
    PL_ACCESSOR_PROPERTY("UseColorTemperature", GetUsingColorTemperature, SetUsingColorTemperature),
    PL_ACCESSOR_PROPERTY("LightColor", GetLightColor, SetLightColor),
    PL_ACCESSOR_PROPERTY("Temperature", GetTemperature, SetTemperature)->AddAttributes(new plImageSliderUiAttribute("LightTemperature"), new plDefaultValueAttribute(6550), new plClampValueAttribute(1000, 15000)),
    PL_ACCESSOR_PROPERTY("Intensity", GetIntensity, SetIntensity)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(10.0f)),
    PL_ACCESSOR_PROPERTY("SpecularMultiplier", GetSpecularMultiplier, SetSpecularMultiplier)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(1.0f)),
    PL_ACCESSOR_PROPERTY("Width", GetWidth, SetWidth)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.0f)),
    PL_ACCESSOR_PROPERTY("Length", GetLength, SetLength)->AddAttributes(new plClampValueAttribute(0.0f, plVariant()), new plDefaultValueAttribute(0.0f)),
    PL_ACCESSOR_PROPERTY("CastShadows", GetCastShadows, SetCastShadows),
    PL_ACCESSOR_PROPERTY("PenumbraSize", GetPenumbraSize, SetPenumbraSize)->AddAttributes(new plClampValueAttribute(0.0f, 0.5f), new plDefaultValueAttribute(0.1f), new plSuffixAttribute(" m")),
    PL_ACCESSOR_PROPERTY("SlopeBias", GetSlopeBias, SetSlopeBias)->AddAttributes(new plClampValueAttribute(0.0f, 10.0f), new plDefaultValueAttribute(0.25f)),
    PL_ACCESSOR_PROPERTY("ConstantBias", GetConstantBias, SetConstantBias)->AddAttributes(new plClampValueAttribute(0.0f, 10.0f), new plDefaultValueAttribute(0.1f))
  }
  PL_END_PROPERTIES;
  PL_BEGIN_ATTRIBUTES
  {
    new plCategoryAttribute("Lighting"),
    new plCapsuleVisualizerAttribute("Length", "Width"),
    new plCapsuleVisualizerAttribute("Length",  "Width", plColor::Grey),
  }
  PL_END_ATTRIBUTES;
  PL_BEGIN_MESSAGEHANDLERS
  {
    PL_MESSAGE_HANDLER(plMsgSetColor, OnMsgSetColor),
  }
  PL_END_MESSAGEHANDLERS;
}
PL_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

plLightComponent::plLightComponent() = default;
plLightComponent::~plLightComponent() = default;

void plLightComponent::SetUsingColorTemperature(bool bUseColorTemperature)
{
  m_bUseColorTemperature = bUseColorTemperature;

  InvalidateCachedRenderData();
}

bool plLightComponent::GetUsingColorTemperature() const
{
  return m_bUseColorTemperature;
}

void plLightComponent::SetLightColor(plColorGammaUB lightColor)
{
  m_LightColor = lightColor;

  InvalidateCachedRenderData();
}

plColorGammaUB plLightComponent::GetBaseLightColor() const
{
  return m_LightColor;
}

plColorGammaUB plLightComponent::GetLightColor() const
{
  if (m_bUseColorTemperature)
  {
    return plColor::MakeFromKelvin(m_uiTemperature);
  }
  else
  {
    return m_LightColor;
  }
}

void plLightComponent::SetIntensity(float fIntensity)
{
  m_fIntensity = plMath::Max(fIntensity, 0.0f);

  TriggerLocalBoundsUpdate();
}

void plLightComponent::SetTemperature(plUInt32 uiTemperature)
{
  m_uiTemperature = plMath::Clamp(uiTemperature, 1500u, 40000u);

  InvalidateCachedRenderData();
}

plUInt32 plLightComponent::GetTemperature() const
{
  return m_uiTemperature;
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

void plLightComponent::SetWidth(float fWidth)
{
  m_fWidth = fWidth;

  InvalidateCachedRenderData();
}

float plLightComponent::GetWidth() const
{
  return m_fWidth;
}

void plLightComponent::SetLength(float fLength)
{
  m_fLength = fLength;

  InvalidateCachedRenderData();
}

float plLightComponent::GetLength() const
{
  return m_fLength;
}

void plLightComponent::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);
  plStreamWriter& s = inout_stream.GetStream();

  s << m_LightColor;
  s << m_fIntensity;
  s << m_fPenumbraSize;
  s << m_fSlopeBias;
  s << m_fConstantBias;
  s << m_bCastShadows;
  s << m_bUseColorTemperature;
  s << m_uiTemperature;
  s << m_fSpecularMultiplier;
  s << m_fWidth;
  s << m_fLength;
}

void plLightComponent::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  const plUInt32 uiVersion = inout_stream.GetComponentTypeVersion(GetStaticRTTI());

  plStreamReader& s = inout_stream.GetStream();

  s >> m_LightColor;
  s >> m_fIntensity;

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

  if (uiVersion >= 5)
  {

    s >> m_bUseColorTemperature;
    s >> m_uiTemperature;
    s >> m_fSpecularMultiplier;
  }

  if(uiVersion >= 6)
  {
    s >> m_fWidth;
    s >> m_fLength;
  }
}

void plLightComponent::OnMsgSetColor(plMsgSetColor& ref_msg)
{
  ref_msg.ModifyColor(m_LightColor);

  InvalidateCachedRenderData();
}

// static
float plLightComponent::CalculateEffectiveRange(float fRange, float fIntensity, float fWidth, float fLength)
{
  const float fThreshold = 0.10f; // aggressive threshold to prevent large lights
  const float fEffectiveRange = plMath::Sqrt(plMath::Max(0.0f, fIntensity) * ((1 + fWidth) * (1 + fLength))) / plMath::Sqrt(fThreshold);

  PL_ASSERT_DEBUG(!plMath::IsNaN(fEffectiveRange), "Light range is NaN");

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

#include <Foundation/Serialization/AbstractObjectGraph.h>
#include <Foundation/Serialization/GraphPatch.h>

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



PL_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_LightComponent);
