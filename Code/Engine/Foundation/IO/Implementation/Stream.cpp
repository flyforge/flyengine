#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/Stream.h>
#include <Foundation/IO/StringDeduplicationContext.h>
#include <Foundation/Strings/String.h>

plStreamReader::plStreamReader() = default;
plStreamReader::~plStreamReader() = default;

plResult plStreamReader::ReadString(plStringBuilder& ref_sBuilder)
{
  if (auto context = plStringDeduplicationReadContext::GetContext())
  {
    ref_sBuilder = context->DeserializeString(*this);
  }
  else
  {
    plUInt32 uiCount = 0;
    PLASMA_SUCCEED_OR_RETURN(ReadDWordValue(&uiCount));

    if (uiCount > 0)
    {
      // We access the string builder directly here to
      // read the string efficiently with one allocation
      ref_sBuilder.m_Data.Reserve(uiCount + 1);
      ref_sBuilder.m_Data.SetCountUninitialized(uiCount);
      ReadBytes(ref_sBuilder.m_Data.GetData(), uiCount);
      ref_sBuilder.m_uiCharacterCount = uiCount;
      ref_sBuilder.AppendTerminator();
    }
    else
    {
      ref_sBuilder.Clear();
    }
  }

  return PLASMA_SUCCESS;
}

plResult plStreamReader::ReadString(plString& ref_sString)
{
  plStringBuilder tmp;
  const plResult res = ReadString(tmp);
  ref_sString = tmp;

  return res;
}

plStreamWriter::plStreamWriter() = default;
plStreamWriter::~plStreamWriter() = default;

plResult plStreamWriter::WriteString(const plStringView sStringView)
{
  const plUInt32 uiCount = sStringView.GetElementCount();

  if (auto context = plStringDeduplicationWriteContext::GetContext())
  {
    context->SerializeString(sStringView, *this);
  }
  else
  {
    PLASMA_SUCCEED_OR_RETURN(WriteDWordValue(&uiCount));
    if (uiCount > 0)
    {
      PLASMA_SUCCEED_OR_RETURN(WriteBytes(sStringView.GetStartPointer(), uiCount));
    }
  }

  return PLASMA_SUCCESS;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_Stream);
