#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

plStandardJSONWriter::JSONState::JSONState()
{
  m_State = Invalid;
  m_bRequireComma = false;
  m_bValueWasWritten = false;
}

plStandardJSONWriter::CommaWriter::CommaWriter(plStandardJSONWriter* pWriter)
{
  const plStandardJSONWriter::State state = pWriter->m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV(state == plStandardJSONWriter::Array || state == plStandardJSONWriter::NamedArray || state == plStandardJSONWriter::Variable,
    "Values can only be written inside BeginVariable() / EndVariable() and BeginArray() / EndArray().");

  m_pWriter = pWriter;

  if (m_pWriter->m_StateStack.PeekBack().m_bRequireComma)
  {
    // we are writing the comma now, so it is not required anymore
    m_pWriter->m_StateStack.PeekBack().m_bRequireComma = false;

    if (m_pWriter->m_StateStack.PeekBack().m_State == plStandardJSONWriter::Array ||
        m_pWriter->m_StateStack.PeekBack().m_State == plStandardJSONWriter::NamedArray)
    {
      if (pWriter->m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
      {
        if (pWriter->m_ArrayMode == plJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(",");
        else
          m_pWriter->OutputString(",\n");
      }
      else
      {
        if (pWriter->m_ArrayMode == plJSONWriter::ArrayMode::InOneLine)
          m_pWriter->OutputString(", ");
        else
        {
          m_pWriter->OutputString(",\n");
          m_pWriter->OutputIndentation();
        }
      }
    }
    else
    {
      if (pWriter->m_WhitespaceMode >= plJSONWriter::WhitespaceMode::None)
        m_pWriter->OutputString(",");
      else
        m_pWriter->OutputString(",\n");

      m_pWriter->OutputIndentation();
    }
  }
}

plStandardJSONWriter::CommaWriter::~CommaWriter()
{
  m_pWriter->m_StateStack.PeekBack().m_bRequireComma = true;
  m_pWriter->m_StateStack.PeekBack().m_bValueWasWritten = true;
}

plStandardJSONWriter::plStandardJSONWriter()
{
  m_iIndentation = 0;
  m_pOutput = nullptr;
  JSONState s;
  s.m_State = plStandardJSONWriter::Empty;
  m_StateStack.PushBack(s);
}

plStandardJSONWriter::~plStandardJSONWriter()
{
  if (!HadWriteError())
  {
    PLASMA_ASSERT_DEV(m_StateStack.PeekBack().m_State == plStandardJSONWriter::Empty, "The JSON stream must be closed properly.");
  }
}

void plStandardJSONWriter::SetOutputStream(plStreamWriter* pOutput)
{
  m_pOutput = pOutput;
}

void plStandardJSONWriter::OutputString(plStringView s)
{
  PLASMA_ASSERT_DEBUG(m_pOutput != nullptr, "No output stream has been set yet.");

  if (m_pOutput->WriteBytes(s.GetStartPointer(), s.GetElementCount()).Failed())
  {
    SetWriteErrorState();
  }
}

void plStandardJSONWriter::OutputEscapedString(plStringView s)
{
  plStringBuilder sEscaped = s;
  sEscaped.ReplaceAll("\\", "\\\\");
  // sEscaped.ReplaceAll("/", "\\/"); // this is not necessary to escape
  sEscaped.ReplaceAll("\"", "\\\"");
  sEscaped.ReplaceAll("\b", "\\b");
  sEscaped.ReplaceAll("\r", "\\r");
  sEscaped.ReplaceAll("\f", "\\f");
  sEscaped.ReplaceAll("\n", "\\n");
  sEscaped.ReplaceAll("\t", "\\t");

  OutputString("\"");
  OutputString(sEscaped);
  OutputString("\"");
}

void plStandardJSONWriter::OutputIndentation()
{
  if (m_WhitespaceMode >= WhitespaceMode::NoIndentation)
    return;

  plInt32 iIndentation = m_iIndentation * 2;

  if (m_WhitespaceMode == WhitespaceMode::LessIndentation)
    iIndentation = m_iIndentation;

  plStringBuilder s;
  s.Printf("%*s", iIndentation, "");

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteBool(bool value)
{
  CommaWriter cw(this);

  if (value)
    OutputString("true");
  else
    OutputString("false");
}

void plStandardJSONWriter::WriteInt32(plInt32 value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteUInt32(plUInt32 value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteInt64(plInt64 value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteUInt64(plUInt64 value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteFloat(float value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteDouble(double value)
{
  CommaWriter cw(this);

  plStringBuilder s;
  s.Format("{0}", value);

  OutputString(s.GetData());
}

void plStandardJSONWriter::WriteString(plStringView value)
{
  CommaWriter cw(this);

  OutputEscapedString(value);
}

void plStandardJSONWriter::WriteNULL()
{
  CommaWriter cw(this);

  OutputString("null");
}

void plStandardJSONWriter::WriteTime(plTime value)
{
  WriteDouble(value.GetSeconds());
}

void plStandardJSONWriter::WriteColor(const plColor& value)
{
  plVec4 temp(value.r, value.g, value.b, value.a);

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", plArgF(value.r, 4), plArgF(value.g, 4), plArgF(value.b, 4), plArgF(value.a, 4));
  else
    s.Format("({0}, {1}, {2}, {3})", plArgF(value.r, 4), plArgF(value.g, 4), plArgF(value.b, 4), plArgF(value.a, 4));

  WriteBinaryData("color", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteColorGamma(const plColorGammaUB& value)
{
  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", value.r, value.g, value.b, value.a);
  else
    s.Format("({0}, {1}, {2}, {3})", value.r, value.g, value.b, value.a);

  WriteBinaryData("gamma", value.GetData(), sizeof(plColorGammaUB), s.GetData());
}

void plStandardJSONWriter::WriteVec2(const plVec2& value)
{
  plVec2 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1})", plArgF(value.x, 4), plArgF(value.y, 4));
  else
    s.Format("({0}, {1})", plArgF(value.x, 4), plArgF(value.y, 4));

  WriteBinaryData("vec2", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteVec3(const plVec3& value)
{
  plVec3 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2})", plArgF(value.x, 4), plArgF(value.y, 4), plArgF(value.z, 4));
  else
    s.Format("({0}, {1}, {2})", plArgF(value.x, 4), plArgF(value.y, 4), plArgF(value.z, 4));

  WriteBinaryData("vec3", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteVec4(const plVec4& value)
{
  plVec4 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", plArgF(value.x, 4), plArgF(value.y, 4), plArgF(value.z, 4), plArgF(value.w, 4));
  else
    s.Format("({0}, {1}, {2}, {3})", plArgF(value.x, 4), plArgF(value.y, 4), plArgF(value.z, 4), plArgF(value.w, 4));

  WriteBinaryData("vec4", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteVec2I32(const plVec2I32& value)
{
  CommaWriter cw(this);

  plVec2I32 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(plInt32));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1})", value.x, value.y);
  else
    s.Format("({0}, {1})", value.x, value.y);

  WriteBinaryData("vec2i", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteVec3I32(const plVec3I32& value)
{
  CommaWriter cw(this);

  plVec3I32 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(plInt32));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2})", value.x, value.y, value.z);
  else
    s.Format("({0}, {1}, {2})", value.x, value.y, value.z);

  WriteBinaryData("vec3i", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteVec4I32(const plVec4I32& value)
{
  CommaWriter cw(this);

  plVec4I32 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(plInt32));

  plStringBuilder s;

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::NewlinesOnly)
    s.Format("({0},{1},{2},{3})", value.x, value.y, value.z, value.w);
  else
    s.Format("({0}, {1}, {2}, {3})", value.x, value.y, value.z, value.w);

  WriteBinaryData("vec4i", &temp, sizeof(temp), s.GetData());
}

void plStandardJSONWriter::WriteQuat(const plQuat& value)
{
  plQuat temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("quat", &temp, sizeof(temp));
}

void plStandardJSONWriter::WriteMat3(const plMat3& value)
{
  plMat3 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat3", &temp, sizeof(temp));
}

void plStandardJSONWriter::WriteMat4(const plMat4& value)
{
  plMat4 temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt32*)&temp, sizeof(temp) / sizeof(float));

  WriteBinaryData("mat4", &temp, sizeof(temp));
}

void plStandardJSONWriter::WriteUuid(const plUuid& value)
{
  CommaWriter cw(this);

  plUuid temp = value;

  plEndianHelper::NativeToLittleEndian((plUInt64*)&temp, sizeof(temp) / sizeof(plUInt64));

  WriteBinaryData("uuid", &temp, sizeof(temp));
}

void plStandardJSONWriter::WriteAngle(plAngle value)
{
  WriteFloat(value.GetDegree());
}

void plStandardJSONWriter::WriteDataBuffer(const plDataBuffer& value)
{
  WriteBinaryData("data", value.GetData(), value.GetCount());
}

void plStandardJSONWriter::BeginVariable(plStringView sName)
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV(state == plStandardJSONWriter::Empty || state == plStandardJSONWriter::Object || state == plStandardJSONWriter::NamedObject,
    "Variables can only be written inside objects.");

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  OutputEscapedString(sName);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString(":");
  else
    OutputString(" : ");

  JSONState s;
  s.m_State = plStandardJSONWriter::Variable;
  m_StateStack.PushBack(s);
}

void plStandardJSONWriter::EndVariable()
{
  PLASMA_ASSERT_DEV(m_StateStack.PeekBack().m_State == plStandardJSONWriter::Variable, "EndVariable() must be called in sync with BeginVariable().");
  PLASMA_ASSERT_DEV(m_StateStack.PeekBack().m_bValueWasWritten, "EndVariable() cannot be called without writing any value in between.");

  End();
}

void plStandardJSONWriter::BeginArray(plStringView sName)
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV((state == plStandardJSONWriter::Empty) ||
                  ((state == plStandardJSONWriter::Object || state == plStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == plStandardJSONWriter::Array || state == plStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == plStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin arrays when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous arrays, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString(",");
    else
      OutputString(", ");
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("[");
  else
    OutputString("[ ");

  JSONState s;
  s.m_State = (sName == nullptr) ? plStandardJSONWriter::Array : plStandardJSONWriter::NamedArray;
  m_StateStack.PushBack(s);
  ++m_iIndentation;
}

void plStandardJSONWriter::EndArray()
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV(
    state == plStandardJSONWriter::Array || state == plStandardJSONWriter::NamedArray, "EndArray() must be called in sync with BeginArray().");


  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == plStandardJSONWriter::NamedArray)
    EndVariable();
}

void plStandardJSONWriter::BeginObject(plStringView sName)
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV((state == plStandardJSONWriter::Empty) ||
                  ((state == plStandardJSONWriter::Object || state == plStandardJSONWriter::NamedObject) && !sName.IsEmpty()) ||
                  ((state == plStandardJSONWriter::Array || state == plStandardJSONWriter::NamedArray) && sName.IsEmpty()) ||
                  (state == plStandardJSONWriter::Variable && sName == nullptr),
    "Inside objects you can only begin objects when also giving them a (non-empty) name.\n"
    "Inside arrays you can only nest anonymous objects, so names are forbidden.\n"
    "Inside variables you cannot specify a name again.");

  if (sName != nullptr)
    BeginVariable(sName);

  m_StateStack.PeekBack().m_bValueWasWritten = true;

  if (m_StateStack.PeekBack().m_bRequireComma)
  {
    if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::None)
      OutputString(",");
    else
      OutputString(",\n");

    OutputIndentation();
  }

  if (m_WhitespaceMode >= plJSONWriter::WhitespaceMode::None)
    OutputString("{");
  else
    OutputString("{\n");

  JSONState s;
  s.m_State = (sName == nullptr) ? plStandardJSONWriter::Object : plStandardJSONWriter::NamedObject;
  m_StateStack.PushBack(s);
  ++m_iIndentation;

  OutputIndentation();
}

void plStandardJSONWriter::EndObject()
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;
  PLASMA_IGNORE_UNUSED(state);
  PLASMA_ASSERT_DEV(
    state == plStandardJSONWriter::Object || state == plStandardJSONWriter::NamedObject, "EndObject() must be called in sync with BeginObject().");

  const State CurState = m_StateStack.PeekBack().m_State;

  End();

  if (CurState == plStandardJSONWriter::NamedObject)
    EndVariable();
}

void plStandardJSONWriter::End()
{
  const plStandardJSONWriter::State state = m_StateStack.PeekBack().m_State;

  if (m_StateStack.PeekBack().m_State == plStandardJSONWriter::Array || m_StateStack.PeekBack().m_State == plStandardJSONWriter::NamedArray)
  {
    --m_iIndentation;

    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("]");
    else
      OutputString(" ]");
  }


  m_StateStack.PopBack();
  m_StateStack.PeekBack().m_bRequireComma = true;

  if (state == plStandardJSONWriter::Object || state == plStandardJSONWriter::NamedObject)
  {
    --m_iIndentation;

    if (m_WhitespaceMode < plJSONWriter::WhitespaceMode::None)
      OutputString("\n");

    OutputIndentation();
    OutputString("}");
  }
}


void plStandardJSONWriter::WriteBinaryData(plStringView sDataType, const void* pData, plUInt32 uiBytes, plStringView sValueString)
{
  CommaWriter cw(this);

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("{\"$t\":\"");
  else
    OutputString("{ \"$t\" : \"");

  OutputString(sDataType);

  if (!sValueString.IsEmpty())
  {
    if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
      OutputString("\",\"$v\":\"");
    else
      OutputString("\", \"$v\" : \"");

    OutputString(sValueString);
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\",\"$b\":\"0x");
  else
    OutputString("\", \"$b\" : \"0x");

  plStringBuilder s;

  plUInt8* pBytes = (plUInt8*)pData;

  for (plUInt32 i = 0; i < uiBytes; ++i)
  {
    s.Format("{0}", plArgU((plUInt32)*pBytes, 2, true, 16, true));
    ++pBytes;

    OutputString(s.GetData());
  }

  if (m_WhitespaceMode >= WhitespaceMode::NewlinesOnly)
    OutputString("\"}");
  else
    OutputString("\" }");
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StandardJSONWriter);
