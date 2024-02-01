#include <Core/CorePCH.h>

#include <Core/Console/Console.h>
#include <Core/Console/QuakeConsole.h>

void plCommandInterpreter::FindPossibleCVars(plStringView sVariable, plDeque<plString>& inout_autoCompleteOptions, plDeque<plConsoleString>& inout_autoCompleteDescriptions)
{
  plStringBuilder sText;

  plCVar* pCVar = plCVar::GetFirstInstance();
  while (pCVar)
  {
    if (pCVar->GetName().StartsWith_NoCase(sVariable))
    {
      sText.SetFormat("    {0} = {1}", pCVar->GetName(), plQuakeConsole::GetFullInfoAsString(pCVar));

      plConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = plConsoleString::Type::VarName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pCVar->GetName());
    }

    pCVar = pCVar->GetNextInstance();
  }
}

void plCommandInterpreter::FindPossibleFunctions(plStringView sVariable, plDeque<plString>& inout_autoCompleteOptions, plDeque<plConsoleString>& inout_autoCompleteDescriptions)
{
  plStringBuilder sText;

  plConsoleFunctionBase* pFunc = plConsoleFunctionBase::GetFirstInstance();
  while (pFunc)
  {
    if (pFunc->GetName().StartsWith_NoCase(sVariable))
    {
      sText.SetFormat("    {0} {1}", pFunc->GetName(), pFunc->GetDescription());

      plConsoleString cs;
      cs.m_sText = sText;
      cs.m_Type = plConsoleString::Type::FuncName;
      inout_autoCompleteDescriptions.PushBack(cs);

      inout_autoCompleteOptions.PushBack(pFunc->GetName());
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
      s.SetFormat("{0}", pInt->GetValue());
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
      s.SetFormat("\"{0}\"", pString->GetValue());
    }
    break;

    case plCVarType::Float:
    {
      plCVarFloat* pFloat = static_cast<plCVarFloat*>(pCVar);
      s.SetFormat("{0}", plArgF(pFloat->GetValue(), 3));
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

  const bool bAnyFlags = pCVar->GetFlags().IsAnySet(plCVarFlags::Save | plCVarFlags::ShowRequiresRestartMsg);

  if (bAnyFlags)
    s.Append(" [ ");

  if (pCVar->GetFlags().IsAnySet(plCVarFlags::Save))
    s.Append("SAVE ");

  if (pCVar->GetFlags().IsAnySet(plCVarFlags::ShowRequiresRestartMsg))
    s.Append("RESTART ");

  if (bAnyFlags)
    s.Append("]");

  return s;
}

const plString plCommandInterpreter::FindCommonString(const plDeque<plString>& strings)
{
  plStringBuilder sCommon;
  plUInt32 c;

  plUInt32 uiPos = 0;
  auto it1 = strings[0].GetIteratorFront();
  while (it1.IsValid())
  {
    c = it1.GetCharacter();

    for (int v = 1; v < (int)strings.GetCount(); v++)
    {
      auto it2 = strings[v].GetIteratorFront();

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

void plCommandInterpreter::AutoComplete(plCommandInterpreterState& inout_state)
{
  plString sVarName = inout_state.m_sInput;

  auto it = rbegin(inout_state.m_sInput);

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

    inout_state.AddOutputLine("");

    for (plUInt32 i = 0; i < AutoCompleteDescriptions.GetCount(); i++)
    {
      inout_state.AddOutputLine(AutoCompleteDescriptions[i].m_sText.GetData(), AutoCompleteDescriptions[i].m_Type);
    }

    inout_state.AddOutputLine("");
  }

  if (AutoCompleteOptions.GetCount() > 0)
  {
    if (szLastWordDelimiter != nullptr)
      inout_state.m_sInput = plStringView(inout_state.m_sInput.GetData(), szLastWordDelimiter + 1);
    else
      inout_state.m_sInput.Clear();

    inout_state.m_sInput.Append(FindCommonString(AutoCompleteOptions).GetData());
  }
}
