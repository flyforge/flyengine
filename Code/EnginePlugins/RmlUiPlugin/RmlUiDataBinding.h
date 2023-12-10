#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class PLASMA_RMLUIPLUGIN_DLL plRmlUiDataBinding
{
public:
  virtual ~plRmlUiDataBinding() {}

  virtual plResult Initialize(Rml::Context& context) = 0;
  virtual void Deinitialize(Rml::Context& context) = 0;

  virtual void Update() = 0;
};
