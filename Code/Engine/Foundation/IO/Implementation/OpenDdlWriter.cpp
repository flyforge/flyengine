#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Utilities/ConversionUtils.h>

void plOpenDdlWriter::OutputEscapedString(const plStringView& string)
{
  m_sTemp = string;
  m_sTemp.ReplaceAll("\\", "\\\\");
  m_sTemp.ReplaceAll("\"", "\\\"");
  m_sTemp.ReplaceAll("\b", "\\b");
  m_sTemp.ReplaceAll("\r", "\\r");
  m_sTemp.ReplaceAll("\f", "\\f");
  m_sTemp.ReplaceAll("\n", "\\n");
  m_sTemp.ReplaceAll("\t", "\\t");

  OutputString("\"", 1);
  OutputString(m_sTemp.GetData());
  OutputString("\"", 1);
}

void plOpenDdlWriter::OutputIndentation()
{
  if (m_bCompactMode)
    return;

  plInt32 iIndentation = m_iIndentation;

  // I need my space!
  const char* szIndentation = "\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t";

  while (iIndentation >= 16)
  {
    OutputString(szIndentation, 16);
    iIndentation -= 16;
  }

  if (iIndentation > 0)
  {
    OutputString(szIndentation, iIndentation);
  }
}

void plOpenDdlWriter::OutputPrimitiveTypeNameCompliant(plOpenDdlPrimitiveType type)
{
  switch (type)
  {
    case plOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case plOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case plOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case plOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case plOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case plOpenDdlPrimitiveType::UInt8:
      OutputString("unsigned_int8", 13);
      break;
    case plOpenDdlPrimitiveType::UInt16:
      OutputString("unsigned_int16", 14);
      break;
    case plOpenDdlPrimitiveType::UInt32:
      OutputString("unsigned_int32", 14);
      break;
    case plOpenDdlPrimitiveType::UInt64:
      OutputString("unsigned_int64", 14);
      break;
    case plOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case plOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case plOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      PLASMA_REPORT_FAILURE("Unknown DDL primitive type {0}", (plUInt32)type);
      break;
  }
}
void plOpenDdlWriter::OutputPrimitiveTypeNameShort(plOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write uint8 etc. instead of unsigned_int

  switch (type)
  {
    case plOpenDdlPrimitiveType::Bool:
      OutputString("bool", 4);
      break;
    case plOpenDdlPrimitiveType::Int8:
      OutputString("int8", 4);
      break;
    case plOpenDdlPrimitiveType::Int16:
      OutputString("int16", 5);
      break;
    case plOpenDdlPrimitiveType::Int32:
      OutputString("int32", 5);
      break;
    case plOpenDdlPrimitiveType::Int64:
      OutputString("int64", 5);
      break;
    case plOpenDdlPrimitiveType::UInt8:
      OutputString("uint8", 5);
      break;
    case plOpenDdlPrimitiveType::UInt16:
      OutputString("uint16", 6);
      break;
    case plOpenDdlPrimitiveType::UInt32:
      OutputString("uint32", 6);
      break;
    case plOpenDdlPrimitiveType::UInt64:
      OutputString("uint64", 6);
      break;
    case plOpenDdlPrimitiveType::Float:
      OutputString("float", 5);
      break;
    case plOpenDdlPrimitiveType::Double:
      OutputString("double", 6);
      break;
    case plOpenDdlPrimitiveType::String:
      OutputString("string", 6);
      break;

    default:
      PLASMA_REPORT_FAILURE("Unknown DDL primitive type {0}", (plUInt32)type);
      break;
  }
}

void plOpenDdlWriter::OutputPrimitiveTypeNameShortest(plOpenDdlPrimitiveType type)
{
  // Change to OpenDDL: We write super short type strings

  switch (type)
  {
    case plOpenDdlPrimitiveType::Bool:
      OutputString("b", 1);
      break;
    case plOpenDdlPrimitiveType::Int8:
      OutputString("i1", 2);
      break;
    case plOpenDdlPrimitiveType::Int16:
      OutputString("i2", 2);
      break;
    case plOpenDdlPrimitiveType::Int32:
      OutputString("i3", 2);
      break;
    case plOpenDdlPrimitiveType::Int64:
      OutputString("i4", 2);
      break;
    case plOpenDdlPrimitiveType::UInt8:
      OutputString("u1", 2);
      break;
    case plOpenDdlPrimitiveType::UInt16:
      OutputString("u2", 2);
      break;
    case plOpenDdlPrimitiveType::UInt32:
      OutputString("u3", 2);
      break;
    case plOpenDdlPrimitiveType::UInt64:
      OutputString("u4", 2);
      break;
    case plOpenDdlPrimitiveType::Float:
      OutputString("f", 1);
      break;
    case plOpenDdlPrimitiveType::Double:
      OutputString("d", 1);
      break;
    case plOpenDdlPrimitiveType::String:
      OutputString("s", 1);
      break;

    default:
      PLASMA_REPORT_FAILURE("Unknown DDL primitive type {0}", (plUInt32)type);
      break;
  }
}

plOpenDdlWriter::plOpenDdlWriter()
{
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesBool == (int)plOpenDdlPrimitiveType::Bool);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesInt8 == (int)plOpenDdlPrimitiveType::Int8);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesInt16 == (int)plOpenDdlPrimitiveType::Int16);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesInt32 == (int)plOpenDdlPrimitiveType::Int32);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesInt64 == (int)plOpenDdlPrimitiveType::Int64);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesUInt8 == (int)plOpenDdlPrimitiveType::UInt8);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesUInt16 == (int)plOpenDdlPrimitiveType::UInt16);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesUInt32 == (int)plOpenDdlPrimitiveType::UInt32);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesUInt64 == (int)plOpenDdlPrimitiveType::UInt64);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesFloat == (int)plOpenDdlPrimitiveType::Float);
  PLASMA_CHECK_AT_COMPILETIME((int)plOpenDdlWriter::State::PrimitivesString == (int)plOpenDdlPrimitiveType::String);

  m_StateStack.ExpandAndGetRef().m_State = State::Invalid;
  m_StateStack.ExpandAndGetRef().m_State = State::Empty;
}

// All,              ///< All whitespace is output. This is the default, it should be used for files that are read by humans.
// LessIndentation,  ///< Saves some space by using less space for indentation
// NoIndentation,    ///< Saves even more space by dropping all indentation from the output. The result will be noticeably less readable.
// NewlinesOnly,     ///< All unnecessary whitespace, except for newlines, is not output.
// None,             ///< No whitespace, not even newlines, is output. This should be used when DDL is used for data exchange, but probably not read
// by humans.

void plOpenDdlWriter::BeginObject(plStringView sType, plStringView sName /*= {}*/, bool bGlobalName /*= false*/, bool bSingleLine /*= false*/)
{
  {
    const auto state = m_StateStack.PeekBack().m_State;
    PLASMA_IGNORE_UNUSED(state);
    PLASMA_ASSERT_DEBUG(state == State::Empty || state == State::ObjectMultiLine || state == State::ObjectStart,
      "DDL Writer is in a state where no further objects may be created");
  }

  OutputObjectBeginning();

  {
    const auto state = m_StateStack.PeekBack().m_State;
    PLASMA_IGNORE_UNUSED(state);
    PLASMA_ASSERT_DEBUG(state != State::ObjectSingleLine, "Cannot put an object into another single-line object");
    PLASMA_ASSERT_DEBUG(state != State::ObjectStart, "Object beginning should have been written");
  }

  OutputIndentation();
  OutputString(sType);

  OutputObjectName(sName, bGlobalName);

  if (bSingleLine)
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectSingleLine;
  }
  else
  {
    m_StateStack.ExpandAndGetRef().m_State = State::ObjectMultiLine;
  }

  m_StateStack.ExpandAndGetRef().m_State = State::ObjectStart;
}


void plOpenDdlWriter::OutputObjectBeginning()
{
  if (m_StateStack.PeekBack().m_State != State::ObjectStart)
    return;

  m_StateStack.PopBack();

  const auto state = m_StateStack.PeekBack().m_State;

  if (state == State::ObjectSingleLine)
  {
    // if (m_bCompactMode)
    OutputString("{", 1); // more compact
    // else
    // OutputString(" { ", 3);
  }
  else if (state == State::ObjectMultiLine)
  {
    if (m_bCompactMode)
    {
      OutputString("{", 1);
    }
    else
    {
      OutputString("\n", 1);
      OutputIndentation();
      OutputString("{\n", 2);
    }
  }

  m_iIndentation++;
}

bool IsDdlIdentifierCharacter(plUInt32 uiByte);

void plOpenDdlWriter::OutputObjectName(plStringView sName, bool bGlobalName)
{
  if (!sName.IsEmpty())
  {
    // PLASMA_ASSERT_DEBUG(plStringUtils::FindSubString(szName, " ") == nullptr, "Spaces are not allowed in DDL object names: '{0}'", szName);


    /// \test This code path is untested
    bool bEscape = false;
    for (auto nameIt = sName.GetIteratorFront(); nameIt.IsValid(); ++nameIt)
    {
      if (!IsDdlIdentifierCharacter(nameIt.GetCharacter()))
      {
        bEscape = true;
        break;
      }
    }

    if (m_bCompactMode)
    {
      // even remove the whitespace between type and name

      if (bGlobalName)
        OutputString("$", 1);
      else
        OutputString("%", 1);
    }
    else
    {
      if (bGlobalName)
        OutputString(" $", 2);
      else
        OutputString(" %", 2);
    }

    if (bEscape)
      OutputString("\'", 1);

    OutputString(sName);

    if (bEscape)
      OutputString("\'", 1);
  }
}

void plOpenDdlWriter::EndObject()
{
  const auto state = m_StateStack.PeekBack().m_State;
  PLASMA_ASSERT_DEBUG(state == State::ObjectSingleLine || state == State::ObjectMultiLine || state == State::ObjectStart, "No object is open");

  if (state == State::ObjectStart)
  {
    // object is empty

    OutputString("{}\n", 3);
    m_StateStack.PopBack();

    const auto newState = m_StateStack.PeekBack().m_State;
    PLASMA_IGNORE_UNUSED(newState);
    PLASMA_ASSERT_DEBUG(newState == State::ObjectSingleLine || newState == State::ObjectMultiLine, "No object is open");
  }
  else
  {
    m_iIndentation--;

    if (m_bCompactMode)
      OutputString("}", 1);
    else
    {
      if (state == State::ObjectMultiLine)
      {
        OutputIndentation();
        OutputString("}\n", 2);
      }
      else
      {
        // OutputString(" }\n", 3);
        OutputString("}\n", 2); // more compact
      }
    }
  }

  m_StateStack.PopBack();
}

void plOpenDdlWriter::BeginPrimitiveList(plOpenDdlPrimitiveType type, plStringView sName /*= {}*/, bool bGlobalName /*= false*/)
{
  OutputObjectBeginning();

  const auto state = m_StateStack.PeekBack().m_State;
  PLASMA_ASSERT_DEBUG(state == State::Empty || state == State::ObjectSingleLine || state == State::ObjectMultiLine,
    "DDL Writer is in a state where no primitive list may be created");

  if (state == State::ObjectMultiLine)
  {
    OutputIndentation();
  }

  if (m_TypeStringMode == TypeStringMode::Shortest)
    OutputPrimitiveTypeNameShortest(type);
  else if (m_TypeStringMode == TypeStringMode::ShortenedUnsignedInt)
    OutputPrimitiveTypeNameShort(type);
  else
    OutputPrimitiveTypeNameCompliant(type);

  OutputObjectName(sName, bGlobalName);

  // more compact
  // if (m_bCompactMode)
  OutputString("{", 1);
  // else
  // OutputString(" {", 2);

  m_StateStack.ExpandAndGetRef().m_State = static_cast<State>(type);
}

void plOpenDdlWriter::EndPrimitiveList()
{
  const auto state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEBUG(state >= State::PrimitivesBool && state <= State::PrimitivesString, "No primitive list is open");

  m_StateStack.PopBack();

  if (m_bCompactMode)
    OutputString("}", 1);
  else
  {
    if (m_StateStack.PeekBack().m_State == State::ObjectSingleLine)
      OutputString("}", 1);
    else
      OutputString("}\n", 2);
  }
}

void plOpenDdlWriter::WritePrimitiveType(plOpenDdlWriter::State exp)
{
  auto& state = m_StateStack.PeekBack();
  PLASMA_ASSERT_DEBUG(state.m_State == exp, "Cannot write thie primitive type without have the correct primitive list open");

  if (state.m_bPrimitivesWritten)
  {
    // already wrote some primitives, so append a comma
    OutputString(",", 1);
  }

  state.m_bPrimitivesWritten = true;
}


void plOpenDdlWriter::WriteBinaryAsHex(const void* pData, plUInt32 uiBytes)
{
  char tmp[4];

  plUInt8* pBytes = (plUInt8*)pData;

  for (plUInt32 i = 0; i < uiBytes; ++i)
  {
    plStringUtils::snprintf(tmp, 4, "%02X", (plUInt32)*pBytes);
    ++pBytes;

    OutputString(tmp, 2);
  }
}

void plOpenDdlWriter::WriteBool(const bool* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesBool);

  if (m_bCompactMode || m_TypeStringMode == TypeStringMode::Shortest)
  {
    // Extension to OpenDDL: We write only '1' or '0' in compact mode

    if (pValues[0])
      OutputString("1", 1);
    else
      OutputString("0", 1);

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",1", 2);
      else
        OutputString(",0", 2);
    }
  }
  else
  {
    if (pValues[0])
      OutputString("true", 4);
    else
      OutputString("false", 5);

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i])
        OutputString(",true", 5);
      else
        OutputString(",false", 6);
    }
  }
}

void plOpenDdlWriter::WriteInt8(const plInt8* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt8);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteInt16(const plInt16* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt16);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteInt32(const plInt32* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt32);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteInt64(const plInt64* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesInt64);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}


void plOpenDdlWriter::WriteUInt8(const plUInt8* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt8);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteUInt16(const plUInt16* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt16);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteUInt32(const plUInt32* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt32);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteUInt64(const plUInt64* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesUInt64);

  m_sTemp.Format("{0}", pValues[0]);
  OutputString(m_sTemp.GetData());

  for (plUInt32 i = 1; i < uiCount; ++i)
  {
    m_sTemp.Format(",{0}", pValues[i]);
    OutputString(m_sTemp.GetData());
  }
}

void plOpenDdlWriter::WriteFloat(const float* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesFloat);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.Format("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.Format(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 4);
    }

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 4);
      }
    }
  }
}

void plOpenDdlWriter::WriteDouble(const double* pValues, plUInt32 uiCount /*= 1*/)
{
  PLASMA_ASSERT_DEBUG(pValues != nullptr, "Invalid value array");
  PLASMA_ASSERT_DEBUG(uiCount > 0, "This is pointless");

  WritePrimitiveType(State::PrimitivesDouble);

  if (m_FloatPrecisionMode == FloatPrecisionMode::Readable)
  {
    m_sTemp.Format("{0}", pValues[0]);
    OutputString(m_sTemp.GetData());

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      m_sTemp.Format(",{0}", pValues[i]);
      OutputString(m_sTemp.GetData());
    }
  }
  else
  {
    // zeros are so common that writing them in HEX blows up file size, so write them as decimals

    if (pValues[0] == 0)
    {
      OutputString("0", 1);
    }
    else
    {
      OutputString("0x", 2);
      WriteBinaryAsHex(&pValues[0], 8);
    }

    for (plUInt32 i = 1; i < uiCount; ++i)
    {
      if (pValues[i] == 0)
      {
        OutputString(",0", 2);
      }
      else
      {
        OutputString(",0x", 3);
        WriteBinaryAsHex(&pValues[i], 8);
      }
    }
  }
}

void plOpenDdlWriter::WriteString(const plStringView& sString)
{
  WritePrimitiveType(State::PrimitivesString);

  OutputEscapedString(sString);
}

void plOpenDdlWriter::WriteBinaryAsString(const void* pData, plUInt32 uiBytes)
{
  /// \test plOpenDdlWriter::WriteBinaryAsString

  WritePrimitiveType(State::PrimitivesString);

  OutputString("\"", 1);
  WriteBinaryAsHex(pData, uiBytes);
  OutputString("\"", 1);
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_OpenDdlWriter);
