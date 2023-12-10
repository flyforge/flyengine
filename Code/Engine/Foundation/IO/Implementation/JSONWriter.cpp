#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/JSONWriter.h>

plJSONWriter::plJSONWriter() = default;
plJSONWriter::~plJSONWriter() = default;

void plJSONWriter::AddVariableBool(plStringView sName, bool value)
{
  BeginVariable(sName);
  WriteBool(value);
  EndVariable();
}

void plJSONWriter::AddVariableInt32(plStringView sName, plInt32 value)
{
  BeginVariable(sName);
  WriteInt32(value);
  EndVariable();
}

void plJSONWriter::AddVariableUInt32(plStringView sName, plUInt32 value)
{
  BeginVariable(sName);
  WriteUInt32(value);
  EndVariable();
}

void plJSONWriter::AddVariableInt64(plStringView sName, plInt64 value)
{
  BeginVariable(sName);
  WriteInt64(value);
  EndVariable();
}

void plJSONWriter::AddVariableUInt64(plStringView sName, plUInt64 value)
{
  BeginVariable(sName);
  WriteUInt64(value);
  EndVariable();
}

void plJSONWriter::AddVariableFloat(plStringView sName, float value)
{
  BeginVariable(sName);
  WriteFloat(value);
  EndVariable();
}

void plJSONWriter::AddVariableDouble(plStringView sName, double value)
{
  BeginVariable(sName);
  WriteDouble(value);
  EndVariable();
}

void plJSONWriter::AddVariableString(plStringView sName, plStringView value)
{
  BeginVariable(sName);
  WriteString(value);
  EndVariable();
}

void plJSONWriter::AddVariableNULL(plStringView sName)
{
  BeginVariable(sName);
  WriteNULL();
  EndVariable();
}

void plJSONWriter::AddVariableTime(plStringView sName, plTime value)
{
  BeginVariable(sName);
  WriteTime(value);
  EndVariable();
}

void plJSONWriter::AddVariableUuid(plStringView sName, plUuid value)
{
  BeginVariable(sName);
  WriteUuid(value);
  EndVariable();
}

void plJSONWriter::AddVariableAngle(plStringView sName, plAngle value)
{
  BeginVariable(sName);
  WriteAngle(value);
  EndVariable();
}

void plJSONWriter::AddVariableColor(plStringView sName, const plColor& value)
{
  BeginVariable(sName);
  WriteColor(value);
  EndVariable();
}

void plJSONWriter::AddVariableColorGamma(plStringView sName, const plColorGammaUB& value)
{
  BeginVariable(sName);
  WriteColorGamma(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec2(plStringView sName, const plVec2& value)
{
  BeginVariable(sName);
  WriteVec2(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec3(plStringView sName, const plVec3& value)
{
  BeginVariable(sName);
  WriteVec3(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec4(plStringView sName, const plVec4& value)
{
  BeginVariable(sName);
  WriteVec4(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec2I32(plStringView sName, const plVec2I32& value)
{
  BeginVariable(sName);
  WriteVec2I32(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec3I32(plStringView sName, const plVec3I32& value)
{
  BeginVariable(sName);
  WriteVec3I32(value);
  EndVariable();
}

void plJSONWriter::AddVariableVec4I32(plStringView sName, const plVec4I32& value)
{
  BeginVariable(sName);
  WriteVec4I32(value);
  EndVariable();
}

void plJSONWriter::AddVariableQuat(plStringView sName, const plQuat& value)
{
  BeginVariable(sName);
  WriteQuat(value);
  EndVariable();
}

void plJSONWriter::AddVariableMat3(plStringView sName, const plMat3& value)
{
  BeginVariable(sName);
  WriteMat3(value);
  EndVariable();
}

void plJSONWriter::AddVariableMat4(plStringView sName, const plMat4& value)
{
  BeginVariable(sName);
  WriteMat4(value);
  EndVariable();
}

void plJSONWriter::AddVariableDataBuffer(plStringView sName, const plDataBuffer& value)
{
  BeginVariable(sName);
  WriteDataBuffer(value);
  EndVariable();
}

void plJSONWriter::AddVariableVariant(plStringView sName, const plVariant& value)
{
  BeginVariable(sName);
  WriteVariant(value);
  EndVariable();
}

void plJSONWriter::WriteColor(const plColor& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plColor is not supported by this JSON writer.");
}

void plJSONWriter::WriteColorGamma(const plColorGammaUB& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plColorGammaUB is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec2(const plVec2& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec2 is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec3(const plVec3& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec3 is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec4(const plVec4& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec4 is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec2I32(const plVec2I32& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec2I32 is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec3I32(const plVec3I32& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec3I32 is not supported by this JSON writer.");
}

void plJSONWriter::WriteVec4I32(const plVec4I32& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plVec4I32 is not supported by this JSON writer.");
}

void plJSONWriter::WriteQuat(const plQuat& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plQuat is not supported by this JSON writer.");
}

void plJSONWriter::WriteMat3(const plMat3& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plMat3 is not supported by this JSON writer.");
}

void plJSONWriter::WriteMat4(const plMat4& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plMat4 is not supported by this JSON writer.");
}

void plJSONWriter::WriteDataBuffer(const plDataBuffer& value)
{
  PLASMA_REPORT_FAILURE("The complex data type plDateBuffer is not supported by this JSON writer.");
}

void plJSONWriter::WriteVariant(const plVariant& value)
{
  switch (value.GetType())
  {
    case plVariant::Type::Invalid:
      // PLASMA_REPORT_FAILURE("Variant of Type 'Invalid' cannot be written as JSON.");
      WriteNULL();
      return;
    case plVariant::Type::Bool:
      WriteBool(value.Get<bool>());
      return;
    case plVariant::Type::Int8:
      WriteInt32(value.Get<plInt8>());
      return;
    case plVariant::Type::UInt8:
      WriteUInt32(value.Get<plUInt8>());
      return;
    case plVariant::Type::Int16:
      WriteInt32(value.Get<plInt16>());
      return;
    case plVariant::Type::UInt16:
      WriteUInt32(value.Get<plUInt16>());
      return;
    case plVariant::Type::Int32:
      WriteInt32(value.Get<plInt32>());
      return;
    case plVariant::Type::UInt32:
      WriteUInt32(value.Get<plUInt32>());
      return;
    case plVariant::Type::Int64:
      WriteInt64(value.Get<plInt64>());
      return;
    case plVariant::Type::UInt64:
      WriteUInt64(value.Get<plUInt64>());
      return;
    case plVariant::Type::Float:
      WriteFloat(value.Get<float>());
      return;
    case plVariant::Type::Double:
      WriteDouble(value.Get<double>());
      return;
    case plVariant::Type::Color:
      WriteColor(value.Get<plColor>());
      return;
    case plVariant::Type::ColorGamma:
      WriteColorGamma(value.Get<plColorGammaUB>());
      return;
    case plVariant::Type::Vector2:
      WriteVec2(value.Get<plVec2>());
      return;
    case plVariant::Type::Vector3:
      WriteVec3(value.Get<plVec3>());
      return;
    case plVariant::Type::Vector4:
      WriteVec4(value.Get<plVec4>());
      return;
    case plVariant::Type::Vector2I:
      WriteVec2I32(value.Get<plVec2I32>());
      return;
    case plVariant::Type::Vector3I:
      WriteVec3I32(value.Get<plVec3I32>());
      return;
    case plVariant::Type::Vector4I:
      WriteVec4I32(value.Get<plVec4I32>());
      return;
    case plVariant::Type::Quaternion:
      WriteQuat(value.Get<plQuat>());
      return;
    case plVariant::Type::Matrix3:
      WriteMat3(value.Get<plMat3>());
      return;
    case plVariant::Type::Matrix4:
      WriteMat4(value.Get<plMat4>());
      return;
    case plVariant::Type::String:
      WriteString(value.Get<plString>().GetData());
      return;
    case plVariant::Type::StringView:
    {
      plStringBuilder s = value.Get<plStringView>();
      WriteString(s.GetData());
      return;
    }
    case plVariant::Type::Time:
      WriteTime(value.Get<plTime>());
      return;
    case plVariant::Type::Uuid:
      WriteUuid(value.Get<plUuid>());
      return;
    case plVariant::Type::Angle:
      WriteAngle(value.Get<plAngle>());
      return;
    case plVariant::Type::DataBuffer:
      WriteDataBuffer(value.Get<plDataBuffer>());
      return;
    case plVariant::Type::VariantArray:
    {
      BeginArray();

      const auto& ar = value.Get<plVariantArray>();

      for (const auto& val : ar)
      {
        WriteVariant(val);
      }

      EndArray();
    }
      return;

    default:
      break;
  }

  PLASMA_REPORT_FAILURE("The Variant Type {0} is not supported by plJSONWriter::WriteVariant.", value.GetType());
}


bool plJSONWriter::HadWriteError() const
{
  return m_bHadWriteError;
}

void plJSONWriter::SetWriteErrorState()
{
  m_bHadWriteError = true;
}

PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_JSONWriter);
