#pragma once

#include <Core/CoreDLL.h>
#include <Foundation/Reflection/Reflection.h>

class PLASMA_CORE_DLL plScriptExtensionClass_Log
{
public:
  static void Error(const char* szText, const plVariantArray& params);
  static void SeriousWarning(const char* szText, const plVariantArray& params);
  static void Warning(const char* szText, const plVariantArray& params);
  static void Success(const char* szText, const plVariantArray& params);
  static void Info(const char* szText, const plVariantArray& params);
  static void Dev(const char* szText, const plVariantArray& params);
  static void Debug(const char* szText, const plVariantArray& params);

};

PLASMA_DECLARE_REFLECTABLE_TYPE(PLASMA_CORE_DLL, plScriptExtensionClass_Log);
