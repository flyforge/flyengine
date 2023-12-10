#include <AudioSystemPlugin/AudioSystemPluginPCH.h>

#include <AudioSystemPlugin/Core/AudioWorldModule.h>

#include <Core/World/World.h>

// clang-format off
PLASMA_IMPLEMENT_WORLD_MODULE(plAudioWorldModule);

PLASMA_BEGIN_DYNAMIC_REFLECTED_TYPE(plAudioWorldModule, 1, plRTTINoAllocator)
PLASMA_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

plAudioWorldModule::plAudioWorldModule(plWorld* pWorld)
  : plWorldModule(pWorld)
  , m_pDefaultListener(nullptr)
{
}

plAudioWorldModule::~plAudioWorldModule()
{
  m_lEnvironmentComponents.Clear();
}

void plAudioWorldModule::Initialize()
{
  SUPER::Initialize();
}

void plAudioWorldModule::AddEnvironment(const plAudioSystemEnvironmentComponent* pComponent)
{
  m_lEnvironmentComponents.Insert(pComponent);
}

void plAudioWorldModule::RemoveEnvironment(const plAudioSystemEnvironmentComponent* pComponent)
{
  m_lEnvironmentComponents.Remove(pComponent);
}

plAudioWorldModule::EnvironmentSet::Iterator plAudioWorldModule::GetEnvironments() const
{
  return m_lEnvironmentComponents.GetIterator();
}

void plAudioWorldModule::SetDefaultListener(const plAudioListenerComponent* pListener)
{
  m_pDefaultListener = pListener;
}

const plAudioListenerComponent* plAudioWorldModule::GetDefaultListener() const
{
  return m_pDefaultListener;
}

PLASMA_STATICLINK_FILE(AudioSystemPlugin, AudioSystemPlugin_Implementation_Core_AudioWorldModule);
