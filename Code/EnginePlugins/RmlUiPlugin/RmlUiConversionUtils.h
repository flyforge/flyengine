#pragma once

#include <RmlUiPlugin/RmlUiPluginDLL.h>

#include <RmlUi/Include/RmlUi/Core.h>

namespace plRmlUiConversionUtils
{
  PLASMA_RMLUIPLUGIN_DLL plVariant ToVariant(const Rml::Variant& value, plVariant::Type::Enum targetType = plVariant::Type::Invalid);
  PLASMA_RMLUIPLUGIN_DLL Rml::Variant ToVariant(const plVariant& value);
} // namespace plRmlUiConversionUtils
