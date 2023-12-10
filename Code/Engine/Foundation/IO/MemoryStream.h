#pragma once

#include <Foundation/Basics.h>
#include <Foundation/Containers/HybridArray.h>
#include <Foundation/IO/Stream.h>
#include <Foundation/Strings/String.h>
#include <Foundation/Types/RefCounted.h>

class plMemoryStreamReader;
class plMemoryStreamWriter;

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Instances of this class act as storage for memory streams
class PLASMA_FOUNDATION_DLL plMemoryStreamStorageInterface
{
public:
  plMemoryStreamStorageInterface();
  virtual ~plMemoryStreamStorageInterface();

  /// \brief Returns the number of bytes that are currently stored. Asserts that the stored amount is less than 4GB.
  plUInt32 GetStorageSize32() const
  {
    PLASMA_ASSERT_ALWAYS(GetStorageSize64() <= plMath::MaxValue<plUInt32>(), "The memory stream storage object has grown beyond 4GB. The code using it has to be adapted to support this.");
    return (plUInt32)GetStorageSize64();
  }

  /// \brief Returns the number of bytes that are currently stored.
  virtual plUInt64 GetStorageSize64() const = 0; // [tested]

  /// \brief Clears the entire storage. All readers and writers must be reset to start from the beginning again.
  virtual void Clear() = 0;

  /// \brief Deallocates any allocated memory that's not needed to hold the currently stored data.
  virtual void Compact() = 0;

  /// \brief Returns the amount of bytes that are currently allocated on the heap.
  virtual plUInt64 GetHeapMemoryUsage() const = 0;

  /// \brief Copies all data from the given stream into the storage.
  void ReadAll(plStreamReader& inout_stream, plUInt64 uiMaxBytes = plMath::MaxValue<plUInt64>());

  /// \brief Reserves N bytes of storage.
  virtual void Reserve(plUInt64 uiBytes) = 0;

  /// \brief Writes the entire content of the storage to the provided stream.
  virtual plResult CopyToStream(plStreamWriter& inout_stream) const = 0;

  /// \brief Returns a read-only plArrayPtr that represents a contiguous area in memory which starts at the given first byte.
  ///
  /// This piece of memory can be read/copied/modified in one operation (memcpy etc).
  /// The next byte after this slice may be located somewhere entirely different in memory.
  /// Call GetContiguousMemoryRange() again with the next byte after this range, to get access to the next memory area.
  ///
  /// Chunks may differ in size.
  virtual plArrayPtr<const plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) const = 0;

  /// Non-const overload of GetContiguousMemoryRange().
  virtual plArrayPtr<plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) = 0;

private:
  virtual void SetInternalSize(plUInt64 uiSize) = 0;

  friend class plMemoryStreamReader;
  friend class plMemoryStreamWriter;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Templated implementation of plMemoryStreamStorageInterface that adapts most standard pl containers to the interface.
///
/// Note that plMemoryStreamContainerStorage assumes contiguous storage, so using an plDeque for storage will not work.
template <typename CONTAINER>
class plMemoryStreamContainerStorage : public plMemoryStreamStorageInterface
{
public:
  /// \brief Creates the storage object for a memory stream. Use \a uiInitialCapacity to reserve some memory up front.
  plMemoryStreamContainerStorage(plUInt32 uiInitialCapacity = 0, plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator())
    : m_Storage(pAllocator)
  {
    m_Storage.Reserve(uiInitialCapacity);
  }

  virtual plUInt64 GetStorageSize64() const override { return m_Storage.GetCount(); }
  virtual void Clear() override { m_Storage.Clear(); }
  virtual void Compact() override { m_Storage.Compact(); }
  virtual plUInt64 GetHeapMemoryUsage() const override { return m_Storage.GetHeapMemoryUsage(); }

  virtual void Reserve(plUInt64 uiBytes) override
  {
    PLASMA_ASSERT_DEV(uiBytes <= plMath::MaxValue<plUInt32>(), "plMemoryStreamContainerStorage only supports 32 bit addressable sizes.");
    m_Storage.Reserve(static_cast<plUInt32>(uiBytes));
  }

  virtual plResult CopyToStream(plStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_Storage.GetData(), m_Storage.GetCount());
  }

  virtual plArrayPtr<const plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return plArrayPtr<const plUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<plUInt32>(uiStartByte));
  }

  virtual plArrayPtr<plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) override
  {
    if (uiStartByte >= m_Storage.GetCount())
      return {};

    return plArrayPtr<plUInt8>(m_Storage.GetData() + uiStartByte, m_Storage.GetCount() - static_cast<plUInt32>(uiStartByte));
  }

  /// \brief The data is guaranteed to be contiguous.
  const plUInt8* GetData() const { return &m_Storage[0]; }

private:
  virtual void SetInternalSize(plUInt64 uiSize) override
  {
    PLASMA_ASSERT_DEV(uiSize <= plMath::MaxValue<plUInt32>(), "Storage that large is not supported.");
    m_Storage.SetCountUninitialized(static_cast<plUInt32>(uiSize));
  }

  CONTAINER m_Storage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// plContiguousMemoryStreamStorage holds internally an plHybridArray<plUInt8, 256>, to prevent allocations when only small temporary memory streams
/// are needed. That means it will have a memory overhead of that size.
/// Also it reallocates memory on demand, and the data is guaranteed to be contiguous. This may be desirable,
/// but can have a high performance overhead when data grows very large.
class PLASMA_FOUNDATION_DLL plContiguousMemoryStreamStorage : public plMemoryStreamContainerStorage<plHybridArray<plUInt8, 256>>
{
public:
  plContiguousMemoryStreamStorage(plUInt32 uiInitialCapacity = 0, plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator())
    : plMemoryStreamContainerStorage<plHybridArray<plUInt8, 256>>(uiInitialCapacity, pAllocator)
  {
  }
};

/// \brief The default implementation for memory stream storage.
///
/// This implementation of plMemoryStreamStorageInterface handles use cases both from very small to extremely large storage needs.
/// It starts out with some inplace memory that can accommodate small amounts of data.
/// To grow, additional chunks of data are allocated. No memory ever needs to be copied to grow the container.
/// However, that also means that the memory isn't stored in one contiguous array, therefore data has to be accessed piece-wise
/// through GetContiguousMemoryRange().
class PLASMA_FOUNDATION_DLL plDefaultMemoryStreamStorage final : public plMemoryStreamStorageInterface
{
public:
  plDefaultMemoryStreamStorage(plUInt32 uiInitialCapacity = 0, plAllocatorBase* pAllocator = plFoundation::GetDefaultAllocator());
  ~plDefaultMemoryStreamStorage();

  virtual void Reserve(plUInt64 uiBytes) override; // [tested]

  virtual plUInt64 GetStorageSize64() const override; // [tested]
  virtual void Clear() override;
  virtual void Compact() override;
  virtual plUInt64 GetHeapMemoryUsage() const override;
  virtual plResult CopyToStream(plStreamWriter& inout_stream) const override;
  virtual plArrayPtr<const plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) const override; // [tested]
  virtual plArrayPtr<plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) override;             // [tested]

private:
  virtual void SetInternalSize(plUInt64 uiSize) override;

  void AddChunk(plUInt32 uiMinimumSize);

  struct Chunk
  {
    plUInt64 m_uiStartOffset = 0;
    plArrayPtr<plUInt8> m_Bytes;
  };

  plHybridArray<Chunk, 16> m_Chunks;

  plUInt64 m_uiCapacity = 0;
  plUInt64 m_uiInternalSize = 0;
  plUInt8 m_InplaceMemory[512]; // used for the very first bytes, might cover small memory streams without an allocation
  mutable plUInt32 m_uiLastChunkAccessed = 0;
  mutable plUInt64 m_uiLastByteAccessed = 0;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Wrapper around an existing container to implement plMemoryStreamStorageInterface
template <typename CONTAINER>
class plMemoryStreamContainerWrapperStorage : public plMemoryStreamStorageInterface
{
public:
  plMemoryStreamContainerWrapperStorage(CONTAINER* pContainer) { m_pStorage = pContainer; }

  virtual plUInt64 GetStorageSize64() const override { return m_pStorage->GetCount(); }
  virtual void Clear() override { m_pStorage->Clear(); }
  virtual void Compact() override { m_pStorage->Compact(); }
  virtual plUInt64 GetHeapMemoryUsage() const override { return m_pStorage->GetHeapMemoryUsage(); }

  virtual void Reserve(plUInt64 uiBytes) override
  {
    PLASMA_ASSERT_DEV(uiBytes <= plMath::MaxValue<plUInt32>(), "plMemoryStreamContainerWrapperStorage only supports 32 bit addressable sizes.");
    m_pStorage->Reserve(static_cast<plUInt32>(uiBytes));
  }

  virtual plResult CopyToStream(plStreamWriter& inout_stream) const override
  {
    return inout_stream.WriteBytes(m_pStorage->GetData(), m_pStorage->GetCount());
  }

  virtual plArrayPtr<const plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) const override
  {
    if (uiStartByte >= m_pStorage->GetCount())
      return {};

    return plArrayPtr<const plUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<plUInt32>(uiStartByte));
  }

  virtual plArrayPtr<plUInt8> GetContiguousMemoryRange(plUInt64 uiStartByte) override
  {
    if (uiStartByte >= m_pStorage->GetCount())
      return {};

    return plArrayPtr<plUInt8>(m_pStorage->GetData() + uiStartByte, m_pStorage->GetCount() - static_cast<plUInt32>(uiStartByte));
  }

private:
  virtual void SetInternalSize(plUInt64 uiSize) override
  {
    PLASMA_ASSERT_DEV(uiSize <= plMath::MaxValue<plUInt32>(), "plMemoryStreamContainerWrapperStorage only supports up to 4GB sizes.");
    m_pStorage->SetCountUninitialized(static_cast<plUInt32>(uiSize));
  }

  CONTAINER* m_pStorage;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A reader which can access a memory stream.
///
/// Please note that the functions exposed by this object are not thread safe! If access to the same plMemoryStreamStorage object from
/// multiple threads is desired please create one instance of plMemoryStreamReader per thread.
class PLASMA_FOUNDATION_DLL plMemoryStreamReader : public plStreamReader
{
public:
  /// \brief Pass the memory storage object from which to read from.
  /// Pass nullptr if you are going to set the storage stream later via SetStorage().
  plMemoryStreamReader(const plMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~plMemoryStreamReader();

  /// \brief Sets the storage object upon which to operate. Resets the read position to zero.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(const plMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiReadPosition = 0;
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual plUInt64 SkipBytes(plUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(plUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position
  plUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  plUInt32 GetByteCount32() const; // [tested]
  plUInt64 GetByteCount64() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(plStringView sDebugSourceInformation);

private:
  const plMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  plString m_sDebugSourceInformation;

  plUInt64 m_uiReadPosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief A writer which can access a memory stream
///
/// Please note that the functions exposed by this object are not thread safe!
class PLASMA_FOUNDATION_DLL plMemoryStreamWriter : public plStreamWriter
{
public:
  /// \brief Pass the memory storage object to which to write to.
  plMemoryStreamWriter(plMemoryStreamStorageInterface* pStreamStorage = nullptr);

  ~plMemoryStreamWriter();

  /// \brief Sets the storage object upon which to operate. Resets the write position to the end of the storage stream.
  /// Pass nullptr if you want to detach from any previous storage stream, for example to ensure its reference count gets properly reduced.
  void SetStorage(plMemoryStreamStorageInterface* pStreamStorage)
  {
    m_pStreamStorage = pStreamStorage;
    m_uiWritePosition = 0;
    if (m_pStreamStorage)
      m_uiWritePosition = m_pStreamStorage->GetStorageSize64();
  }

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override; // [tested]

  /// \brief Sets the write position to be used
  void SetWritePosition(plUInt64 uiWritePosition); // [tested]

  /// \brief Returns the current write position
  plUInt64 GetWritePosition() const { return m_uiWritePosition; }

  /// \brief Returns the total stored bytes in the memory stream
  plUInt32 GetByteCount32() const; // [tested]
  plUInt64 GetByteCount64() const; // [tested]

private:
  plMemoryStreamStorageInterface* m_pStreamStorage = nullptr;

  plUInt64 m_uiWritePosition = 0;
};


//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////

/// \brief Maps a raw chunk of memory to the plStreamReader interface.
class PLASMA_FOUNDATION_DLL plRawMemoryStreamReader : public plStreamReader
{
public:
  plRawMemoryStreamReader();

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  plRawMemoryStreamReader(const void* pData, plUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard pl container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  plRawMemoryStreamReader(const CONTAINER& container) // [tested]
  {
    Reset(container);
  }

  ~plRawMemoryStreamReader();

  void Reset(const void* pData, plUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(const CONTAINER& container) // [tested]
  {
    Reset(static_cast<const plUInt8*>(container.GetData()), container.GetCount());
  }

  /// \brief Reads either uiBytesToRead or the amount of remaining bytes in the stream into pReadBuffer.
  ///
  /// It is valid to pass nullptr for pReadBuffer, in this case the memory stream position is only advanced by the given number of bytes.
  virtual plUInt64 ReadBytes(void* pReadBuffer, plUInt64 uiBytesToRead) override; // [tested]

  /// \brief Skips bytes in the stream (e.g. for skipping objects which can't be serialized due to missing information etc.)
  virtual plUInt64 SkipBytes(plUInt64 uiBytesToSkip) override; // [tested]

  /// \brief Sets the read position to be used
  void SetReadPosition(plUInt64 uiReadPosition); // [tested]

  /// \brief Returns the current read position in the raw memory block
  plUInt64 GetReadPosition() const { return m_uiReadPosition; }

  /// \brief Returns the total available bytes in the memory stream
  plUInt64 GetByteCount() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(plStringView sDebugSourceInformation);

private:
  const plUInt8* m_pRawMemory = nullptr;

  plUInt64 m_uiChunkSize = 0;
  plUInt64 m_uiReadPosition = 0;

  plString m_sDebugSourceInformation;
};

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////


/// \brief Maps a raw chunk of memory to the plStreamReader interface.
class PLASMA_FOUNDATION_DLL plRawMemoryStreamWriter : public plStreamWriter
{
public:
  plRawMemoryStreamWriter(); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory that is the data storage.
  plRawMemoryStreamWriter(void* pData, plUInt64 uiDataSize); // [tested]

  /// \brief Initialize the raw memory reader with the chunk of memory from a standard pl container.
  /// \note The container must store the data in a contiguous array.
  template <typename CONTAINER>
  plRawMemoryStreamWriter(CONTAINER& ref_container) // [tested]
  {
    Reset(ref_container);
  }

  ~plRawMemoryStreamWriter(); // [tested]

  void Reset(void* pData, plUInt64 uiDataSize); // [tested]

  template <typename CONTAINER>
  void Reset(CONTAINER& ref_container) // [tested]
  {
    Reset(static_cast<plUInt8*>(ref_container.GetData()), ref_container.GetCount());
  }

  /// \brief Returns the total available bytes in the memory stream
  plUInt64 GetStorageSize() const; // [tested]

  /// \brief Returns the number of bytes written to the storage
  plUInt64 GetNumWrittenBytes() const; // [tested]

  /// \brief Allows to set a string as the source of information in the memory stream for debug purposes.
  void SetDebugSourceInformation(plStringView sDebugSourceInformation);

  /// \brief Copies uiBytesToWrite from pWriteBuffer into the memory stream.
  ///
  /// pWriteBuffer must be a valid buffer and must hold that much data.
  virtual plResult WriteBytes(const void* pWriteBuffer, plUInt64 uiBytesToWrite) override; // [tested]

private:
  plUInt8* m_pRawMemory = nullptr;

  plUInt64 m_uiChunkSize = 0;
  plUInt64 m_uiWritePosition = 0;

  plString m_sDebugSourceInformation;
};
