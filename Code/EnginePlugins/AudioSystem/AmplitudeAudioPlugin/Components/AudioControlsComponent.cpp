#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AmplitudeAudioPlugin/Components/AudioControlsComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemRequests.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>

constexpr plTypeVersion kVersion_AudioControlsComponent = 1;

// clang-format off
PL_BEGIN_COMPONENT_TYPE(plAudioControlsComponent, kVersion_AudioControlsComponent, plComponentMode::Static)
{
  PL_BEGIN_PROPERTIES
  {
    PL_MEMBER_PROPERTY("ControlsAsset", m_sControlsAsset)->AddAttributes(new plAssetBrowserAttribute("Audio Control Collection")),
    PL_MEMBER_PROPERTY("AutoLoad", m_bAutoLoad),

    PL_MEMBER_PROPERTY_READ_ONLY("Loaded", m_bLoaded)->AddAttributes(new plHiddenAttribute()),
  }
  PL_END_PROPERTIES;

  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Load),
    PL_SCRIPT_FUNCTION_PROPERTY(Unload),
  }
  PL_END_FUNCTIONS;
}
PL_END_COMPONENT_TYPE;
// clang-format on

void plAudioControlsComponent::Initialize()
{
  SUPER::Initialize();

  if (m_bAutoLoad)
    Load();
}

void plAudioControlsComponent::Deinitialize()
{
  if (m_bLoaded)
    Unload();

  SUPER::Deinitialize();
}

void plAudioControlsComponent::SerializeComponent(plWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  s.WriteVersion(kVersion_AudioControlsComponent);

  s << m_sControlsAsset;
  s << m_bAutoLoad;
}

void plAudioControlsComponent::DeserializeComponent(plWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);

  auto& s = stream.GetStream();

  s.ReadVersion(kVersion_AudioControlsComponent);

  s >> m_sControlsAsset;
  s >> m_bAutoLoad;
}

plAudioControlsComponent::plAudioControlsComponent()
  : plAmplitudeComponent()
  , m_bAutoLoad(false)
  , m_bLoaded(false)
{
}

plAudioControlsComponent::~plAudioControlsComponent() = default;

bool plAudioControlsComponent::Load()
{
  if (m_sControlsAsset.IsEmpty())
    return false;

  if (m_bLoaded)
    return true;

  m_hControlsResource = plResourceManager::LoadResource<plAmplitudeAudioControlCollectionResource>(m_sControlsAsset);
  if (const plResourceLock resource(m_hControlsResource, plResourceAcquireMode::BlockTillLoaded); resource.GetAcquireResult() == plResourceAcquireResult::MissingFallback)
  {
    plLog::Error("Failed to load audio control collection '{0}'", m_sControlsAsset);
    return false;
  }

  m_bLoaded = true;
  return true;
}

bool plAudioControlsComponent::Unload()
{
  if (!m_bLoaded || !m_hControlsResource.IsValid())
    return false;

  plResourceManager::FreeAllUnusedResources();
  m_hControlsResource.Invalidate();
  plResourceManager::FreeAllUnusedResources();

  m_bLoaded = false;
  return true;
}

#if defined(AM_WINDOWS_VERSION)
#  include <Foundation/Basics/Platform/Win/IncludeWindows.h>
#endif

PL_STATICLINK_FILE(AmplitudeAudioPlugin, AmplitudeAudioPlugin_Components_AudioControlsComponent);
