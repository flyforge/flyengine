#include <Foundation/FoundationPCH.h>

#include <Foundation/Containers/HybridArray.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Strings/StringBuilder.h>
#include <Foundation/Types/Enum.h>

// C-style strings
// No read equivalent for C-style strings (but can be read as plString & plStringBuilder instances)

plStreamWriter& operator<<(plStreamWriter& inout_stream, const char* szValue)
{
  plStringView szView(szValue);
  inout_stream.WriteString(szView).AssertSuccess();

  return inout_stream;
}

plStreamWriter& operator<<(plStreamWriter& inout_stream, plStringView sValue)
{
  inout_stream.WriteString(sValue).AssertSuccess();

  return inout_stream;
}

// plStringBuilder

plStreamWriter& operator<<(plStreamWriter& inout_stream, const plStringBuilder& sValue)
{
  inout_stream.WriteString(sValue.GetView()).AssertSuccess();
  return inout_stream;
}

plStreamReader& operator>>(plStreamReader& inout_stream, plStringBuilder& out_sValue)
{
  inout_stream.ReadString(out_sValue).AssertSuccess();
  return inout_stream;
}



PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StreamOperations);
