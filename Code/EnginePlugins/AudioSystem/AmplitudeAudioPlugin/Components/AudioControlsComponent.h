#pragma once

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

#include <AmplitudeAudioPlugin/Components/AmplitudeComponent.h>
#include <AmplitudeAudioPlugin/Resources/AudioControlCollectionResource.h>
#include <AudioSystemPlugin/Core/AudioSystemData.h>

typedef plComponentManager<class plAudioControlsComponent, plBlockStorageType::FreeList> plAudioControlsComponentManager;

/// \brief Component used to load and unload a set of audio controls.
///
/// The audio controls are provided by the selected audio control collection. Loaded audio
/// controls can be automatically registered to the audio system at component activation.
class PLASMA_AMPLITUDEAUDIOPLUGIN_DLL plAudioControlsComponent : public plAmplitudeComponent
{
  PLASMA_DECLARE_COMPONENT_TYPE(plAudioControlsComponent, plAmplitudeComponent, plAudioControlsComponentManager);

  // plComponent

public:
  void Initialize() override;
  void Deinitialize() override;
  void SerializeComponent(plWorldWriter& stream) const override;
  void DeserializeComponent(plWorldReader& stream) override;

  // plAmplitudeComponent

private:
  void plAmplitudeComponentIsAbstract() override {}

  // plAudioControlsComponent

public:
  plAudioControlsComponent();
  ~plAudioControlsComponent() override;

  /// \brief Load the audio controls from the given collection.
  /// This is automatically called on component initialization when
  /// the AutoLoad property is set to true.
  bool Load();

  /// \brief Unloads the audio controls.
  bool Unload();

private:
  plString m_sControlsAsset;
  bool m_bAutoLoad;

  bool m_bLoaded;
  plAudioControlCollectionResourceHandle m_hControlsResource;
};
