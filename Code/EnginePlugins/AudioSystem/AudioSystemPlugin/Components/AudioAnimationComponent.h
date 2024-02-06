#pragma once

#include <AudioSystemPlugin/AudioSystemPluginDLL.h>

#include <AudioSystemPlugin/Components/AudioSystemComponent.h>

#include <Core/World/EventMessageHandlerComponent.h>

struct plMsgGenericEvent;
struct plMsgAnimationPoseUpdated;

typedef plAudioSystemComponentManager<class plAudioAnimationComponent> plAudioAnimationComponentManager;

/// \brief A single entry in the audio animation component.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioAnimationEntry : public plReflectedClass
{
  PL_ADD_DYNAMIC_REFLECTION(plAudioAnimationEntry, plReflectedClass);

public:
  plAudioAnimationEntry();

  /// \brief Gets the name of the skeleton joint from which collect transformation data
  /// when activating the audio trigger.
  /// \param szName The name of the skeleton joint.
  void SetJointName(const char* szName);

  /// \returns The name of the skeleton joint from which collect transformation data.
  [[nodiscard]] const char* GetJointName() const;

  /// \brief The animation event to listen.
  plString m_sEventName;

  /// \brief The audio trigger to activate when the animation
  /// event is triggered.
  plString m_sTriggerName;

private:
  friend class plAudioAnimationComponent;

  void ActivateTrigger() const;
  void Initialize(bool bSync);
  void UnloadTrigger();

  /// \brief (Optional) The name of the joint in the skeleton asset from which
  /// to copy position and orientation data. If not specified, the owner's transformation
  /// is used instead.
  plHashedString m_sJointName;

  plAudioSystemDataID m_uiEntityId;
  plAudioSystemDataID m_uiEventId;
  plUInt16 m_uiJointIndex;
  bool m_bTriggerLoaded;

  plAudioSystemTransform m_LastTransform;
};

/// \brief Component that can be used to activate an audio trigger when animation events got triggered.
class PL_AUDIOSYSTEMPLUGIN_DLL plAudioAnimationComponent : public plEventMessageHandlerComponent
{
  PL_DECLARE_COMPONENT_TYPE(plAudioAnimationComponent, plEventMessageHandlerComponent, plAudioAnimationComponentManager);

  // plComponent

public:
  void Initialize() override;
  void Deinitialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAudioAnimationComponent

protected:
  void Update();
  void OnAnimationPoseUpdated(plMsgAnimationPoseUpdated& msg);
  void OnAnimationEvent(plMsgGenericEvent& msg) const;

private:
  plDynamicArray<plAudioAnimationEntry> m_EventEntries;
};
