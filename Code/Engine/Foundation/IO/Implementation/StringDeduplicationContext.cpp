#include <Foundation/FoundationPCH.h>

#include <Foundation/IO/StringDeduplicationContext.h>

static const plTypeVersion s_uiStringDeduplicationVersion = 1;

PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plStringDeduplicationWriteContext)

plStringDeduplicationWriteContext::plStringDeduplicationWriteContext(plStreamWriter& ref_originalStream)
  : plSerializationContext()
  , m_OriginalStream(ref_originalStream)
{
}

plStringDeduplicationWriteContext::~plStringDeduplicationWriteContext() = default;

plStreamWriter& plStringDeduplicationWriteContext::Begin()
{
  PLASMA_ASSERT_DEV(m_TempStreamStorage.GetStorageSize64() == 0, "Begin() can only be called once on a string deduplication context.");

  m_TempStreamWriter.SetStorage(&m_TempStreamStorage);

  return m_TempStreamWriter;
}

plResult plStringDeduplicationWriteContext::End()
{
  // We set the context manual to null here since we need normal
  // string serialization to write the de-duplicated map
  SetContext(nullptr);

  m_OriginalStream.WriteVersion(s_uiStringDeduplicationVersion);

  const plUInt64 uiNumEntries = m_DeduplicatedStrings.GetCount();
  m_OriginalStream << uiNumEntries;

  plMap<plUInt32, plHybridString<64>> StringsSortedByIndex;

  // Build a new map from index to string so we can use a plain
  // array for serialization and lookup purposes
  for (const auto& it : m_DeduplicatedStrings)
  {
    StringsSortedByIndex.Insert(it.Value(), std::move(it.Key()));
  }

  // Write the new map entries, but just the strings since the indices are linear ascending
  for (const auto& it : StringsSortedByIndex)
  {
    m_OriginalStream << it.Value();
  }

  // Now append the original stream
  PLASMA_SUCCEED_OR_RETURN(m_TempStreamStorage.CopyToStream(m_OriginalStream));

  return PLASMA_SUCCESS;
}

void plStringDeduplicationWriteContext::SerializeString(const plStringView& sString, plStreamWriter& ref_writer)
{
  bool bAlreadDeduplicated = false;
  auto it = m_DeduplicatedStrings.FindOrAdd(sString, &bAlreadDeduplicated);

  if (!bAlreadDeduplicated)
  {
    it.Value() = m_DeduplicatedStrings.GetCount() - 1;
  }

  ref_writer << it.Value();
}

plUInt32 plStringDeduplicationWriteContext::GetUniqueStringCount() const
{
  return m_DeduplicatedStrings.GetCount();
}


PLASMA_IMPLEMENT_SERIALIZATION_CONTEXT(plStringDeduplicationReadContext)

plStringDeduplicationReadContext::plStringDeduplicationReadContext(plStreamReader& inout_stream)
  : plSerializationContext()
{
  // We set the context manually to nullptr to get the original string table
  SetContext(nullptr);

  // Read the string table first
  /*auto version =*/inout_stream.ReadVersion(s_uiStringDeduplicationVersion);

  plUInt64 uiNumEntries = 0;
  inout_stream >> uiNumEntries;

  m_DeduplicatedStrings.Reserve(static_cast<plUInt32>(uiNumEntries));

  for (plUInt64 i = 0; i < uiNumEntries; ++i)
  {
    plStringBuilder s;
    inout_stream >> s;

    m_DeduplicatedStrings.PushBackUnchecked(std::move(s));
  }

  SetContext(this);
}

plStringDeduplicationReadContext::~plStringDeduplicationReadContext() = default;

plStringView plStringDeduplicationReadContext::DeserializeString(plStreamReader& ref_reader)
{
  plUInt32 uiIndex;
  ref_reader >> uiIndex;

  if (uiIndex >= m_DeduplicatedStrings.GetCount())
  {
    PLASMA_ASSERT_DEBUG(uiIndex < m_DeduplicatedStrings.GetCount(), "Failed to read data from file.");
    return {};
  }

  return m_DeduplicatedStrings[uiIndex].GetView();
}


PLASMA_STATICLINK_FILE(Foundation, Foundation_IO_Implementation_StringDeduplicationContext);
