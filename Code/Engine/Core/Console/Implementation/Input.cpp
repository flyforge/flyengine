#include <Core/CorePCH.h>

#include <Core/Console/QuakeConsole.h>
#include <Core/Input/InputManager.h>

bool plQuakeConsole::ProcessInputCharacter(plUInt32 uiChar)
{
  switch (uiChar)
  {
    case 27: // Escape
      ClearInputLine();
      return false;

    case '\b': // backspace
    {
      if (!m_sInputLine.IsEmpty() && m_iCaretPosition > 0)
      {
        RemoveCharacter(m_iCaretPosition - 1);
        MoveCaret(-1);
      }
    }
      return false;

    case '\t':
      if (AutoComplete(m_sInputLine))
      {
        MoveCaret(500);
      }
      return false;

    case 13: // Enter
      AddToInputHistory(m_sInputLine);
      ExecuteCommand(m_sInputLine);
      ClearInputLine();
      return false;
  }

  return true;
}

bool plQuakeConsole::FilterInputCharacter(plUInt32 uiChar)
{
  // filter out not only all non-ASCII characters, but also all the non-printable ASCII characters
  // if you want to support full Unicode characters in the console, override this function and change this restriction
  if (uiChar < 32 || uiChar > 126)
    return false;

  return true;
}

void plQuakeConsole::ClampCaretPosition()
{
  m_iCaretPosition = plMath::Clamp<plInt32>(m_iCaretPosition, 0, m_sInputLine.GetCharacterCount());
}

void plQuakeConsole::MoveCaret(plInt32 iMoveOffset)
{
  m_iCaretPosition += iMoveOffset;

  ClampCaretPosition();
}

void plQuakeConsole::Scroll(plInt32 iLines)
{
  if (m_bUseFilteredStrings)
    m_iScrollPosition = plMath::Clamp<plInt32>(m_iScrollPosition + iLines, 0, plMath::Max<plInt32>(m_FilteredConsoleStrings.GetCount() - 10, 0));
  else
    m_iScrollPosition = plMath::Clamp<plInt32>(m_iScrollPosition + iLines, 0, plMath::Max<plInt32>(m_ConsoleStrings.GetCount() - 10, 0));
}

void plQuakeConsole::ClearInputLine()
{
  m_sInputLine.Clear();
  m_iCaretPosition = 0;
  m_iScrollPosition = 0;
  m_iCurrentInputHistoryElement = -1;

  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;

  InputStringChanged();
}

void plQuakeConsole::ClearConsoleStrings()
{
  m_ConsoleStrings.Clear();
  m_FilteredConsoleStrings.Clear();
  m_bUseFilteredStrings = false;
  m_iScrollPosition = 0;
}

void plQuakeConsole::DeleteNextCharacter()
{
  RemoveCharacter(m_iCaretPosition);
}

void plQuakeConsole::RemoveCharacter(plUInt32 uiInputLinePosition)
{
  if (uiInputLinePosition >= m_sInputLine.GetCharacterCount())
    return;

  auto it = m_sInputLine.GetIteratorFront();
  it += uiInputLinePosition;

  auto itNext = it;
  ++itNext;

  m_sInputLine.Remove(it.GetData(), itNext.GetData());

  InputStringChanged();
}

void plQuakeConsole::AddInputCharacter(plUInt32 uiChar)
{
  if (uiChar == '\0')
    return;

  if (!ProcessInputCharacter(uiChar))
    return;

  if (!FilterInputCharacter(uiChar))
    return;

  ClampCaretPosition();

  auto it = m_sInputLine.GetIteratorFront();
  it += m_iCaretPosition;

  plUInt32 uiString[2] = {uiChar, 0};

  m_sInputLine.Insert(it.GetData(), plStringUtf8(uiString).GetData());

  MoveCaret(1);

  InputStringChanged();
}

void plQuakeConsole::DoDefaultInputHandling(bool bConsoleOpen)
{
  if (!m_bDefaultInputHandlingInitialized)
  {
    m_bDefaultInputHandlingInitialized = true;

    plInputActionConfig cfg;
    cfg.m_bApplyTimeScaling = true;

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyLeft;
    plInputManager::SetInputActionConfig("Console", "MoveCaretLeft", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyRight;
    plInputManager::SetInputActionConfig("Console", "MoveCaretRight", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyHome;
    plInputManager::SetInputActionConfig("Console", "MoveCaretStart", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyEnd;
    plInputManager::SetInputActionConfig("Console", "MoveCaretEnd", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyDelete;
    plInputManager::SetInputActionConfig("Console", "DeleteCharacter", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyPageUp;
    plInputManager::SetInputActionConfig("Console", "ScrollUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyPageDown;
    plInputManager::SetInputActionConfig("Console", "ScrollDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyUp;
    plInputManager::SetInputActionConfig("Console", "HistoryUp", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyDown;
    plInputManager::SetInputActionConfig("Console", "HistoryDown", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyF2;
    plInputManager::SetInputActionConfig("Console", "RepeatLast", cfg, true);

    cfg.m_sInputSlotTrigger[0] = plInputSlot_KeyF3;
    plInputManager::SetInputActionConfig("Console", "RepeatSecondLast", cfg, true);

    return;
  }

  if (bConsoleOpen)
  {
    if (plInputManager::GetInputActionState("Console", "MoveCaretLeft") == plKeyState::Pressed)
      MoveCaret(-1);
    if (plInputManager::GetInputActionState("Console", "MoveCaretRight") == plKeyState::Pressed)
      MoveCaret(1);
    if (plInputManager::GetInputActionState("Console", "MoveCaretStart") == plKeyState::Pressed)
      MoveCaret(-1000);
    if (plInputManager::GetInputActionState("Console", "MoveCaretEnd") == plKeyState::Pressed)
      MoveCaret(1000);
    if (plInputManager::GetInputActionState("Console", "DeleteCharacter") == plKeyState::Pressed)
      DeleteNextCharacter();
    if (plInputManager::GetInputActionState("Console", "ScrollUp") == plKeyState::Pressed)
      Scroll(10);
    if (plInputManager::GetInputActionState("Console", "ScrollDown") == plKeyState::Pressed)
      Scroll(-10);
    if (plInputManager::GetInputActionState("Console", "HistoryUp") == plKeyState::Pressed)
    {
      RetrieveInputHistory(1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }
    if (plInputManager::GetInputActionState("Console", "HistoryDown") == plKeyState::Pressed)
    {
      RetrieveInputHistory(-1, m_sInputLine);
      m_iCaretPosition = m_sInputLine.GetCharacterCount();
    }

    const plUInt32 uiChar = plInputManager::RetrieveLastCharacter();

    if (uiChar != '\0')
      AddInputCharacter(uiChar);
  }
  else
  {
    const plUInt32 uiChar = plInputManager::RetrieveLastCharacter(false);

    char szCmd[16] = "";
    char* szIterator = szCmd;
    plUnicodeUtils::EncodeUtf32ToUtf8(uiChar, szIterator);
    *szIterator = '\0';
    ExecuteBoundKey(szCmd);
  }

  if (plInputManager::GetInputActionState("Console", "RepeatLast") == plKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 1)
      ExecuteCommand(GetInputHistory()[0]);
  }

  if (plInputManager::GetInputActionState("Console", "RepeatSecondLast") == plKeyState::Pressed)
  {
    if (GetInputHistory().GetCount() >= 2)
      ExecuteCommand(GetInputHistory()[1]);
  }
}


