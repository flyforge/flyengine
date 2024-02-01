#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

class PL_RMLUIPLUGIN_DLL plRmlUiDataBinding
{
public:
  virtual ~plRmlUiDataBinding() = default;

  virtual plResult Initialize(Rml::Context& ref_context) = 0;
  virtual void Deinitialize(Rml::Context& ref_context) = 0;

  virtual void Update() = 0;
};
