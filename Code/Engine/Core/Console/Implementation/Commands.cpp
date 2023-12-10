#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Foundation/Configuration/CVar.h>

void plQuakeConsole::ExecuteCommand(plStringView input)
{
  const bool bBind = input.StartsWith_NoCase("bind ");
  const bool bUnbind = input.StartsWith_NoCase("unbind ");

  if (bBind || bUnbind)
  {
    plStringBuilder tmp;
    const char* szAfterCmd = plStringUtils::FindWordEnd(input.GetData(tmp), plStringUtils::IsWhiteSpace); // skip the word 'bind' or 'unbind'

    const char* szKeyNameStart = plStringUtils::SkipCharacters(szAfterCmd, plStringUtils::IsWhiteSpace);                // go to the next word
    const char* szKeyNameEnd = plStringUtils::FindWordEnd(szKeyNameStart, plStringUtils::IsIdentifierDelimiter_C_Code); // find its end

    plStringView sKey(szKeyNameStart, szKeyNameEnd);
    tmp = sKey; // copy the word into a zero terminated string

    const char* szCommandToBind = plStringUtils::SkipCharacters(szKeyNameEnd, plStringUtils::IsWhiteSpace);

    if (bUnbind || plStringUtils::IsNullOrEmpty(szCommandToBind))
    {
      UnbindKey(tmp);
      return;
    }

    BindKey(tmp, szCommandToBind);
    return;
  }

  plConsole::ExecuteCommand(input);
}

void plQuakeConsole::BindKey(plStringView sKey, plStringView sCommand)
{
  plStringBuilder s;
  s.Format("Binding key '{0}' to command '{1}'", sKey, sCommand);
  AddConsoleString(s, plConsoleString::Type::Success);

  m_BoundKeys[sKey] = sCommand;
}

void plQuakeConsole::UnbindKey(plStringView sKey)
{
  plStringBuilder s;
  s.Format("Unbinding key '{0}'", sKey);
  AddConsoleString(s, plConsoleString::Type::Success);

  m_BoundKeys.Remove(sKey);
}

void plQuakeConsole::ExecuteBoundKey(plStringView sKey)
{
  auto it = m_BoundKeys.Find(sKey);

  if (it.IsValid())
  {
    ExecuteCommand(it.Value().GetData());
  }
}



PLASMA_STATICLINK_FILE(Core, Core_Console_Implementation_Commands);
