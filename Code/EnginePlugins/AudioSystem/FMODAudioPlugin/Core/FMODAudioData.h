#pragma once

#include "Foundation/IO/Stream.h"
#include <FMODAudioPlugin/FMODAudioPluginDLL.h>

#include <AudioSystemPlugin/Core/AudioSystemData.h>

/// \brief The type of a control. This is used by control assets to determine the type of the control
/// when the audio system is parsing them.
struct PLASMA_FMODAUDIOPLUGIN_DLL plFMODAudioControlType
{
  using StorageType = plUInt8;

  enum Enum : StorageType
  {
    /// \brief The control is not known to the audio system.
    Invalid = 0,

    /// \brief The control is a source.
    Trigger = 1,

    /// \brief The control is a real-time parameter.
    Rtpc = 2,

    /// \brief The control is a sound bank.
    SoundBank = 3,

    /// \brief The control is a switch container.
    Switch = 4,

    /// \brief The control is a switch state.
    SwitchState = 5,

    /// \brief The control is an environment effect.
    Environment = 6,

    Default = Invalid,
  };
};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_FMODAUDIOPLUGIN_DLL, plFMODAudioControlType);