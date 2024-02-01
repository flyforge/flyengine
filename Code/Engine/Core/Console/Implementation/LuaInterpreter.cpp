#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Core/Scripting/LuaWrapper.h>

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT

static void AllowScriptCVarAccess(plLuaWrapper& ref_script);

static const plString GetNextWord(plStringView& ref_sString)
{
  const char* szStartWord = plStringUtils::SkipCharacters(ref_sString.GetStartPointer(), plStringUtils::IsWhiteSpace, false);
  const char* szEndWord = plStringUtils::FindWordEnd(szStartWord, plStringUtils::IsIdentifierDelimiter_C_Code, true);

  ref_sString = plStringView(szEndWord);

  return plStringView(szStartWord, szEndWord);
}

static plString GetRestWords(plStringView sString)
{
  return plStringUtils::SkipCharacters(sString.GetStartPointer(), plStringUtils::IsWhiteSpace, false);
}

static int LUAFUNC_ConsoleFunc(lua_State* pState)
{
  plLuaWrapper s(pState);

  plConsoleFunctionBase* pFunc = (plConsoleFunctionBase*)s.GetFunctionLightUserData();

  if (pFunc->GetNumParameters() != s.GetNumberOfFunctionParameters())
  {
    plLog::Error("Function '{0}' expects {1} parameters, {2} were provided.", pFunc->GetName(), pFunc->GetNumParameters(), s.GetNumberOfFunctionParameters());
    return s.ReturnToScript();
  }

  plHybridArray<plVariant, 8> m_Params;
  m_Params.SetCount(pFunc->GetNumParameters());

  for (plUInt32 p = 0; p < pFunc->GetNumParameters(); ++p)
  {
    switch (pFunc->GetParameterType(p))
    {
      case plVariant::Type::Bool:
        m_Params[p] = s.GetBoolParameter(p);
        break;
      case plVariant::Type::Int8:
      case plVariant::Type::Int16:
      case plVariant::Type::Int32:
      case plVariant::Type::Int64:
      case plVariant::Type::UInt8:
      case plVariant::Type::UInt16:
      case plVariant::Type::UInt32:
      case plVariant::Type::UInt64:
        m_Params[p] = s.GetIntParameter(p);
        break;
      case plVariant::Type::Float:
      case plVariant::Type::Double:
        m_Params[p] = s.GetFloatParameter(p);
        break;
      case plVariant::Type::String:
        m_Params[p] = s.GetStringParameter(p);
        break;
      default:
        plLog::Error("Function '{0}': Type of parameter {1} is not supported by the Lua interpreter.", pFunc->GetName(), p);
        return s.ReturnToScript();
    }
  }

  if (!m_Params.IsEmpty())
    pFunc->Call(plArrayPtr<plVariant>(&m_Params[0], m_Params.GetCount())).IgnoreResult();
  else
    pFunc->Call(plArrayPtr<plVariant>()).IgnoreResult();

  return s.ReturnToScript();
}

static void SanitizeCVarNames(plStringBuilder& ref_sCommand)
{
  plStringBuilder sanitizedCVarName;

  for (const plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    ref_sCommand.ReplaceAll(pCVar->GetName(), sanitizedCVarName);
  }
}

static void UnSanitizeCVarName(plStringBuilder& ref_sCvarName)
{
  plStringBuilder sanitizedCVarName;

  for (const plCVar* pCVar = plCVar::GetFirstInstance(); pCVar != nullptr; pCVar = pCVar->GetNextInstance())
  {
    sanitizedCVarName = pCVar->GetName();
    sanitizedCVarName.ReplaceAll(".", "_");

    if (ref_sCvarName == sanitizedCVarName)
    {
      ref_sCvarName = pCVar->GetName();
      return;
    }
  }
}

void plCommandInterpreterLua::Interpret(plCommandInterpreterState& inout_state)
{
  inout_state.m_sOutput.Clear();

  plStringBuilder sRealCommand = inout_state.m_sInput;

  if (sRealCommand.IsEmpty())
  {
    inout_state.AddOutputLine("");
    return;
  }

  sRealCommand.Trim(" \t\n\r");
  plStringBuilder sSanitizedCommand = sRealCommand;
  SanitizeCVarNames(sSanitizedCommand);

  plStringView sCommandIt = sSanitizedCommand;

  const plString sSanitizedVarName = GetNextWord(sCommandIt);
  plStringBuilder sRealVarName = sSanitizedVarName;
  UnSanitizeCVarName(sRealVarName);

  while (plStringUtils::IsWhiteSpace(sCommandIt.GetCharacter()))
  {
    sCommandIt.Shrink(1, 0);
  }

  const bool bSetValue = sCommandIt.StartsWith("=");

  if (bSetValue)
  {
    sCommandIt.Shrink(1, 0);
  }

  plStringBuilder sValue = GetRestWords(sCommandIt);
  bool bValueEmpty = sValue.IsEmpty();

  plStringBuilder sTemp;

  plLuaWrapper Script;
  AllowScriptCVarAccess(Script);

  // Register all ConsoleFunctions
  {
    plConsoleFunctionBase* pFunc = plConsoleFunctionBase::GetFirstInstance();
    while (pFunc)
    {
      Script.RegisterCFunction(pFunc->GetName().GetData(sTemp), LUAFUNC_ConsoleFunc, pFunc);

      pFunc = pFunc->GetNextInstance();
    }
  }

  sTemp = "> ";
  sTemp.Append(sRealCommand);
  inout_state.AddOutputLine(sTemp, plConsoleString::Type::Executed);

  plCVar* pCVAR = plCVar::FindCVarByName(sRealVarName.GetData());
  if (pCVAR != nullptr)
  {
    if ((bSetValue) && (sValue == "") && (pCVAR->GetType() == plCVarType::Bool))
    {
      // someone typed "myvar =" -> on bools this is the short form for "myvar = not myvar" (toggle), so insert the rest here

      bValueEmpty = false;

      sSanitizedCommand.AppendFormat(" not {0}", sSanitizedVarName);
    }

    if (bSetValue && !bValueEmpty)
    {
      plMuteLog muteLog;

      if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
      {
        inout_state.AddOutputLine("  Error Executing Command.", plConsoleString::Type::Error);
        return;
      }
      else
      {
        if (pCVAR->GetFlags().IsAnySet(plCVarFlags::ShowRequiresRestartMsg))
        {
          inout_state.AddOutputLine("  This change takes only effect after a restart.", plConsoleString::Type::Note);
        }

        sTemp.SetFormat("  {0} = {1}", sRealVarName, plQuakeConsole::GetFullInfoAsString(pCVAR));
        inout_state.AddOutputLine(sTemp, plConsoleString::Type::Success);
      }
    }
    else
    {
      sTemp.SetFormat("{0} = {1}", sRealVarName, plQuakeConsole::GetFullInfoAsString(pCVAR));
      inout_state.AddOutputLine(sTemp);

      if (!pCVAR->GetDescription().IsEmpty())
      {
        sTemp.SetFormat("  Description: {0}", pCVAR->GetDescription());
        inout_state.AddOutputLine(sTemp, plConsoleString::Type::Success);
      }
      else
        inout_state.AddOutputLine("  No Description available.", plConsoleString::Type::Success);
    }

    return;
  }
  else
  {
    plMuteLog muteLog;

    if (Script.ExecuteString(sSanitizedCommand, "console", &muteLog).Failed())
    {
      inout_state.AddOutputLine("  Error Executing Command.", plConsoleString::Type::Error);
      return;
    }
  }
}

static int LUAFUNC_ReadCVAR(lua_State* pState)
{
  plLuaWrapper s(pState);

  plStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  plCVar* pCVar = plCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValueNil();
    return s.ReturnToScript();
  }

  switch (pCVar->GetType())
  {
    case plCVarType::Int:
    {
      plCVarInt* pVar = (plCVarInt*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case plCVarType::Bool:
    {
      plCVarBool* pVar = (plCVarBool*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case plCVarType::Float:
    {
      plCVarFloat* pVar = (plCVarFloat*)pCVar;
      s.PushReturnValue(pVar->GetValue());
    }
    break;
    case plCVarType::String:
    {
      plCVarString* pVar = (plCVarString*)pCVar;
      s.PushReturnValue(pVar->GetValue().GetData());
    }
    break;
    case plCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}


static int LUAFUNC_WriteCVAR(lua_State* pState)
{
  plLuaWrapper s(pState);

  plStringBuilder cvarName = s.GetStringParameter(0);
  UnSanitizeCVarName(cvarName);

  plCVar* pCVar = plCVar::FindCVarByName(cvarName);

  if (pCVar == nullptr)
  {
    s.PushReturnValue(false);
    return s.ReturnToScript();
  }

  s.PushReturnValue(true);

  switch (pCVar->GetType())
  {
    case plCVarType::Int:
    {
      plCVarInt* pVar = (plCVarInt*)pCVar;
      *pVar = s.GetIntParameter(1);
    }
    break;
    case plCVarType::Bool:
    {
      plCVarBool* pVar = (plCVarBool*)pCVar;
      *pVar = s.GetBoolParameter(1);
    }
    break;
    case plCVarType::Float:
    {
      plCVarFloat* pVar = (plCVarFloat*)pCVar;
      *pVar = s.GetFloatParameter(1);
    }
    break;
    case plCVarType::String:
    {
      plCVarString* pVar = (plCVarString*)pCVar;
      *pVar = s.GetStringParameter(1);
    }
    break;
    case plCVarType::ENUM_COUNT:
      break;
  }

  return s.ReturnToScript();
}

static void AllowScriptCVarAccess(plLuaWrapper& ref_script)
{
  ref_script.RegisterCFunction("ReadCVar", LUAFUNC_ReadCVAR);
  ref_script.RegisterCFunction("WriteCVar", LUAFUNC_WriteCVAR);

  plStringBuilder sInit = "\
function readcvar (t, key)\n\
return (ReadCVar (key))\n\
end\n\
\n\
function writecvar (t, key, value)\n\
if not WriteCVar (key, value) then\n\
rawset (t, key, value or false)\n\
end\n\
end\n\
\n\
setmetatable (_G, {\n\
__newindex = writecvar,\n\
__index = readcvar,\n\
__metatable = \"Access Denied\",\n\
})";

  ref_script.ExecuteString(sInit.GetData()).IgnoreResult();
}

#endif // BUILDSYSTEM_ENABLE_LUA_SUPPORT


