#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>

#include <AmplitudeAudioPlugin/AmplitudeAudioPluginDLL.h>

/// \brief Base class for all Amplitude components, such that they all have a common ancestor
class PL_AMPLITUDEAUDIOPLUGIN_DLL plAmplitudeComponent : public plComponent
{
  PL_DECLARE_ABSTRACT_COMPONENT_TYPE(plAmplitudeComponent, plComponent);

  // plAmplitudeComponent

private:
  // Dummy method to hide this component in the editor UI.
  virtual void plAmplitudeComponentIsAbstract() = 0;
};
