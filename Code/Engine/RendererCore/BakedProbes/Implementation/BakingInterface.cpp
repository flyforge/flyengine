#include <RendererCore/RendererCorePCH.h>

#include <RendererCore/BakedProbes/BakingInterface.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plBakingSettings, plNoBase, 1, plRTTIDefaultAllocator<plBakingSettings>)
{
  PLASMA_BEGIN_PROPERTIES
  {
    PLASMA_MEMBER_PROPERTY("ProbeSpacing", m_vProbeSpacing)->AddAttributes(new plDefaultValueAttribute(plVec3(4)), new plClampValueAttribute(plVec3(0.1f), plVariant())),
    PLASMA_MEMBER_PROPERTY("NumSamplesPerProbe", m_uiNumSamplesPerProbe)->AddAttributes(new plDefaultValueAttribute(128), new plClampValueAttribute(32, 1024)),
    PLASMA_MEMBER_PROPERTY("MaxRayDistance", m_fMaxRayDistance)->AddAttributes(new plDefaultValueAttribute(1000), new plClampValueAttribute(1, plVariant())),
  }
  PLASMA_END_PROPERTIES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

static plTypeVersion s_BakingSettingsVersion = 1;
plResult plBakingSettings::Serialize(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_BakingSettingsVersion);

  inout_stream << m_vProbeSpacing;
  inout_stream << m_uiNumSamplesPerProbe;
  inout_stream << m_fMaxRayDistance;

  return PLASMA_SUCCESS;
}

plResult plBakingSettings::Deserialize(plStreamReader& inout_stream)
{
  const plTypeVersion version = inout_stream.ReadVersion(s_BakingSettingsVersion);

  inout_stream >> m_vProbeSpacing;
  inout_stream >> m_uiNumSamplesPerProbe;
  inout_stream >> m_fMaxRayDistance;

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(RendererCore, RendererCore_BakedProbes_Implementation_BakingInterface);
