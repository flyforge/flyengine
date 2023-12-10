#pragma once

#include <Foundation/Types/UniquePtr.h>

struct plMemoryMappedFileImpl;

/// \brief Allows to map an entire file into memory for random access
class PLASMA_FOUNDATION_DLL plMemoryMappedFile
{
  PLASMA_DISALLOW_COPY_AND_ASSIGN(plMemoryMappedFile);

public:
  plMemoryMappedFile();
  ~plMemoryMappedFile();

  enum class Mode
  {
    None,      ///< Currently no file is mapped
    ReadOnly,  ///< File is mapped for read-only access
    ReadWrite, ///< File is mapped for read/write access
  };

  /// \brief The start point for interpreting byte offsets into the memory
  enum class OffsetBase
  {
    Start, ///< Byte offsets are relative to the start of the mapped memory
    End,   ///< Byte offsets are relative to the end of the mapped memory. Increasing positive values go towards the start
           ///< of the memory.
  };

#if PLASMA_ENABLED(PLASMA_SUPPORTS_MEMORY_MAPPED_FILE) || defined(PLASMA_DOCS)
  /// \brief Attempts to open the given file and map it into memory
  ///
  /// \param szAbsolutePath must be an absolute path to the file that should be mapped.
  ///        The file also must exist and have a size larger than zero bytes.
  /// \param mode How to map the file into memory.
  plResult Open(plStringView sAbsolutePath, Mode mode);
#endif

#if PLASMA_ENABLED(PLASMA_SUPPORTS_SHARED_MEMORY) || defined(PLASMA_DOCS)
  /// \brief Attempts to open or create the given shared memory block addressed by szSharedName
  ///
  /// \param szSharedName The name of the shared memory region.
  /// \param uiSize The size of the memory which should be mapped.
  /// \param mode How to map the file into memory.
  plResult OpenShared(plStringView sSharedName, plUInt64 uiSize, Mode mode);
#endif

  /// \brief Removes the memory mapping. Outstanding modifications will be written back to disk at this point.
  void Close();

  /// \brief Returns the mode with which the file was opened or None, if is currently not in use.
  Mode GetMode() const;

  /// \brief Returns the size (in bytes) of the memory mapping. Zero if no file is mapped at the moment.
  plUInt64 GetFileSize() const;

  /// \brief Returns a pointer for reading the mapped file. Asserts that the memory mapping was done successfully.
  const void* GetReadPointer(plUInt64 uiOffset = 0, OffsetBase base = OffsetBase::Start) const;

  /// \brief Returns a pointer for writing the mapped file. Asserts that the memory mapping was successful and the mode was ReadWrite.
  void* GetWritePointer(plUInt64 uiOffset = 0, OffsetBase base = OffsetBase::Start);

private:
  plUniquePtr<plMemoryMappedFileImpl> m_pImpl;
};
