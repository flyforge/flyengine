#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>
#include <AudioSystemPlugin/Core/AudioSystemAllocator.h>

#include <Foundation/Types/RefCounted.h>

typedef plAudioSystemComponentManager<class plAudioProxyComponent> plAudioProxyComponentManager;

/// \brief Stores the environment amounts for a single audio proxy.
struct plAudioProxyEnvironmentAmounts
{
  float m_fNextAmount{0.0f};
  float m_fPreviousAmount{0.0f};
};

/// \brief Component that represent an audio entity in the scene graph.
///
/// Audio proxies captures position and orientation data from the owner game object and pass them
/// to the audio system to update entity transformation.
///
/// Audio components can't work without an audio proxy, since it is used to identify which entity
/// has made a request. With that fact, exactly one audio proxy component should be present in the
/// game object willing to send audio requests. If no audio proxy component is detected at the
/// initialization of the scene, one will be created automatically, hence, it is not required to
/// add this component manually in the editor.
class PLASMA_AUDIOSYSTEMPLUGIN_DLL plAudioProxyComponent : public plAudioSystemComponent, public plRefCounted
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioProxyComponent, plAudioSystemComponent, plAudioProxyComponentManager);

  // plComponent

public:
  void Initialize() override;
  void Deinitialize() override;

  // plAudioSystemComponent

private:
  void plAudioSystemComponentIsAbstract() override {}

  // plAudioProxyComponent

public:
  plAudioProxyComponent();
  ~plAudioProxyComponent() override;

  /// \brief Gets the assigned Audio System Entity ID to this game object.
  ///
  /// Entity ids are automatically assigned at the start of the simulation. They may change each time
  /// the game is simulated. You should not assert that the same ID will always be assigned to the same game object every time.
  PLASMA_NODISCARD plAudioSystemDataID GetEntityId() const;

  /// \brief Unregisters this proxy from the audio system.
  ///
  /// Note that the proxy will be unregistered only if it is not referenced anymore, unless bForce is set to true.
  ///
  /// \param bForce Indicates if the entity should be unregistered even if some audio components depends on it.
  void Unregister(bool bForce = false) const;

protected:
  void Update();

private:
  friend class plAudioSystemProxyDependentComponent;

  plAudioSystemDataID m_uiEntityId{0};

  plAudioSystemTransform m_LastTransform;
  plMap<plAudioSystemDataID, plAudioProxyEnvironmentAmounts, plCompareHelper<plAudioSystemDataID>, plAudioSystemAllocatorWrapper> m_mEnvironmentAmounts;
};
