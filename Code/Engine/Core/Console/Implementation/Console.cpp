#include <Core/CorePCH.h>

#include <Core/Console/LuaInterpreter.h>
#include <Core/Console/QuakeConsole.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>

PL_ENUMERABLE_CLASS_IMPLEMENTATION(plConsoleFunctionBase);

plQuakeConsole::plQuakeConsole()
{
  ClearInputLine();

  m_bLogOutputEnabled = false;
  m_bDefaultInputHandlingInitialized = false;
  m_uiMaxConsoleStrings = 1000;

  EnableLogOutput(true);

#ifdef BUILDSYSTEM_ENABLE_LUA_SUPPORT
  SetCommandInterpreter(PL_DEFAULT_NEW(plCommandInterpreterLua));
#endif
}

plQuakeConsole::~plQuakeConsole()
{
  EnableLogOutput(false);
}

void plQuakeConsole::AddConsoleString(plStringView sText, plConsoleString::Type type)
{
  PL_LOCK(m_Mutex);

  m_ConsoleStrings.PushFront();

  plConsoleString& cs = m_ConsoleStrings.PeekFront();
  cs.m_sText = sText;
  cs.m_Type = type;

  if (m_ConsoleStrings.GetCount() > m_uiMaxConsoleStrings)
    m_ConsoleStrings.PopBack(m_ConsoleStrings.GetCount() - m_uiMaxConsoleStrings);

  plConsole::AddConsoleString(sText, type);
}

const plDeque<plConsoleString>& plQuakeConsole::GetConsoleStrings() const
{
  if (m_bUseFilteredStrings)
  {
    return m_FilteredConsoleStrings;
  }

  return m_ConsoleStrings;
}

void plQuakeConsole::LogHandler(const plLoggingEventData& data)
{
  plConsoleString::Type type = plConsoleString::Type::Default;

  switch (data.m_EventType)
  {
    case plLogMsgType::GlobalDefault:
    case plLogMsgType::Flush:
    case plLogMsgType::BeginGroup:
    case plLogMsgType::EndGroup:
    case plLogMsgType::None:
    case plLogMsgType::ENUM_COUNT:
    case plLogMsgType::All:
      return;

    case plLogMsgType::ErrorMsg:
      type = plConsoleString::Type::Error;
      break;

    case plLogMsgType::SeriousWarningMsg:
      type = plConsoleString::Type::SeriousWarning;
      break;

    case plLogMsgType::WarningMsg:
      type = plConsoleString::Type::Warning;
      break;

    case plLogMsgType::SuccessMsg:
      type = plConsoleString::Type::Success;
      break;

    case plLogMsgType::InfoMsg:
      break;

    case plLogMsgType::DevMsg:
      type = plConsoleString::Type::Dev;
      break;

    case plLogMsgType::DebugMsg:
      type = plConsoleString::Type::Debug;
      break;
  }

  plStringBuilder sFormat;
  sFormat.SetPrintf("%*s", data.m_uiIndentation, "");
  sFormat.Append(data.m_sText);

  AddConsoleString(sFormat.GetData(), type);
}

void plQuakeConsole::InputStringChanged()
{
  m_bUseFilteredStrings = false;
  m_FilteredConsoleStrings.Clear();

  if (m_sInputLine.StartsWith("*"))
  {
    plStringBuilder input = m_sInputLine;

    input.Shrink(1, 0);
    input.Trim(" ");

    if (input.IsEmpty())
      return;

    m_FilteredConsoleStrings.Clear();
    m_bUseFilteredStrings = true;

    for (const auto& e : m_ConsoleStrings)
    {
      if (e.m_sText.FindSubString_NoCase(input))
      {
        m_FilteredConsoleStrings.PushBack(e);
      }
    }

    Scroll(0); // clamp scroll position
  }
}

void plQuakeConsole::EnableLogOutput(bool bEnable)
{
  if (m_bLogOutputEnabled == bEnable)
    return;

  m_bLogOutputEnabled = bEnable;

  if (bEnable)
  {
    plGlobalLog::AddLogWriter(plMakeDelegate(&plQuakeConsole::LogHandler, this));
  }
  else
  {
    plGlobalLog::RemoveLogWriter(plMakeDelegate(&plQuakeConsole::LogHandler, this));
  }
}

void plQuakeConsole::SaveState(plStreamWriter& inout_stream) const
{
  PL_LOCK(m_Mutex);

  const plUInt8 uiVersion = 1;
  inout_stream << uiVersion;

  inout_stream << m_InputHistory.GetCount();
  for (plUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
  {
    inout_stream << m_InputHistory[i];
  }

  inout_stream << m_BoundKeys.GetCount();
  for (auto it = m_BoundKeys.GetIterator(); it.IsValid(); ++it)
  {
    inout_stream << it.Key();
    inout_stream << it.Value();
  }
}

void plQuakeConsole::LoadState(plStreamReader& inout_stream)
{
  PL_LOCK(m_Mutex);

  plUInt8 uiVersion = 0;
  inout_stream >> uiVersion;

  if (uiVersion == 1)
  {
    plUInt32 count = 0;
    inout_stream >> count;
    m_InputHistory.SetCount(count);

    for (plUInt32 i = 0; i < m_InputHistory.GetCount(); ++i)
    {
      inout_stream >> m_InputHistory[i];
    }

    inout_stream >> count;

    plString sKey;
    plString sValue;

    for (plUInt32 i = 0; i < count; ++i)
    {
      inout_stream >> sKey;
      inout_stream >> sValue;

      m_BoundKeys[sKey] = sValue;
    }
  }
}

void plCommandInterpreterState::AddOutputLine(const plFormatString& text, plConsoleString::Type type /*= plCommandOutputLine::Type::Default*/)
{
  auto& line = m_sOutput.ExpandAndGetRef();
  line.m_Type = type;

  plStringBuilder tmp;
  line.m_sText = text.GetText(tmp);
}

plColor plConsoleString::GetColor() const
{
  switch (m_Type)
  {
    case plConsoleString::Type::Default:
      return plColor::White;

    case plConsoleString::Type::Error:
      return plColor(1.0f, 0.2f, 0.2f);

    case plConsoleString::Type::SeriousWarning:
      return plColor(1.0f, 0.4f, 0.1f);

    case plConsoleString::Type::Warning:
      return plColor(1.0f, 0.6f, 0.1f);

    case plConsoleString::Type::Note:
      return plColor(1, 200.0f / 255.0f, 0);

    case plConsoleString::Type::Success:
      return plColor(0.1f, 1.0f, 0.1f);

    case plConsoleString::Type::Executed:
      return plColor(1.0f, 0.5f, 0.0f);

    case plConsoleString::Type::VarName:
      return plColorGammaUB(255, 210, 0);

    case plConsoleString::Type::FuncName:
      return plColorGammaUB(100, 255, 100);

    case plConsoleString::Type::Dev:
      return plColor(0.6f, 0.6f, 0.6f);

    case plConsoleString::Type::Debug:
      return plColor(0.4f, 0.6f, 0.8f);

      PL_DEFAULT_CASE_NOT_IMPLEMENTED;
  }

  return plColor::White;
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

plConsole::plConsole() = default;

plConsole::~plConsole()
{
  if (s_pMainConsole == this)
  {
    s_pMainConsole = nullptr;
  }
}

void plConsole::SetMainConsole(plConsole* pConsole)
{
  s_pMainConsole = pConsole;
}

plConsole* plConsole::GetMainConsole()
{
  return s_pMainConsole;
}

plConsole* plConsole::s_pMainConsole = nullptr;

bool plConsole::AutoComplete(plStringBuilder& ref_sText)
{
  PL_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    plCommandInterpreterState s;
    s.m_sInput = ref_sText;

    m_pCommandInterpreter->AutoComplete(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }

    if (ref_sText != s.m_sInput)
    {
      ref_sText = s.m_sInput;
      return true;
    }
  }

  return false;
}

void plConsole::ExecuteCommand(plStringView sInput)
{
  if (sInput.IsEmpty())
    return;

  PL_LOCK(m_Mutex);

  if (m_pCommandInterpreter)
  {
    plCommandInterpreterState s;
    s.m_sInput = sInput;
    m_pCommandInterpreter->Interpret(s);

    for (auto& l : s.m_sOutput)
    {
      AddConsoleString(l.m_sText, l.m_Type);
    }
  }
  else
  {
    AddConsoleString(sInput);
  }
}

void plConsole::AddConsoleString(plStringView sText, plConsoleString::Type type /*= plConsoleString::Type::Default*/)
{
  plConsoleString cs;
  cs.m_sText = sText;
  cs.m_Type = type;

  // Broadcast that we have added a string to the console
  plConsoleEvent e;
  e.m_Type = plConsoleEvent::Type::OutputLineAdded;
  e.m_AddedpConsoleString = &cs;

  m_Events.Broadcast(e);
}

void plConsole::AddToInputHistory(plStringView sText)
{
  PL_LOCK(m_Mutex);

  m_iCurrentInputHistoryElement = -1;

  if (sText.IsEmpty())
    return;

  for (plInt32 i = 0; i < (plInt32)m_InputHistory.GetCount(); i++)
  {
    if (m_InputHistory[i] == sText) // already in the History
    {
      // just move it to the front

      for (plInt32 j = i - 1; j >= 0; j--)
        m_InputHistory[j + 1] = m_InputHistory[j];

      m_InputHistory[0] = sText;
      return;
    }
  }

  m_InputHistory.SetCount(plMath::Min<plUInt32>(m_InputHistory.GetCount() + 1, m_InputHistory.GetCapacity()));

  for (plUInt32 i = m_InputHistory.GetCount() - 1; i > 0; i--)
    m_InputHistory[i] = m_InputHistory[i - 1];

  m_InputHistory[0] = sText;
}

void plConsole::RetrieveInputHistory(plInt32 iHistoryUp, plStringBuilder& ref_sResult)
{
  PL_LOCK(m_Mutex);

  if (m_InputHistory.IsEmpty())
    return;

  m_iCurrentInputHistoryElement = plMath::Clamp<plInt32>(m_iCurrentInputHistoryElement + iHistoryUp, 0, m_InputHistory.GetCount() - 1);

  if (!m_InputHistory[m_iCurrentInputHistoryElement].IsEmpty())
  {
    ref_sResult = m_InputHistory[m_iCurrentInputHistoryElement];
  }
}

plResult plConsole::SaveInputHistory(plStringView sFile)
{
  plFileWriter file;
  PL_SUCCEED_OR_RETURN(file.Open(sFile));

  plStringBuilder str;

  for (const plString& line : m_InputHistory)
  {
    if (line.IsEmpty())
      continue;

    str.Set(line, "\n");

    PL_SUCCEED_OR_RETURN(file.WriteBytes(str.GetData(), str.GetElementCount()));
  }

  return PL_SUCCESS;
}

void plConsole::LoadInputHistory(plStringView sFile)
{
  plFileReader file;
  if (file.Open(sFile).Failed())
    return;

  plStringBuilder str;
  str.ReadAll(file);

  plHybridArray<plStringView, 32> lines;
  str.Split(false, lines, "\n", "\r");

  for (plUInt32 i = 0; i < lines.GetCount(); ++i)
  {
    AddToInputHistory(lines[lines.GetCount() - 1 - i]);
  }
}


