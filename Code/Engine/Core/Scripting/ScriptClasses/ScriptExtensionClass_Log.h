#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class PL_CORE_DLL plScriptExtensionClass_Log
{
public:
  static void Info(plStringView sText, const plVariantArray& params);
  static void Warning(plStringView sText, const plVariantArray& params);
  static void Error(plStringView sText, const plVariantArray& params);
};

PL_DECLARE_REFLECTABLE_TYPE(PL_CORE_DLL, plScriptExtensionClass_Log);
