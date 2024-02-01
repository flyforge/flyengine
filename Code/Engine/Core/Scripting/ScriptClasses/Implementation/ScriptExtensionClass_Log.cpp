#include <Core/CorePCH.h>

#include <Core/Scripting/ScriptAttributes.h>
#include <Core/Scripting/ScriptClasses/ScriptExtensionClass_Log.h>

// clang-format off
PL_BEGIN_STATIC_REFLECTED_TYPE(plScriptExtensionClass_Log, plNoBase, 1, plRTTINoAllocator)
{
  PL_BEGIN_FUNCTIONS
  {
    PL_SCRIPT_FUNCTION_PROPERTY(Info, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PL_SCRIPT_FUNCTION_PROPERTY(Warning, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
    PL_SCRIPT_FUNCTION_PROPERTY(Error, In, "Text", In, "Params")->AddAttributes(new plDynamicPinAttribute("Params")),
  }
  PL_END_FUNCTIONS;
  PL_BEGIN_ATTRIBUTES
  {
    new plScriptExtensionAttribute("Log"),
  }
  PL_END_ATTRIBUTES;
}
PL_END_STATIC_REFLECTED_TYPE;
// clang-format on

static plStringView BuildFormattedText(plStringView sText, const plVariantArray& params, plStringBuilder& ref_sStorage)
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

  plFormatString fs(sText);
  return fs.BuildFormattedText(ref_sStorage, stringViews.GetData(), stringViews.GetCount());
}

// static
void plScriptExtensionClass_Log::Info(plStringView sText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Info(BuildFormattedText(sText, params, sStorage));
}

// static
void plScriptExtensionClass_Log::Warning(plStringView sText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Warning(BuildFormattedText(sText, params, sStorage));
}

// static
void plScriptExtensionClass_Log::Error(plStringView sText, const plVariantArray& params)
{
  plStringBuilder sStorage;
  plLog::Error(BuildFormattedText(sText, params, sStorage));
}


PL_STATICLINK_FILE(Core, Core_Scripting_ScriptClasses_Implementation_ScriptExtensionClass_Log);

