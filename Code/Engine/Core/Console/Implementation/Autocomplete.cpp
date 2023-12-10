#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Core/Console/QuakeConsole.h>

void plCommandInterpreter::FindPossibleCVars(plStringView sVariable, plDeque<plString>& AutoCompleteOptions, plDeque<plConsoleString>& AutoCompleteDescriptions)
{
  plStringBuilder sText;

  plCVar* pCVar = plCVar::GetFirstInstance();
  while (pCVar)
  {
    if (pCVar->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} = {1}", pCVar->GetName(), plQuakeConsole::GetFullInfoAsString(pCVar));

      plConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = plConsoleString::Type::VarName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void plCommandInterpreter::FindPossibleFunctions(plStringView sVariable, plDeque<plString>& AutoCompleteOptions, plDeque<plConsoleString>& AutoCompleteDescriptions)
{
  plStringBuilder sText;

  plConsoleFunctionBase* pFunc = plConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (pFunc->GetName().StartsWith_NoCase(sVariable))
    {
      sText.Format("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      plConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = plConsoleString::Type::FuncName;
      AutoCompleteDescriptions.PushBack(cs);

      AutoCompleteOptions.PushBack(pFunc->GetName());
    }

    pFunc = pFunc->GetNextInstance();
  }
}


const plString plQuakeConsole::GetValueAsString(plCVar* pCVar)
{
  plStringBuilder s = "undefined";

  switch (pCVar->GetType())
  {
    case plCVarType::Int:
    {
      plCVarInt* pInt = static_cast<plCVarInt*>(pCVar);
      s.Format("{0}", pInt->GetValue());
    }
    break;

    case plCVarType::Bool:
    {
      plCVarBool* pBool = static_cast<plCVarBool*>(pCVar);
      if (pBool->GetValue() == true)
        s = "true";
      else
        s = "false";
    }
    break;

    case plCVarType::String:
    {
      plCVarString* pString = static_cast<plCVarString*>(pCVar);
      s.Format("\"{0}\"", pString->GetValue());
    }
    break;

    case plCVarType::Float:
    {
      plCVarFloat* pFloat = static_cast<plCVarFloat*>(pCVar);
      s.Format("{0}", plArgF(pFloat->GetValue(), 3));
    }
    break;

    case plCVarType::ENUM_COUNT:
      break;
  }

  return s.GetData();
}

plString plQuakeConsole::GetFullInfoAsString(plCVar* pCVar)
{
  plStringBuilder s = GetValueAsString(pCVar);

  const bool bAnyFlags = pCVar->GetFlags().IsAnySet(plCVarFlags::RequiresRestart | plCVarFlags::Save);

  if (bAnyFlags)
    s.Append(" [ ");

  if (pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
    s.Append("SAVE ");

  if (pCVar->GetFlags().IsAnySet(plCVarFlags::RequiresRestart))
    s.Append("RESTART ");

  if (bAnyFlags)
    s.Append("]");

  return s;
}

const plString plCommandInterpreter::FindCommonString(const plDeque<plString>& vStrings)
{
  plStringBuilder sCommon;
  plUInt32 c;

  plUInt32 uiPos = 0;
  auto it1 = vStrings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)vStrings.GetCount(); v++)
    {
      auto it2 = vStrings[v].GetIteratorFront();

      it2 += uiPos;

      if (it2.GetCharacter() != c)
        return sCommon;
    }

    sCommon.Append(c);

    ++uiPos;
    ++it1;
  }

  return sCommon;
}

void plCommandInterpreter::AutoComplete(plCommandInterpreterState& inout_State)
{
  plString sVarName = inout_State.m_sInput;

  auto it = rbegin(inout_State.m_sInput);

  // dots are allowed in CVar names
  while (it.IsValid() && (it.GetCharacter() == '.' || !plStringUtils::IsIdentifierDelimiter_C_Code(*it)))
    ++it;

  const char* szLastWordDelimiter = nullptr;
  if (it.IsValid() && plStringUtils::IsIdentifierDelimiter_C_Code(*it) && it.GetCharacter() != '.')
    szLastWordDelimiter = it.GetData();

  if (szLastWordDelimiter != nullptr)
    sVarName = szLastWordDelimiter + 1;

  plDeque<plString> AutoCompleteOptions;
  plDeque<plConsoleString> AutoCompleteDescriptions;

  FindPossibleCVars(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);
  FindPossibleFunctions(sVarName.GetData(), AutoCompleteOptions, AutoCompleteDescriptions);

  if (AutoCompleteDescriptions.GetCount() > 1)
  {
    AutoCompleteDescriptions.Sort();

    inout_State.AddOutputLine("");

    for (plUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_State.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_State.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_State.m_sInput = plStringView(inout_State.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_State.m_sInput.Clear();

    inout_State.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}


PLASMA_STATICLINK_FILE(Core, Core_Console_Implementation_Autocomplete);
