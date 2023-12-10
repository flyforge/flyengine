#pragma once

#include <Foundation/Containers/HashSet.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/IO/SerializationContext.h>

class plStreamWriter;
class plStreamReader;

/// \brief This class allows for writing type versions to a stream in a centralized place so that
/// each object doesn't need to write its own version manually.
///
/// To use, create an object of this type on the stack, call Begin() and use the returned
/// plStreamWriter for subsequent serialization operations. Call AddType to add a type and its parent types to the version table.
/// Call End() once you want to finish writing the type versions.
class PLASMA_FOUNDATION_DLL plTypeVersionWriteContext : public plSerializationContext<plTypeVersionWriteContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plTypeVersionWriteContext);

public:
  plTypeVersionWriteContext();
  ~plTypeVersionWriteContext();

  /// \brief Call this method to begin collecting type version info. You need to use the returned stream writer for subsequent serialization operations until
  /// End() is called.
  plStreamWriter& Begin(plStreamWriter& ref_originalStream);

  /// \brief Ends the type version collection and writes the data to the original stream.
  plResult End();

  /// \brief Adds the given type and its parent types to the version table.
  void AddType(const plRTTI* pRtti);

  /// \brief Manually write the version table to the given stream.
  /// Can be used instead of Begin()/End() if all necessary types are available in one place anyways.
  void WriteTypeVersions(plStreamWriter& inout_stream) const;

  /// \brief Returns the original stream that was passed to Begin().
  plStreamWriter& GetOriginalStream() { return *m_pOriginalStream; }

protected:
  plStreamWriter* m_pOriginalStream = nullptr;

  plDefaultMemoryStreamStorage m_TempStreamStorage;
  plMemoryStreamWriter m_TempStreamWriter;

  plHashSet<const plRTTI*> m_KnownTypes;
};

/// \brief Use this class to restore type versions written to a stream using a plTypeVersionWriteContext.
class PLASMA_FOUNDATION_DLL plTypeVersionReadContext : public plSerializationContext<plTypeVersionReadContext>
{
  PLASMA_DECLARE_SERIALIZATION_CONTEXT(plTypeVersionReadContext);

public:
  /// \brief Reads the type version table from the stream
  plTypeVersionReadContext(plStreamReader& inout_stream);
  ~plTypeVersionReadContext();

  plUInt32 GetTypeVersion(const plRTTI* pRtti) const;

protected:
  plHashTable<const plRTTI*, plUInt32> m_TypeVersions;
};
