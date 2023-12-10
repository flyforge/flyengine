#include <Foundation/FoundationPCH.h>

#include <Foundation/CodeUtils/Tokenizer.h>
#include <Foundation/Memory/CommonAllocators.h>

const char* plTokenType::EnumNames[plTokenType::ENUM_COUNT] = {
  "Unknown",
  "Whitespace",
  "Identifier",
  "NonIdentifier",
  "Newline",
  "LineComment",
  "BlockComment",
  "String1",
  "String2",
  "Integer",
  "Float",
  "RawString1",
  "RawString1Prefix",
  "RawString1Postfix",
  "EndOfFile"};

namespace
{
  // This allocator is used to get rid of some of the memory allocation tracking
  // that would otherwise occur for allocations made by the tokenizer.
  thread_local plAllocator<plMemoryPolicies::plHeapAllocation, plMemoryTrackingFlags::None> s_ClassAllocator("plTokenizer", plFoundation::GetDefaultAllocator());
} // namespace


plTokenizer::plTokenizer(plAllocatorBase* pAllocator)
  : m_Tokens(pAllocator != nullptr ? pAllocator : &s_ClassAllocator)
  , m_Data(pAllocator != nullptr ? pAllocator : &s_ClassAllocator)
{
}

plTokenizer::~plTokenizer() = default;

void plTokenizer::NextChar()
{
  m_uiCurChar = m_uiNextChar;
  m_szCurCharStart = m_szNextCharStart;
  ++m_uiCurColumn;

  if (m_uiCurChar == '\n')
  {
    ++m_uiCurLine;
    m_uiCurColumn = 0;
  }

  if (!m_sIterator.IsValid() || m_sIterator.IsEmpty())
  {
    m_szNextCharStart = m_sIterator.GetEndPointer();
    m_uiNextChar = '\0';
    return;
  }

  m_uiNextChar = m_sIterator.GetCharacter();
  m_szNextCharStart = m_sIterator.GetStartPointer();

  ++m_sIterator;
}

void plTokenizer::AddToken()
{
  const char* szEnd = m_szCurCharStart;

  plToken t;
  t.m_uiLine = m_uiLastLine;
  t.m_uiColumn = m_uiLastColumn;
  t.m_iType = m_CurMode;
  t.m_DataView = plStringView(m_szTokenStart, szEnd);

  m_uiLastLine = m_uiCurLine;
  m_uiLastColumn = m_uiCurColumn;

  m_Tokens.PushBack(t);

  m_szTokenStart = szEnd;

  m_CurMode = plTokenType::Unknown;
}

void plTokenizer::Tokenize(plArrayPtr<const plUInt8> data, plLogInterface* pLog)
{
  m_Data = data;
  TokenizeReference(m_Data, pLog);
}

void plTokenizer::TokenizeReference(plArrayPtr<const plUInt8> data, plLogInterface* pLog)
{
  if (data.GetCount() >= 3)
  {
    const char* dataStart = reinterpret_cast<const char*>(data.GetPtr());

    if (plUnicodeUtils::SkipUtf8Bom(dataStart))
    {
      plLog::Error(pLog, "Data to tokenize contains a Utf-8 BOM.");

      // although the tokenizer should get data without a BOM, it's easy enough to work around that here
      // that's what the tokenizer does in other error cases as well - complain, but continue
      data = plArrayPtr<const plUInt8>((const plUInt8*)dataStart, data.GetCount() - 3);
    }
  }

  //m_Data.Clear();
  //m_Data.Reserve(m_Data.GetCount() + 1);
  //m_Data = data;

  //if (m_Data.IsEmpty() || m_Data[m_Data.GetCount() - 1] != 0)
  //  m_Data.PushBack('\0'); // make sure the string is zero terminated

  m_Tokens.Clear();
  m_pLog = pLog;

  {
    m_CurMode = plTokenType::Unknown;
    m_uiCurLine = 1;
    m_uiCurColumn = -1;
    m_uiCurChar = '\0';
    m_uiNextChar = '\0';
    m_uiLastLine = 1;
    m_uiLastColumn = 1;

    m_szCurCharStart = nullptr;
    m_szNextCharStart = nullptr;
    m_szTokenStart = nullptr;
  }

  m_sIterator = plStringView((const char*)&data[0], (const char*)&data[0] + data.GetCount());

  if (!m_sIterator.IsValid() || m_sIterator.IsEmpty())
  {
    plToken t;
    t.m_uiLine = 1;
    t.m_iType = plTokenType::EndOfFile;
    m_Tokens.PushBack(t);
    return;
  }

  NextChar();
  NextChar();

  m_szTokenStart = m_szCurCharStart;

  while (m_szTokenStart != nullptr && m_szTokenStart != m_sIterator.GetEndPointer())
  {
    switch (m_CurMode)
    {
      case plTokenType::Unknown:
        HandleUnknown();
        break;

      case plTokenType::String1:
        HandleString('\"');
        break;

      case plTokenType::RawString1:
        HandleRawString();
        break;

      case plTokenType::String2:
        HandleString('\'');
        break;

      case plTokenType::Integer:
      case plTokenType::Float:
        HandleNumber();
        break;

      case plTokenType::LineComment:
        HandleLineComment();
        break;

      case plTokenType::BlockComment:
        HandleBlockComment();
        break;

      case plTokenType::Whitespace:
        HandleWhitespace();
        break;

      case plTokenType::Identifier:
        HandleIdentifier();
        break;

      case plTokenType::NonIdentifier:
        HandleNonIdentifier();
        break;

      case plTokenType::RawString1Prefix:
      case plTokenType::RawString1Postfix:
      case plTokenType::Newline:
      case plTokenType::EndOfFile:
      case plTokenType::ENUM_COUNT:
        break;
    }
  }

  plToken t;
  t.m_uiLine = m_uiCurLine;
  t.m_iType = plTokenType::EndOfFile;
  m_Tokens.PushBack(t);
}

void plTokenizer::HandleUnknown()
{
  m_szTokenStart = m_szCurCharStart;

  if ((m_uiCurChar == '/') && (m_uiNextChar == '/'))
  {
    m_CurMode = plTokenType::LineComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_bHashSignIsLineComment && (m_uiCurChar == '#'))
  {
    m_CurMode = plTokenType::LineComment;
    NextChar();
    return;
  }

  if ((m_uiCurChar == '/') && (m_uiNextChar == '*'))
  {
    m_CurMode = plTokenType::BlockComment;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\"')
  {
    m_CurMode = plTokenType::String1;
    NextChar();
    return;
  }

  if (m_uiCurChar == 'R' && m_uiNextChar == '\"')
  {
    m_CurMode = plTokenType::RawString1;
    NextChar();
    NextChar();
    return;
  }

  if (m_uiCurChar == '\'')
  {
    m_CurMode = plTokenType::String2;
    NextChar();
    return;
  }

  if ((m_uiCurChar == ' ') || (m_uiCurChar == '\t'))
  {
    m_CurMode = plTokenType::Whitespace;
    NextChar();
    return;
  }

  if (plStringUtils::IsDecimalDigit(m_uiCurChar) || (m_uiCurChar == '.' && plStringUtils::IsDecimalDigit(m_uiNextChar)))
  {
    m_CurMode = m_uiCurChar == '.' ? plTokenType::Float : plTokenType::Integer;
    // Do not advance to next char here since we need the first character in HandleNumber
    return;
  }

  if (!plStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
  {
    m_CurMode = plTokenType::Identifier;
    NextChar();
    return;
  }

  if (m_uiCurChar == '\n')
  {
    m_CurMode = plTokenType::Newline;
    NextChar();
    AddToken();
    return;
  }

  if ((m_uiCurChar == '\r') && (m_uiNextChar == '\n'))
  {
    NextChar();
    NextChar();
    m_CurMode = plTokenType::Newline;
    AddToken();
    return;
  }

  // else
  m_CurMode = plTokenType::NonIdentifier;
  NextChar();
}

void plTokenizer::HandleString(char terminator)
{
  while (m_uiCurChar != '\0')
  {
    // Escaped quote \"
    if ((m_uiCurChar == '\\') && (m_uiNextChar == terminator))
    {
      // skip this one
      NextChar();
      NextChar();
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\n'))
    {
      AddToken();

      // skip this one entirely
      NextChar();
      NextChar();

      m_CurMode = terminator == '\"' ? plTokenType::String1 : plTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped line break in string
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\r'))
    {
      // this might be a 3 character sequence of \\ \r \n -> skip them all
      AddToken();

      // skip \\ and \r
      NextChar();
      NextChar();

      // skip \n
      if (m_uiCurChar == '\n')
        NextChar();

      m_CurMode = terminator == '\"' ? plTokenType::String1 : plTokenType::String2;
      m_szTokenStart = m_szCurCharStart;
    }
    // escaped backslash
    else if ((m_uiCurChar == '\\') && (m_uiNextChar == '\\'))
    {
      // Skip
      NextChar();
      NextChar();
    }
    // not-escaped line break in string
    else if (m_uiCurChar == '\n')
    {
      plLog::Error(m_pLog, "Unescaped Newline in string line {0} column {1}", m_uiCurLine, m_uiCurColumn);
      // NextChar(); // not sure whether to include the newline in the string or not
      AddToken();
      return;
    }
    // end of string
    else if (m_uiCurChar == terminator)
    {
      NextChar();
      AddToken();
      return;
    }
    else
    {
      NextChar();
    }
  }

  plLog::Error(m_pLog, "String not closed at end of file");
  AddToken();
}

void plTokenizer::HandleRawString()
{
  const char* markerStart = m_szCurCharStart;
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar == '(')
    {
      m_sRawStringMarker = plStringView(markerStart, m_szCurCharStart);
      NextChar(); // consume '('
      break;
    }
    NextChar();
  }
  if (m_uiCurChar == '\0')
  {
    plLog::Error(m_pLog, "Failed to find '(' for raw string before end of file");
    AddToken();
    return;
  }

  m_CurMode = plTokenType::RawString1Prefix;
  AddToken();

  m_CurMode = plTokenType::RawString1;

  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar == ')')
    {
      if (m_sRawStringMarker.GetElementCount() == 0 && m_uiNextChar == '\"')
      {
        AddToken();
        NextChar();
        NextChar();
        m_CurMode = plTokenType::RawString1Postfix;
        AddToken();
        return;
      }
      else if (m_szCurCharStart + m_sRawStringMarker.GetElementCount() + 2 <= m_sIterator.GetEndPointer())
      {
        if (plStringUtils::CompareN(m_szCurCharStart + 1, m_sRawStringMarker.GetStartPointer(), m_sRawStringMarker.GetElementCount()) == 0 &&
            m_szCurCharStart[m_sRawStringMarker.GetElementCount() + 1] == '\"')
        {
          AddToken();
          for (plUInt32 i = 0; i < m_sRawStringMarker.GetElementCount() + 2; ++i) // consume )marker"
          {
            NextChar();
          }
          m_CurMode = plTokenType::RawString1Postfix;
          AddToken();
          return;
        }
      }
      NextChar();
    }
    else
    {
      NextChar();
    }
  }

  plLog::Error(m_pLog, "Raw string not closed at end of file");
  AddToken();
}

void plTokenizer::HandleNumber()
{
  if (m_uiCurChar == '0' && (m_uiNextChar == 'x' || m_uiNextChar == 'X'))
  {
    NextChar();
    NextChar();

    plUInt32 uiDigitsRead = 0;
    while (plStringUtils::IsHexDigit(m_uiCurChar))
    {
      NextChar();
      ++uiDigitsRead;
    }

    if (uiDigitsRead < 1)
    {
      plLog::Error(m_pLog, "Invalid hex literal");
    }
  }
  else
  {
    NextChar();

    while (plStringUtils::IsDecimalDigit(m_uiCurChar) || m_uiCurChar == '\'') // integer literal: 100'000
    {
      NextChar();
    }

    if (m_CurMode != plTokenType::Float && (m_uiCurChar == '.' || m_uiCurChar == 'e' || m_uiCurChar == 'E'))
    {
      m_CurMode = plTokenType::Float;
      bool bAllowExponent = true;

      if (m_uiCurChar == '.')
      {
        NextChar();

        plUInt32 uiDigitsRead = 0;
        while (plStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        bAllowExponent = uiDigitsRead > 0;
      }

      if ((m_uiCurChar == 'e' || m_uiCurChar == 'E') && bAllowExponent)
      {
        NextChar();
        if (m_uiCurChar == '+' || m_uiCurChar == '-')
        {
          NextChar();
        }

        plUInt32 uiDigitsRead = 0;
        while (plStringUtils::IsDecimalDigit(m_uiCurChar))
        {
          NextChar();
          ++uiDigitsRead;
        }

        if (uiDigitsRead < 1)
        {
          plLog::Error(m_pLog, "Invalid float literal");
        }
      }

      if (m_uiCurChar == 'f') // skip float suffix
      {
        NextChar();
      }
    }
  }

  AddToken();
}

void plTokenizer::HandleLineComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '\r') || (m_uiCurChar == '\n'))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // comment at end of file
  AddToken();
}

void plTokenizer::HandleBlockComment()
{
  while (m_uiCurChar != '\0')
  {
    if ((m_uiCurChar == '*') && (m_uiNextChar == '/'))
    {
      NextChar();
      NextChar();
      AddToken();
      return;
    }

    NextChar();
  }

  plLog::Error(m_pLog, "Block comment not closed at end of file.");
  AddToken();
}

void plTokenizer::HandleWhitespace()
{
  while (m_uiCurChar != '\0')
  {
    if (m_uiCurChar != ' ' && m_uiCurChar != '\t')
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // whitespace at end of file
  AddToken();
}

void plTokenizer::HandleIdentifier()
{
  while (m_uiCurChar != '\0')
  {
    if (plStringUtils::IsIdentifierDelimiter_C_Code(m_uiCurChar))
    {
      AddToken();
      return;
    }

    NextChar();
  }

  // identifier at end of file
  AddToken();
}

void plTokenizer::HandleNonIdentifier()
{
  AddToken();
}

void plTokenizer::GetAllLines(plHybridArray<const plToken*, 32>& ref_tokens) const
{
  ref_tokens.Clear();
  ref_tokens.Reserve(m_Tokens.GetCount());

  for (const plToken& curToken : m_Tokens)
  {
    if (curToken.m_iType != plTokenType::Newline)
    {
      ref_tokens.PushBack(&curToken);
    }
  }
}

plResult plTokenizer::GetNextLine(plUInt32& ref_uiFirstToken, plHybridArray<plToken*, 32>& ref_tokens)
{
  ref_tokens.Clear();

  plHybridArray<const plToken*, 32> Tokens0;
  plResult r = GetNextLine(ref_uiFirstToken, Tokens0);

  ref_tokens.SetCountUninitialized(Tokens0.GetCount());
  for (plUInt32 i = 0; i < Tokens0.GetCount(); ++i)
    ref_tokens[i] = const_cast<plToken*>(Tokens0[i]); // soo evil !

  return r;
}

plResult plTokenizer::GetNextLine(plUInt32& ref_uiFirstToken, plHybridArray<const plToken*, 32>& ref_tokens) const
{
  ref_tokens.Clear();

  const plUInt32 uiMaxTokens = m_Tokens.GetCount() - 1;

  while (ref_uiFirstToken < uiMaxTokens)
  {
    const plToken& tCur = m_Tokens[ref_uiFirstToken];

    // found a backslash
    if (tCur.m_iType == plTokenType::NonIdentifier && tCur.m_DataView == "\\")
    {
      const plToken& tNext = m_Tokens[ref_uiFirstToken + 1];

      // and a newline!
      if (tNext.m_iType == plTokenType::Newline)
      {
        /// \todo Theoretically, if the line ends with an identifier, and the next directly starts with one again,
        // we would need to merge the two into one identifier name, because the \ \n combo means it is not a
        // real line break
        // for now we ignore this and assume there is a 'whitespace' between such identifiers

        // we could maybe at least output a warning, if we detect it
        if (ref_uiFirstToken > 0 && m_Tokens[ref_uiFirstToken - 1].m_iType == plTokenType::Identifier && ref_uiFirstToken + 2 < uiMaxTokens && m_Tokens[ref_uiFirstToken + 2].m_iType == plTokenType::Identifier)
        {
          plStringBuilder s1 = m_Tokens[ref_uiFirstToken - 1].m_DataView;
          plStringBuilder s2 = m_Tokens[ref_uiFirstToken + 2].m_DataView;
          plLog::Warning("Line {0}: The \\ at the line end is in the middle of an identifier name ('{1}' and '{2}'). However, merging identifier "
                         "names is currently not supported.",
            m_Tokens[ref_uiFirstToken].m_uiLine, s1, s2);
        }

        // ignore this
        ref_uiFirstToken += 2;
        continue;
      }
    }

    ref_tokens.PushBack(&tCur);

    if (m_Tokens[ref_uiFirstToken].m_iType == plTokenType::Newline)
    {
      ++ref_uiFirstToken;
      return PLASMA_SUCCESS;
    }

    ++ref_uiFirstToken;
  }

  if (ref_tokens.IsEmpty())
    return PLASMA_FAILURE;

  return PLASMA_SUCCESS;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_CodeUtils_Implementation_Tokenizer);
