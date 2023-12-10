#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/TypeVersionContext.h>
#include <Foundation/Logging/Log.h>
#include <Foundation/Reflection/Reflection.h>

static const plTypeVersion s_uiTypeVersionContextVersion = 1;

PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plTypeVersionWriteContext)

plTypeVersionWriteContext::plTypeVersionWriteContext() = default;
plTypeVersionWriteContext::~plTypeVersionWriteContext() = default;

plStreamWriter& plTypeVersionWriteContext::Begin(plStreamWriter& ref_originalStream)
{
  m_pOriginalStream = &ref_originalStream;

  PLASMA_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a type version context.");
  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

plResult plTypeVersionWriteContext::End()
{
  PLASMA_ASSERT_DEV(m_pOriginalStream != nullptr, "End() called before Begin()");

  WriteTypeVersions(*m_pOriginalStream);

  // Now append the original stream
  PLASMA_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(*m_pOriginalStream));

  return PLASMA_SUCCESS;
}

void plTypeVersionWriteContext::AddType(const plRTTI* pRtti)
{
  if (m_KnownTypes.Insert(pRtti) == false)
  {
    if (const plRTTI* pParentRtti = pRtti->GetParentType())
    {
      AddType(pParentRtti);
    }
  }
}

void plTypeVersionWriteContext::WriteTypeVersions(plStreamWriter& inout_stream) const
{
  inout_stream.WriteVersion(s_uiTypeVersionContextVersion);

  const plUInt32 uiNumTypes = m_KnownTypes.GetCount();
  inout_stream << uiNumTypes;

  plMap<plString, const plRTTI*> sortedTypes;
  for (auto pType : m_KnownTypes)
  {
    sortedTypes.Insert(pType->GetTypeName(), pType);
  }

  for (const auto& it : sortedTypes)
  {
    inout_stream << it.Key();
    inout_stream << it.Value()->GetTypeVersion();
  }
}

//////////////////////////////////////////////////////////////////////////

PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plTypeVersionReadContext)

plTypeVersionReadContext::plTypeVersionReadContext(plStreamReader& inout_stream)
{
  auto version = inout_stream.ReadVersion(s_uiTypeVersionContextVersion);
  PLASMA_IGNORE_UNUSED(version);

  plUInt32 uiNumTypes = 0;
  inout_stream >> uiNumTypes;

  plStringBuilder sTypeName;
  plUInt32 uiTypeVersion;

  for (plUInt32 i = 0; i < uiNumTypes; ++i)
  {
    inout_stream >> sTypeName;
    inout_stream >> uiTypeVersion;

    if (const plRTTI* pType = plRTTI::FindTypeByName(sTypeName))
    {
      m_TypeVersions.Insert(pType, uiTypeVersion);
    }
    else
    {
      plLog::Warning("Ignoring unknown type '{}'", sTypeName);
    }
  }
}

plTypeVersionReadContext::~plTypeVersionReadContext() = default;

plUInt32 plTypeVersionReadContext::GetTypeVersion(const plRTTI* pRtti) const
{
  plUInt32 uiVersion = plInvalidIndex;
  m_TypeVersions.TryGetValue(pRtti, uiVersion);

  return uiVersion;
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_TypeVersionContext);
