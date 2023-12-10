
#pragma once

#include <Foundation/Containers/DynamicArray.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>
#include <Foundation/Strings/String.h>

class plStreamWriter;
class plStreamReader;

/// \brief This class allows for automatic deduplication of strings written to a stream.
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// plStreamWriter for subsequent serialization operations. Call End() once you want to finish writing
/// deduplicated strings. For a sample see StreamOperationsTest.cpp
class PLASMA_FOUNDATION_DLL plStringDeduplicationWriteContext : public plSerializationContext<plStringDeduplicationWriteContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plStringDeduplicationWriteContext);

public:
  /// \brief Setup the write context to perform string deduplication.
  plStringDeduplicationWriteContext(plStreamWriter& ref_originalStream);
  ~plStringDeduplicationWriteContext();

  /// \brief Call this method to begin string deduplicaton. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  plStreamWriter& Begin();

  /// \brief Ends the string deduplication and writes the string table to the original stream
  plResult End();

  /// \brief Internal method to serialize a string.
  void SerializeString(const plStringView& sString, plStreamWriter& ref_writer);

  /// \brief Returns the number of unique strings which were serialized with this instance.
  plUInt32 GetUniqueStringCount() const;

  /// \brief Returns the original stream that was passed to the constructor.
  plStreamWriter& GetOriginalStream() { return m_OriginalStream; }

protected:
  plStreamWriter& m_OriginalStream;

  plDefaultMemoryStreamStorage m_TempStreamStorage;
  plMemoryStreamWriter m_TempStreamWriter;

  plMap<plHybridString<64>, plUInt32> m_DeduplicatedStrings;
};

/// \brief This class to restore strings written to a stream using a plStringDeduplicationWriteContext.
class PLASMA_FOUNDATION_DLL plStringDeduplicationReadContext : public plSerializationContext<plStringDeduplicationReadContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plStringDeduplicationReadContext);

public:
  /// \brief Setup the string table used internally.
  plStringDeduplicationReadContext(plStreamReader& inout_stream);
  ~plStringDeduplicationReadContext();

  /// \brief Internal method to deserialize a string.
  plStringView DeserializeString(plStreamReader& ref_reader);

protected:
  plDynamicArray<plHybridString<64>> m_DeduplicatedStrings;
};
