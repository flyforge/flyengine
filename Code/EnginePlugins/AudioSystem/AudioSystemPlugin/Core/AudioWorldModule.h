#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Components/AudioListenerComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Core/World/WorldModule.h>

/// \brief World Module allowing to access audio system features, query environments
///  and environment amounts, and extracting obstruction/occlusion data.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioWorldModule : public plWorldModule
{
  PL_DECLARE_WORLD_MODULE();
  PL_ADD_DYNAMIC_REFLECTION(plAudioWorldModule, plWorldModule);

  using EnvironmentSet = plSet<const plAudioSystemEnvironmentComponent*, plCompareHelper<const plAudioSystemEnvironmentComponent*>, plAudioSystemAllocatorWrapper>;

public:
  explicit plAudioWorldModule(plWorld* pWorld);
  ~plAudioWorldModule() override;

  void Initialize() override;

  void AddEnvironment(const plAudioSystemEnvironmentComponent* pComponent);
  void RemoveEnvironment(const plAudioSystemEnvironmentComponent* pComponent);
  PL_NODISCARD EnvironmentSet::Iterator GetEnvironments() const;

  void SetDefaultListener(const plAudioListenerComponent* pListener);
  PL_NODISCARD const plAudioListenerComponent* GetDefaultListener() const;

private:
  EnvironmentSet m_lEnvironmentComponents;
  const plAudioListenerComponent* m_pDefaultListener;

  // TODO: Add events handlers for default listener change
};
