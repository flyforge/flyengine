#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingInterface.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plBakingSettings, plNoBase, 1, plRTTIDefaultAllocator<plBakingSettings>)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new plDefaultValueAttribute(plVec3(4)), new plClampValueAttribute(plVec3(0.1f), plVariant())),
    PL_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new plDefaultValueAttribute(128), new plClampValueAttribute(32, 1024)),
    PL_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new plDefaultValueAttribute(1000), new plClampValueAttribute(1, plVariant())),
  }
  PL_END_PROPERTIES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

static plTypeVersion s_BakingSettingsVersion = 1;
plResult plBakingSettings::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return PL_SUCCESS;
}

plResult plBakingSettings::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);
  PL_IGNORE_UNUSED(version);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return PL_SUCCESS;
}


PL_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingInterface);
