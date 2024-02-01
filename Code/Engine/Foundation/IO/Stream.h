
#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/ArrayBase.h>
#include <Foundation/Containers/HashTable.h>
#include <Foundation/Containers/Map.h>
#include <Foundation/Containers/Set.h>
#include <Foundation/Containers/SmallArray.h>
#include <Foundation/Math/Math.h>
#include <Foundation/Memory/EndianHelper.h>

using plTypeVersion = plUInt16;

template <plUInt16 Size, typename AllocatorWrapper>
struct plHybridString;

using plString = plHybridString<32, plDefaultAllocatorWrapper>;

/// \brief Interface for binary in (read) streams.
class PL_FOUNDATION_DLL plStreamReader
{
  PL_DISALLOW_COPY_AND_ASSIGN(plStreamReader);

public:
  /// \brief Constructor
  plStreamReader();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~plStreamReader();

  /// \brief Reads a raw number of bytes into the read buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
  virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead) = 0; // [tested]

  /// \brief Helper method to read a word value correctly (copes with potentially different endianess)
  template <typename T>
  plResult ReadWordValue(T* pWordValue); // [tested]

  /// \brief Helper method to read a dword value correctly (copes with potentially different endianess)
  template <typename T>
  plResult ReadDWordValue(T* pDWordValue); // [tested]

  /// \brief Helper method to read a qword value correctly (copes with potentially different endianess)
  template <typename T>
  plResult ReadQWordValue(T* pQWordValue); // [tested]

  /// \brief Reads an array of elements from the stream
  template <typename ArrayType, typename ValueType>
  plResult ReadArray(plArrayBase<ValueType, ArrayType>& inout_array); // [tested]

  /// \brief Reads a small array of elements from the stream
  template <typename ValueType, plUInt16 uiSize, typename AllocatorWrapper>
  plResult ReadArray(plSmallArray<ValueType, uiSize, AllocatorWrapper>& ref_array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, plUInt32 uiSize>
  plResult ReadArray(ValueType (&array)[uiSize]);

  /// \brief Reads a set
  template <typename KeyType, typename Comparer>
  plResult ReadSet(plSetBase<KeyType, Comparer>& inout_set); // [tested]

  /// \brief Reads a map
  template <typename KeyType, typename ValueType, typename Comparer>
  plResult ReadMap(plMapBase<KeyType, ValueType, Comparer>& inout_map); // [tested]

  /// \brief Read a hash table (note that the entry order is not stable)
  template <typename KeyType, typename ValueType, typename Hasher>
  plResult ReadHashTable(plHashTableBase<KeyType, ValueType, Hasher>& inout_hashTable); // [tested]

  /// \brief Reads a string into an plStringBuilder
  plResult ReadString(plStringBuilder& ref_sBuilder); // [tested]

  /// \brief Reads a string into an plString
  plResult ReadString(plString& ref_sString);


  /// \brief Helper method to skip a number of bytes (implementations of the stream reader may implement this more efficiently for example)
  virtual plUInt64 SkipBytes(plUInt64 uiBytesToSkip)
  {
    plUInt8 uiTempBuffer[1024];

    plUInt64 uiBytesSkipped = 0;

    while (uiBytesSkipped < uiBytesToSkip)
    {
      plUInt64 uiBytesToRead = plMath::Min<plUInt64>(uiBytesToSkip - uiBytesSkipped, 1024);

      plUInt64 uiBytesRead = ReadBytes(uiTempBuffer, uiBytesToRead);

      uiBytesSkipped += uiBytesRead;

      // Terminate early if the stream didn't read as many bytes as we requested (EOF for example)
      if (uiBytesRead < uiBytesToRead)
        break;
    }

    return uiBytesSkipped;
  }

  PL_ALWAYS_INLINE plTypeVersion ReadVersion(plTypeVersion expectedMaxVersion);
};

/// \brief Interface for binary out (write) streams.
class PL_FOUNDATION_DLL plStreamWriter
{
  PL_DISALLOW_COPY_AND_ASSIGN(plStreamWriter);

public:
  /// \brief Constructor
  plStreamWriter();

  /// \brief Virtual destructor to ensure correct cleanup
  virtual ~plStreamWriter();

  /// \brief Writes a raw number of bytes from the buffer, this is the only method which has to be implemented to fully implement the
  /// interface.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) = 0; // [tested]

  /// \brief Flushes the stream, may be implemented (not necessary to implement the interface correctly) so that user code can ensure that
  /// content is written
  virtual plResult Flush() // [tested]
  {
    return PL_SUCCESS;
  }

  /// \brief Helper method to write a word value correctly (copes with potentially different endianess)
  template <typename T>
  plResult WriteWordValue(const T* pWordValue); // [tested]

  /// \brief Helper method to write a dword value correctly (copes with potentially different endianess)
  template <typename T>
  plResult WriteDWordValue(const T* pDWordValue); // [tested]

  /// \brief Helper method to write a qword value correctly (copes with potentially different endianess)
  template <typename T>
  plResult WriteQWordValue(const T* pQWordValue); // [tested]

  /// \brief Writes a type version to the stream
  PL_ALWAYS_INLINE void WriteVersion(plTypeVersion version);

  /// \brief Writes an array of elements to the stream
  template <typename ArrayType, typename ValueType>
  plResult WriteArray(const plArrayBase<ValueType, ArrayType>& array); // [tested]

  /// \brief Writes a small array of elements to the stream
  template <typename ValueType, plUInt16 uiSize>
  plResult WriteArray(const plSmallArrayBase<ValueType, uiSize>& array);

  /// \brief Writes a C style fixed array
  template <typename ValueType, plUInt32 uiSize>
  plResult WriteArray(const ValueType (&array)[uiSize]);

  /// \brief Writes a set
  template <typename KeyType, typename Comparer>
  plResult WriteSet(const plSetBase<KeyType, Comparer>& set); // [tested]

  /// \brief Writes a map
  template <typename KeyType, typename ValueType, typename Comparer>
  plResult WriteMap(const plMapBase<KeyType, ValueType, Comparer>& map); // [tested]

  /// \brief Writes a hash table (note that the entry order might change on read)
  template <typename KeyType, typename ValueType, typename Hasher>
  plResult WriteHashTable(const plHashTableBase<KeyType, ValueType, Hasher>& hashTable); // [tested]

  /// \brief Writes a string
  plResult WriteString(const plStringView sStringView); // [tested]
};

// Contains the helper methods of both interfaces
#include <Foundation/IO/Implementation/Stream_inl.h>

// Standard operators for overloads of common data types
#include <Foundation/IO/Implementation/StreamOperations_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsMath_inl.h>

#include <Foundation/IO/Implementation/StreamOperationsOther_inl.h>
