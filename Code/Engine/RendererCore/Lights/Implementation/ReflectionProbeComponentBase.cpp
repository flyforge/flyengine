#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/Lights/ReflectionProbeComponentBase.h>

#include <Core/Graphics/Camera.h>
#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <RendererCore/Pipeline/RenderData.h>
#include <RendererCore/Pipeline/View.h>

namespace
{
  static plVariantArray GetDefaultExcludeTags()
  {
    plVariantArray value(plStaticAllocatorWrapper::GetAllocator());
    value.PushBack(plStringView("SkyLight"));
    return value;
  }
} // namespace


// clang-format off
PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plReflectionProbeComponentBase, 2, plRTTINoAllocator)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_ENUM_ACCESSOR_PROPERTY("ReflectionProbeMode", plReflectionProbeMode, GetReflectionProbeMode, SetReflectionProbeMode)->AddAttributes(new plDefaultValueAttribute(plReflectionProbeMode::Static), new plGroupAttribute("Capture Description")),
    PLASMA_SET_ACCESSOR_PROPERTY("IncludeTags", GetIncludeTags, InsertIncludeTag, RemoveIncludeTag)->AddAttributes(new plTagSetWidgetAttribute("Default")),
    PLASMA_SET_ACCESSOR_PROPERTY("ExcludeTags", GetExcludeTags, InsertExcludeTag, RemoveExcludeTag)->AddAttributes(new plTagSetWidgetAttribute("Default"), new plDefaultValueAttribute(GetDefaultExcludeTags())),
    PLASMA_ACCESSOR_PROPERTY("NearPlane", GetNearPlane, SetNearPlane)->AddAttributes(new plDefaultValueAttribute(0.0f), new plClampValueAttribute(0.0f, {}), new plMinValueTextAttribute("Auto")),
    PLASMA_ACCESSOR_PROPERTY("FarPlane", GetFarPlane, SetFarPlane)->AddAttributes(new plDefaultValueAttribute(100.0f), new plClampValueAttribute(0.01f, 10000.0f)),
    PLASMA_ACCESSOR_PROPERTY("CaptureOffset", GetCaptureOffset, SetCaptureOffset),
    PLASMA_ACCESSOR_PROPERTY("ShowDebugInfo", GetShowDebugInfo, SetShowDebugInfo),
    PLASMA_ACCESSOR_PROPERTY("ShowMipMaps", GetShowMipMaps, SetShowMipMaps),
  }
  PLASMA_END_PROPERTIES;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plTransformManipulatorAttribute("CaptureOffset"),
    new plColorAttribute(plColorScheme::Rendering),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plReflectionProbeComponentBase::plReflectionProbeComponentBase()
{
  m_Desc.m_uniqueID.CreateNewUuid();
}

plReflectionProbeComponentBase::~plReflectionProbeComponentBase()
{
}

void plReflectionProbeComponentBase::SetReflectionProbeMode(plEnum<plReflectionProbeMode> mode)
{
  m_Desc.m_Mode = mode;
  m_bStatesDirty = true;
}

plEnum<plReflectionProbeMode> plReflectionProbeComponentBase::GetReflectionProbeMode() const
{
  return m_Desc.m_Mode;
}

const plTagSet& plReflectionProbeComponentBase::GetIncludeTags() const
{
  return m_Desc.m_IncludeTags;
}

void plReflectionProbeComponentBase::InsertIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::RemoveIncludeTag(const char* szTag)
{
  m_Desc.m_IncludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}


const plTagSet& plReflectionProbeComponentBase::GetExcludeTags() const
{
  return m_Desc.m_ExcludeTags;
}

void plReflectionProbeComponentBase::InsertExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.SetByName(szTag);
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::RemoveExcludeTag(const char* szTag)
{
  m_Desc.m_ExcludeTags.RemoveByName(szTag);
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::SetNearPlane(float fNearPlane)
{
  m_Desc.m_fNearPlane = fNearPlane;
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::SetFarPlane(float fFarPlane)
{
  m_Desc.m_fFarPlane = fFarPlane;
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::SetCaptureOffset(const plVec3& vOffset)
{
  m_Desc.m_vCaptureOffset = vOffset;
  m_bStatesDirty = true;
}

void plReflectionProbeComponentBase::SetShowDebugInfo(bool bShowDebugInfo)
{
  m_Desc.m_bShowDebugInfo = bShowDebugInfo;
  m_bStatesDirty = true;
}

bool plReflectionProbeComponentBase::GetShowDebugInfo() const
{
  return m_Desc.m_bShowDebugInfo;
}

void plReflectionProbeComponentBase::SetShowMipMaps(bool bShowMipMaps)
{
  m_Desc.m_bShowMipMaps = bShowMipMaps;
  m_bStatesDirty = true;
}

bool plReflectionProbeComponentBase::GetShowMipMaps() const
{
  return m_Desc.m_bShowMipMaps;
}

void plReflectionProbeComponentBase::SerializeComponent(plWorldWriter& inout_stream) const
{
  SUPER::SerializeComponent(inout_stream);

  plStreamWriter& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Save(s);
  m_Desc.m_ExcludeTags.Save(s);
  s << m_Desc.m_Mode;
  s << m_Desc.m_bShowDebugInfo;
  s << m_Desc.m_uniqueID;
  s << m_Desc.m_fNearPlane;
  s << m_Desc.m_fFarPlane;
  s << m_Desc.m_vCaptureOffset;
}

void plReflectionProbeComponentBase::DeserializeComponent(plWorldReader& inout_stream)
{
  SUPER::DeserializeComponent(inout_stream);
  //const plUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
  plStreamReader& s = inout_stream.GetStream();

  m_Desc.m_IncludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
  m_Desc.m_ExcludeTags.Load(s, plTagRegistry::GetGlobalRegistry());
  s >> m_Desc.m_Mode;
  s >> m_Desc.m_bShowDebugInfo;
  s >> m_Desc.m_uniqueID;
  s >> m_Desc.m_fNearPlane;
  s >> m_Desc.m_fFarPlane;
  s >> m_Desc.m_vCaptureOffset;
}

float plReflectionProbeComponentBase::ComputePriority(plMsgExtractRenderData& msg, plReflectionProbeRenderData* pRenderData, float fVolume, const plVec3& vScale) const
{
  float fPriority = 0.0f;
  const float fLogVolume = plMath::Log2(1.0f + fVolume); // +1 to make sure it never goes negative.
  // This sorting is only by size to make sure the probes in a cluster are iterating from smallest to largest on the GPU. Which probes are actually used is determined below by the returned priority.
  pRenderData->m_uiSortingKey = plMath::FloatToInt(static_cast<float>(plMath::MaxValue<plUInt32>()) * fLogVolume / 40.0f);

  //#TODO This is a pretty poor distance / size based score.
  if (msg.m_pView)
  {
    if (auto pCamera = msg.m_pView->GetLodCamera())
    {
      float fDistance = (pCamera->GetPosition() - pRenderData->m_GlobalTransform.m_vPosition).GetLength();
      float fRadius = (plMath::Abs(vScale.x) + plMath::Abs(vScale.y) + plMath::Abs(vScale.z)) / 3.0f;
      fPriority = fRadius / fDistance;
    }
  }

#ifdef PLASMA_SHOW_REFLECTION_PROBE_PRIORITIES
  plStringBuilder s;
  s.Format("{}, {}", pRenderData->m_uiSortingKey, fPriority);
  plDebugRenderer::Draw3DText(GetWorld(), s, pRenderData->m_GlobalTransform.m_vPosition, plColor::Wheat);
#endif
  return fPriority;
}

PLASMA_STATICLINK_FILE(RendererCore, RendererCore_Lights_Implementation_ReflectionProbeComponentBase);
