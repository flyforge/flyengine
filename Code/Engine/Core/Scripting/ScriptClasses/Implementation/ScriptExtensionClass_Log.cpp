#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
PLASMA_BEGIN_STATIC_REFLECTED_TYPE(plScriptExtensionClass_Log, plNoBase, 1, plRTTINoAllocator)
{
  PLASMA_BEGIN_FUNCTIONS
  {
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(SeriousWarning, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Success, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Dev, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PLASMA_SCRIPT_FUNCTION_PROPERTY(Debug, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
  }
  PLASMA_END_FUNCTIONS;
  PLASMA_BEGIN_ATTRIBUTES
  {
    new plScriptExtensionAttribute("Log"),
  }
  PLASMA_END_ATTRIBUTES;
}
PLASMA_END_STATIC_REFLECTED_TYPE;
// clang-format on

static plStringView BuildFormattedText(const char* szText, const plVariantArray& params, plStringBuilder& ref_sStorage)
{
  plHybridArray<plString, 12> stringStorage;
  stringStorage.Reserve(params.GetCount());
  for (auto& param : params)
  {
    stringStorage.PushBack(param.ConvertTo<plString>());
  }

  plHybridArray<plStringView, 12> stringViews;
  stringViews.Reserve(stringStorage.GetCount());
  for (auto& s : stringStorage)
  {
    stringViews.PushBack(s);
  }

  plFormatString fs(szText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void plScriptExtensionClass_Log::Error(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Error(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::SeriousWarning(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::SeriousWarning(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::Warning(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Warning(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::Success(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Success(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::Info(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Info(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::Dev(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Dev(BuildFormattedText(szText, params, sStorage));
}

void plScriptExtensionClass_Log::Debug(const char* szText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Debug(BuildFormattedText(szText, params, sStorage));
}

