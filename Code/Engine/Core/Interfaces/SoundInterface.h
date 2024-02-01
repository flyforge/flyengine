#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Basics.h>

class plSoundInterface
{
public:
  /// \brief Can be called before startup to load the configs from a different file.
  /// Otherwise will automatically be loaded by the sound system startup with the default path.
  virtual void LoadConfiguration(plStringView sFile) = 0;

  /// \brief By default the integration should auto-detect the platform (and thus the config) to use.
  /// Calling this before startup allows to override which configuration is used.
  virtual void SetOverridePlatform(plStringView sPlatform) = 0;

  /// \brief Has to be called once per frame to update all sounds
  virtual void UpdateSound() = 0;

  /// \brief Adjusts the master volume. This affects all sounds, with no exception. Value must be between 0.0f and 1.0f.
  virtual void SetMasterChannelVolume(float fVolume) = 0;
  virtual float GetMasterChannelVolume() const = 0;

  /// \brief Allows to mute all sounds. Useful for when the application goes to a background state.
  virtual void SetMasterChannelMute(bool bMute) = 0;
  virtual bool GetMasterChannelMute() const = 0;

  /// \brief Allows to pause all sounds. Useful for when the application goes to a background state and you want to pause all sounds, instead of mute
  /// them.
  virtual void SetMasterChannelPaused(bool bPaused) = 0;
  virtual bool GetMasterChannelPaused() const = 0;

  /// \brief Specifies the volume for a VCA ('Voltage Control Amplifier').
  ///
  /// This is used to control the volume of high level sound groups, such as 'Effects', 'Music', 'Ambiance' or 'Speech'.
  /// Note that the Fmod strings banks are never loaded, so the given string must be a GUID (Fmod Studio -> Copy GUID).
  virtual void SetSoundGroupVolume(plStringView sVcaGroupGuid, float fVolume) = 0;
  virtual float GetSoundGroupVolume(plStringView sVcaGroupGuid) const = 0;

  /// \brief Default is 1. Allows to set how many virtual listeners the sound is mixed for (split screen game play).
  virtual void SetNumListeners(plUInt8 uiNumListeners) = 0;
  virtual plUInt8 GetNumListeners() = 0;

  /// \brief The editor activates this to ignore the listener positions from the listener components, and instead use the editor camera as the
  /// listener position.
  virtual void SetListenerOverrideMode(bool bEnabled) = 0;

  /// \brief Sets the position for listener N. Index -1 is used for the override mode listener.
  virtual void SetListener(plInt32 iIndex, const plVec3& vPosition, const plVec3& vForward, const plVec3& vUp, const plVec3& vVelocity) = 0;

  /// \brief Plays a sound once. Callced by plSoundInterface::PlaySound().
  virtual plResult OneShotSound(plStringView sResourceID, const plTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true) = 0;

  /// \brief Plays a sound once.
  ///
  /// Convenience function to call OneShotSound() without having to retrieve the plSoundInterface first.
  ///
  /// Which sound to play is specified through a resource ID ('Asset GUID').
  /// This is not the most efficient way to load a sound, as there is no way to preload the resource.
  /// If preloading is desired, you need to access the implementation-specific resource type directly (e.g. plFmodSoundEventResource).
  /// Also see plFmodSoundEventResource::PlayOnce().
  /// In practice, though, sounds are typically loaded in bulk from sound-banks, and preloading is not necessary.
  ///
  /// Be aware that this does not allow to adjust volume, pitch or position after creation. Stopping is also not possible.
  /// Use a sound component, if that is necessary.
  ///
  /// Also by default a pitch of 1 is always used. If the game speed is not 1 (plWorld clock), a custom pitch would need to be provided,
  /// if the sound should play at the same speed.
  PL_CORE_DLL static plResult PlaySound(plStringView sResourceID, const plTransform& globalPosition, float fPitch = 1.0f, float fVolume = 1.0f, bool bBlockIfNotLoaded = true);
};
