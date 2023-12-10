#pragma once

#include <Core/World/Component.h>
#include <Core/World/World.h>
#include <RecastPlugin/RecastPluginDLL.h>

/// \brief Base class for all components that implement 'non player character' behavior.
/// Ie, game logic for how an AI controlled character should behave (state machines etc)
class PLASMA_RECASTPLUGIN_DLL plNpcComponent : public plComponent
{
  PLASMA_DECLARE_ABSTRACT_COMPONENT_TYPE(plNpcComponent, plComponent);

  //////////////////////////////////////////////////////////////////////////
  // plComponent

public:
  virtual void SerializeComponent(plWorldWriter& inout_stream) const override;
  virtual void DeserializeComponent(plWorldReader& inout_stream) override;

  //////////////////////////////////////////////////////////////////////////
  // plNpcComponent

public:
  plNpcComponent();
  ~plNpcComponent();
};
